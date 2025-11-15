#ifndef TESTING_TASK_UTILS_H
#define TESTING_TASK_UTILS_H

#include "Utils.h"
#include <fcntl.h>
#include <chrono>
#include <ctime>

/**
 * Устанавливает неблокирующий режим для файлового дескриптора
 * @param fd Файловый дескриптор сокета
 * @return Результат операции fcntl или -1 при ошибке
 */
int makeNonBlocking(int fd)
{
    // Получаем текущие флаги файлового дескриптора
    int flags = fcntl(fd, F_GETFL, 0);
    // Устанавливаем флаг O_NONBLOCK для неблокирующих операций
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * Возвращает текущее время в формате "YYYY-MM-DD HH:MM:SS"
 * Используется для команды /time
 * @return Строка с текущей датой и временем
 */
std::string nowString()
{
    // Получаем текущее системное время
    auto t = std::time(nullptr);
    char buf[32];
    // Форматируем время в читаемый строковый формат
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

#endif