#include "EpollServer.h"
#include "Utils.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

EpollServer::EpollServer(int port, int threads, const std::string& token) :
    port_(port),
    shutdownToken_(token),
    shutdownFlag_(false),
    epollFd_(-1),
    listenFd_(-1),
    udpFd_(-1),
    pool_(threads) {}

EpollServer::~EpollServer() {
    if (listenFd_ != -1) close(listenFd_);
    if (udpFd_ != -1) close(udpFd_);
    if (epollFd_ != -1) close(epollFd_);
}

void EpollServer::run() {
    initSockets();
    initEpoll();
    initEpoll();

    std::cout << "Server started on port: " << port_ << std::endl;

    constexpr int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (!shutdownFlag_) {
        int n = epoll_wait(epollFd_, events, MAX_EVENTS, 1000);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            pool_.enqueue([this, fd, ev]() {
                handleEvent(fd, ev);
            });
        }
    }

    shutdown();
}

void EpollServer::initSockets() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    udpFd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenFd_ < 0 || udpFd_ < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind TCP");
        exit(1);
    }

    if (bind(udpFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind UDP");
        exit(1);
    }

    makeNonBlocking(listenFd_);
    makeNonBlocking(udpFd_);

    if (listen(listenFd_, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
    }
}

void EpollServer::initEpoll() {
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        perror("epoll_create1");
        exit(1);
    }

    auto add_fd = [this](int fd) {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            perror("epoll_ctl");
            exit(1);
        }
    };

    add_fd(listenFd_);
    add_fd(udpFd_);
}

void EpollServer::handleEvent(int fd, uint32_t events) {
    if (fd == listenFd_) {
        handleTcpAccept();
    } else if (fd == udpFd_) {
        handleUdpRead();
    } else {
        handleTcpRead(fd);
    }
}

void EpollServer::handleTcpAccept() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(listenFd_, (sockaddr*)&clientAddr, &len);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            break;
        }

        makeNonBlocking(clientFd);

        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
            perror("epoll_ctl ADD client");
            close(clientFd);
            continue;
        }

        clients_[clientFd] = {clientFd, clientAddr, ""};
        tcpTotal_++;
        tcpCurrent_++;

        std::lock_guard<std::mutex> lock(coutMutex_);
        std::cout << "New TCP client " << inet_ntoa(clientAddr.sin_addr)
                  << ":" << ntohs(clientAddr.sin_port) << std::endl;
    }
}

void EpollServer::handleTcpRead(int fd) {
    char buf[4096];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
            epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            clients_.erase(fd);
            tcpCurrent_--;
            return;
        }

        clients_[fd].buffer.append(buf, n);
    }

    auto& data = clients_[fd].buffer;
    size_t pos;
    while ((pos = data.find('\n')) != std::string::npos) {
        std::string msg = data.substr(0, pos);
        data.erase(0, pos + 1);

        if (msg.empty()) continue;

        if (msg[0] == '/') {

            if (msg.rfind("/time", 0) == 0) {
                sendToClient(fd, currentTime() + "\n");
            }

            else if (msg.rfind("/stats", 0) == 0) {
                sendToClient(fd, stats() + "\n");
            }

            else if (msg.rfind("/shutdown", 0) == 0) {
                std::string providedToken;

                if (msg.size() > 10) {
                    providedToken = msg.substr(10);
                    if (!providedToken.empty() && providedToken[0] == ' ')
                        providedToken.erase(0, 1);
                }

                if (shutdownToken_.empty()) {
                    sendToClient(fd, "Server shutting down\n");
                    shutdownFlag_ = true;
                }

                else if (providedToken != shutdownToken_) {
                    sendToClient(fd, "Invalid token\n");
                }
                else {
                    sendToClient(fd, "Server shutting down\n");
                    shutdownFlag_ = true;
                }
            }

            else {
                sendToClient(fd, "Unknown command\n");
            }
        }

        else {
            sendToClient(fd, msg + "\n");
        }
    }

}

void EpollServer::handleUdpRead() {
    char buf[2048];
    sockaddr_in peer{};
    socklen_t len = sizeof(peer);

    ssize_t n = recvfrom(udpFd_, buf, sizeof(buf) - 1, 0,
                         (sockaddr*)&peer, &len);
    if (n <= 0) return;
    buf[n] = '\0';

    std::string msg(buf);
    std::string key = std::string(inet_ntoa(peer.sin_addr)) + ":" + std::to_string(ntohs(peer.sin_port));
    udpPeers_.insert(key);

    if (msg[0] == '/') {
        if (msg.rfind("/time", 0) == 0)
            sendToUdpPeer(peer, currentTime());
        else if (msg.rfind("/stats", 0) == 0)
            sendToUdpPeer(peer, stats());
        else
            sendToUdpPeer(peer, "Unknown command");
    } else {
        sendToUdpPeer(peer, msg);
    }
}

void EpollServer::sendToClient(int fd, const std::string& msg) {
    send(fd, msg.c_str(), msg.size(), 0);
}

void EpollServer::sendToUdpPeer(const sockaddr_in& peer, const std::string& msg) {
    sendto(udpFd_, msg.c_str(), msg.size(), 0, (sockaddr*)&peer, sizeof(peer));
}

std::string EpollServer::currentTime() const {
    return nowString();
}

std::string EpollServer::stats() const {
    return "TCP total=" + std::to_string(tcpTotal_) +
           " current=" + std::to_string(tcpCurrent_) +
           " UDP unique=" + std::to_string(udpPeers_.size());
}

void EpollServer::shutdown() {
    std::lock_guard<std::mutex> lock(coutMutex_);
    std::cout << "Server shutting down" << std::endl;
    for (auto& [fd, c] : clients_) {
        sendToClient(fd, "Server shutting down\n");
        close(fd);
    }
    clients_.clear();
}
