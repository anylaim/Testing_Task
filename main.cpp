#include "EpollServer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Проверка минимального количества аргументов
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [--threads N] [--shutdown-token TOKEN]\n";
        return 1;
    }

    char* end;
    // Парсинг порта из первого аргумента
    int port = std::strtol(argv[1], &end, 10);
    // Валидация порта: должен быть числом в диапазоне 1-65535
    if (*end != '\0' || port <= 0 || port > 65535) {
        std::cerr << "Error: Invalid port number: " << argv[1] << "\n";
        return 1;
    }

    // Установка значений по умолчанию
    int threads = std::thread::hardware_concurrency(); // Количество ядер процессора
    std::string token; // Токен для команды shutdown

    // Обработка опциональных аргументов
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        // Обработка параметра --threads
        if (arg == "--threads" && i + 1 < argc) {
            threads = std::strtol(argv[++i], &end, 10);
            // Валидация количества потоков
            if (*end != '\0' || threads <= 0) {
                std::cerr << "Error: Invalid thread count: " << argv[i] << "\n";
                return 1;
            }
        }
        // Обработка параметра --shutdown-token
        else if (arg == "--shutdown-token" && i + 1 < argc) {
            token = argv[++i]; // Сохраняем токен для завершения работы
        }
    }

    // Создание и запуск epoll сервера
    EpollServer server(port, threads, token);
    server.run();

    return 0;
}