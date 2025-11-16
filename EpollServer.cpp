#include "EpollServer.h"
#include "Utils.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

/**
 * Конструктор epoll сервера
 * @param port Порт для прослушивания
 * @param threads Количество потоков в пуле
 * @param token Токен для команды shutdown
 */
EpollServer::EpollServer(int port, int threads, const std::string& token) :
    port_(port),
    shutdownToken_(token),
    shutdownFlag_(false),
    epollFd_(-1),
    listenFd_(-1),
    udpFd_(-1),
    pool_(threads) {}

/**
 * Деструктор - освобождает ресурсы сокетов
 */
EpollServer::~EpollServer() {
    if (listenFd_ != -1) close(listenFd_);
    if (udpFd_ != -1) close(udpFd_);
    if (epollFd_ != -1) close(epollFd_);
}

/**
 * Основной цикл работы сервера
 */
void EpollServer::run() {
    initSockets();  // Инициализация TCP и UDP сокетов
    initEpoll();    // Настройка epoll

    std::cout << "Server started on port: " << port_ << std::endl;

    constexpr int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    // Главный цикл обработки событий
    while (!shutdownFlag_) {
        // Ожидаем события с таймаутом 1 секунда
        int n = epoll_wait(epollFd_, events, MAX_EVENTS, 1000);
        if (n < 0) {
            if (errno == EINTR) continue;  // Игнорируем прерывания
            perror("epoll_wait");
            break;
        }

        // Обрабатываем все произошедшие события
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            // Добавляем обработку события в пул потоков
            pool_.enqueue([this, fd, ev]() {
                handleEvent(fd, ev);
            });
        }
    }

    shutdown();  // Корректное завершение работы
}

/**
 * Инициализация TCP и UDP сокетов
 */
void EpollServer::initSockets() {
    // Создаем TCP и UDP сокеты
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    udpFd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenFd_ < 0 || udpFd_ < 0) {
        perror("socket");
        exit(1);
    }

    // Разрешаем переиспользование порта
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Настраиваем адрес для привязки сокета
    sockaddr_in addr{};
    addr.sin_family = AF_INET;                    // Семейство адресов - IPv4
    addr.sin_addr.s_addr = INADDR_ANY;           // Принимаем подключения со всех сетевых интерфейсов
    addr.sin_port = htons(port_);                // Порт: htons() преобразует число в сетевой порядок байт (big-endian)

    // Привязываем сокеты к порту
    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind TCP");
        exit(1);
    }

    if (bind(udpFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind UDP");
        exit(1);
    }

    // Устанавливаем неблокирующий режим
    makeNonBlocking(listenFd_);
    makeNonBlocking(udpFd_);

    // Начинаем прослушивать TCP соединения
    if (listen(listenFd_, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
    }
}

/**
 * Инициализация epoll для мониторинга сокетов
 */
void EpollServer::initEpoll() {
    // Создаем epoll instance
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        perror("epoll_create1");
        exit(1);
    }

    // Лямбда для добавления файлового дескриптора в epoll
    auto add_fd = [this](int fd) {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;  // Чтение + edge-triggered режим
        ev.data.fd = fd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            perror("epoll_ctl");
            exit(1);
        }
    };

    // Добавляем TCP и UDP сокеты в epoll
    add_fd(listenFd_);
    add_fd(udpFd_);
}

/**
 * Обработка epoll события
 * @param fd Файловый дескриптор, на котором произошло событие
 * @param events Маска произошедших событий
 */
void EpollServer::handleEvent(int fd, uint32_t events) {
    if (fd == listenFd_) {
        handleTcpAccept();  // Новое TCP соединение
    } else if (fd == udpFd_) {
        handleUdpRead();    // Пришла UDP датаграмма
    } else {
        handleTcpRead(fd);  // Данные от TCP клиента
    }
}

/**
 * Принятие новых TCP соединений
 */
void EpollServer::handleTcpAccept() {
    // Обрабатываем все ожидающие соединения (edge-triggered)
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(listenFd_, (sockaddr*)&clientAddr, &len);
        if (clientFd < 0) {
            // Больше нет ожидающих соединений
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            break;
        }

        // Устанавливаем неблокирующий режим для клиента
        makeNonBlocking(clientFd);

        // Добавляем клиента в epoll для мониторинга
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
            perror("epoll_ctl ADD client");
            close(clientFd);
            continue;
        }

        // Сохраняем информацию о клиенте
        clients_[clientFd] = {clientFd, clientAddr, ""};
        tcpTotal_++;
        tcpCurrent_++;

        // Логируем новое подключение
        std::lock_guard<std::mutex> lock(coutMutex_);
        std::cout << "New TCP client " << inet_ntoa(clientAddr.sin_addr)
                  << ":" << ntohs(clientAddr.sin_port) << std::endl;
    }
}

