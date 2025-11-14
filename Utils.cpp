#ifndef TESTING_TASK_UTILS_H
#define TESTING_TASK_UTILS_H


#include "Utils.h"
#include <fcntl.h>
#include <chrono>
#include <ctime>

int makeNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::string nowString()
{
    auto t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}



#endif