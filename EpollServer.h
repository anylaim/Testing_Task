#ifndef TESTING_TASK_EPOLLSERVER_H
#define TESTING_TASK_EPOLLSERVER_H


#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include "Client.h"
#include "ThreadPool.h"

class EpollServer {
public:
    EpollServer(int port, int threads, const std::string& token = "");
    ~EpollServer();

    void run();

private:
    void initSockets();
    void initEpoll();
    void handleEvent(int fd, uint32_t events);
    void handleTcpAccept();
    void handleTcpRead(int fd);
    void handleUdpRead();
    void sendToClient(int fd, const std::string& msg);
    void sendToUdpPeer(const sockaddr_in& peer, const std::string& msg);
    void shutdown();

    std::string currentTime() const;
    std::string stats() const;

    int port_;
    std::string shutdownToken_;
    std::atomic<bool> shutdownFlag_;

    int epollFd_;
    int listenFd_;
    int udpFd_;

    ThreadPool pool_;
    std::unordered_map<int, Client> clients_;
    std::unordered_set<std::string> udpPeers_;

    size_t tcpTotal_ = 0;
    size_t tcpCurrent_ = 0;
    mutable std::mutex coutMutex_;
};



#endif