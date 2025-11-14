#ifndef TESTING_TASK_CLIENT_H
#define TESTING_TASK_CLIENT_H


#pragma once
#include <string>
#include <netinet/in.h>

struct Client
{
    int fd;
    sockaddr_in addr;
    std::string buffer;
};



#endif //TESTING_TASK_CLIENT_H