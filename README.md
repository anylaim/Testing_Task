# Testing Task - Epoll Server

Асинхронный сервер на C++ с использованием epoll для обработки TCP/UDP подключений.

## Функции
- Обработка команд `/time`, `/stats`, `/shutdown`
- Зеркалирование сообщений
- Systemd service
- .deb пакет

## Установка

### Установка из .deb пакета
sudo dpkg -i testing-task-1.0.0-Linux.deb
sudo systemctl start Testing_Task
sudo systemctl status Testing_Task
Сборка из исходников
bash
git clone <repository>
cd Testing_Task
mkdir build && cd build
cmake ..
make deb-package
sudo dpkg -i testing-task-1.0.0-Linux.deb

# Запуск с параметрами по умолчанию
Порт: 8080

Потоки: 4

Токен:

sudo systemctl start Testing_Task
sudo systemctl status Testing_Task

# Запуск с кастомными параметрами

# Остановите systemd сервис
sudo systemctl stop Testing_Task

# Запустите вручную с нужными параметрами
sudo /usr/bin/Testing_Task --port 9090 --threads 8 --shutdown-token mytoken

# Или в фоне
sudo nohup /usr/bin/Testing_Task --port 9090 --threads 8 --shutdown-token mytoken &

# Остановка сервера
- Через команду shutdown

echo "/shutdown" | nc localhost 8080
- Через systemd

sudo systemctl stop Testing_Task
- Принудительная остановка

# Если сервис не отвечает
sudo systemctl kill Testing_Task

# Или найти процесс
sudo pkill -f Testing_Task


### Параметры запуска
Формат команды:

/usr/bin/Testing_Task [OPTIONS]

Options:

--port PORT - Server port (default: 8080)
--threads NUM - Number of threads (default: hardware_concurrency)
--shutdown-token TOKEN - Shutdown token (default: qwerty)

# Примеры параметров:

# Все по умолчанию
/usr/bin/Testing_Task

# Кастомный порт
/usr/bin/Testing_Task --port 9090

# Полная кастомизация
/usr/bin/Testing_Task --port 9090 --threads 8 --shutdown-token mysecret

# Управление сервисом

# Запуск
sudo systemctl start Testing_Task

# Остановка
sudo systemctl stop Testing_Task

# Перезапуск
sudo systemctl restart Testing_Task

# Статус
sudo systemctl status Testing_Task

# Логи
sudo journalctl -u Testing_Task -f

# Включение автозапуска
sudo systemctl enable Testing_Task



========== Переустановить пакет ==========

# Удалить старую версию
sudo dpkg -r testing-task

# Установить новую
sudo dpkg -i testing-task-1.0.0-Linux.deb 

========== Или принудительно обновить ==========

# Переустановить поверх существующей
sudo dpkg -i testing-task-1.0.0-Linux.deb

# Если ошибка зависимостей
sudo apt-get install -f 
Перезапустить сервис:
    sudo systemctl daemon-reload
    sudo systemctl restart Testing_Task
    sudo systemctl status Testing_Task