/**
 * Чтение данных от TCP клиента
 * @param fd Файловый дескриптор клиента
 */
void EpollServer::handleTcpRead(int fd) {
    char buf[4096];

    // Читаем все доступные данные (edge-triggered)
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            // Закрываем соединение при ошибке или разрыве
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
            epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            clients_.erase(fd);
            tcpCurrent_--;
            return;
        }

        // Добавляем данные в буфер клиента
        clients_[fd].buffer.append(buf, n);
    }

    // Обрабатываем полные сообщения (до символа новой строки)
    auto& data = clients_[fd].buffer;
    size_t pos;
    while ((pos = data.find('\n')) != std::string::npos) {
        std::string msg = data.substr(0, pos);
        data.erase(0, pos + 1);

        if (msg.empty()) continue;

        // Обработка команд, начинающихся с '/'
        if (msg[0] == '/') {
            // Команда /time - возвращает текущее время
            if (msg.rfind("/time", 0) == 0) {
                sendToClient(fd, currentTime() + "\n");
            }
            // Команда /stats - возвращает статистику
            else if (msg.rfind("/stats", 0) == 0) {
                sendToClient(fd, stats() + "\n");
            }
            // Команда /shutdown - завершает работу сервера
            else if (msg.rfind("/shutdown", 0) == 0) {
                std::string providedToken;

                // Извлекаем переданный токен
                if (msg.size() > 10) {
                    providedToken = msg.substr(10);
                    if (!providedToken.empty() && providedToken[0] == ' ')
                        providedToken.erase(0, 1);
                }

                // Если токен не установлен, разрешаем shutdown без проверки
                if (shutdownToken_.empty()) {
                    sendToClient(fd, "Server shutting down\n");
                    shutdownFlag_ = true;
                }
                // Проверяем корректность токена
                else if (providedToken != shutdownToken_) {
                    sendToClient(fd, "Invalid token\n");
                }
                else {
                    sendToClient(fd, "Server shutting down\n");
                    shutdownFlag_ = true;
                }
            }
            // Неизвестная команда
            else {
                sendToClient(fd, "Unknown command\n");
            }
        }
        // Зеркалирование обычных сообщений
        else {
            sendToClient(fd, msg + "\n");
        }
    }
}

/**
 * Обработка входящих UDP датаграмм
 */
void EpollServer::handleUdpRead() {
    char buf[2048];
    sockaddr_in peer{};
    socklen_t len = sizeof(peer);

    // Читаем датаграмму
    ssize_t n = recvfrom(udpFd_, buf, sizeof(buf) - 1, 0, (sockaddr*)&peer, &len);
    if (n <= 0) return;
    buf[n] = '\0';

    std::string msg(buf);
    // Создаем уникальный ключ для UDP пира
    std::string key = std::string(inet_ntoa(peer.sin_addr)) + ":" + std::to_string(ntohs(peer.sin_port));
    udpPeers_.insert(key);

    // Обрабатываем команды аналогично TCP
    if (msg[0] == '/') {
        if (msg.rfind("/time", 0) == 0)
            sendToUdpPeer(peer, currentTime());
        else if (msg.rfind("/stats", 0) == 0)
            sendToUdpPeer(peer, stats());
        else
            sendToUdpPeer(peer, "Unknown command");
    } else {
        sendToUdpPeer(peer, msg);  // Зеркалирование
    }
}

/**
 * Отправка сообщения TCP клиенту
 * @param fd Файловый дескриптор клиента
 * @param msg Сообщение для отправки
 */
void EpollServer::sendToClient(int fd, const std::string& msg) {
    send(fd, msg.c_str(), msg.size(), 0);
}

/**
 * Отправка сообщения UDP пиру
 * @param peer Адрес получателя
 * @param msg Сообщение для отправки
 */
void EpollServer::sendToUdpPeer(const sockaddr_in& peer, const std::string& msg) {
    sendto(udpFd_, msg.c_str(), msg.size(), 0, (sockaddr*)&peer, sizeof(peer));
}

/**
 * Получение текущего времени
 * @return Строка с текущим временем
 */
std::string EpollServer::currentTime() const {
    return nowString();
}

/**
 * Получение статистики сервера
 * @return Строка со статистикой
 */
std::string EpollServer::stats() const {
    return "TCP total=" + std::to_string(tcpTotal_) +
           " current=" + std::to_string(tcpCurrent_) +
           " UDP unique=" + std::to_string(udpPeers_.size());
}

/**
 * Корректное завершение работы сервера
 */
void EpollServer::shutdown() {
    std::lock_guard<std::mutex> lock(coutMutex_);
    std::cout << "Server shutting down" << std::endl;
    // Уведомляем всех клиентов о завершении работы
    for (auto& [fd, c] : clients_) {
        sendToClient(fd, "Server shutting down\n");
        close(fd);
    }
    clients_.clear();
}