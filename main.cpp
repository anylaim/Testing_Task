#include "EpollServer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [--threads N] [--shutdown-token TOKEN]\n";
        return 1;
    }

    char* end;
    int port = std::strtol(argv[1], &end, 10);
    if (*end != '\0' || port <= 0 || port > 65535) {
        std::cerr << "Error: Invalid port number: " << argv[1] << "\n";
        return 1;
    }

    int threads = std::thread::hardware_concurrency();
    std::string token;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            threads = std::strtol(argv[++i], &end, 10);
            if (*end != '\0' || threads <= 0) {
                std::cerr << "Error: Invalid thread count: " << argv[i] << "\n";
                return 1;
            }
        }
        else if (arg == "--shutdown-token" && i + 1 < argc) {
            token = argv[++i];
        }
    }

    EpollServer server(port, threads, token);
    server.run();
    return 0;
}