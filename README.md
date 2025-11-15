# Testing Task - Epoll Server

Асинхронный сервер на C++ с использованием epoll для обработки TCP/UDP подключений.

## Функции
- Обработка команд `/time`, `/stats`, `/shutdown`
- Зеркалирование сообщений
- Systemd service
- .deb пакет

## Установка

### Сборка из исходников
```bash

git clone https://github.com/anylaim/Testing_Task

cd Testing_Task

mkdir build && cd build

cmake ..

make deb-package

sudo dpkg -i testing-task-1.0.0-Linux.deb
```

### Установка из .deb пакета
sudo dpkg -i testing-task-1.0.0-Linux.deb

sudo systemctl start Testing_Task

sudo systemctl status Testing_Task


# Запуск с параметрами по умолчанию
Порт: 8080

Потоки: 4

Токен:

sudo systemctl start Testing_Task
sudo systemctl status Testing_Task

# Запуск с кастомными параметрами

- Остановите systemd сервис
sudo systemctl stop Testing_Task

- Запустите вручную с нужными параметрами
```bash
sudo /usr/bin/Testing_Task 9090 --threads 8 --shutdown-token mytoken
```
- Или в фоне
```bash
sudo nohup /usr/bin/Testing_Task 9090 --threads 8 --shutdown-token mytoken &
```


### Параметры запуска
Формат команды:

/usr/bin/Testing_Task [OPTIONS]

Options:

PORT - Server port (default: 8080)
--threads NUM - Number of threads (default: hardware_concurrency)
--shutdown-token TOKEN - Shutdown token (default: qwerty)

# Примеры параметров:

# Все по умолчанию
```bash
/usr/bin/Testing_Task
```
# Кастомный порт
```bash
/usr/bin/Testing_Task 9090
```
# Полная кастомизация
```bash
/usr/bin/Testing_Task 9090 --threads 8 --shutdown-token mysecret
```
# Управление сервисом

# Запуск
```bash
sudo systemctl start Testing_Task
```
# Остановка
```bash
sudo systemctl stop Testing_Task
```
# Перезапуск
```bash
sudo systemctl restart Testing_Task
```
# Статус
```bash
sudo systemctl status Testing_Task
```
# Логи
```bash
sudo journalctl -u Testing_Task -f
```
# Включение автозапуска
```bash
sudo systemctl enable Testing_Task
```


# ========== Переустановить пакет ==========

### Удалить старую версию
```bash
sudo dpkg -r testing-task
```
### Установить новую
```bash
sudo dpkg -i testing-task-1.0.0-Linux.deb 
```
# ========== Или принудительно обновить ==========

### Переустановить поверх существующей
```bash
sudo dpkg -i testing-task-1.0.0-Linux.deb
```
### Если ошибка зависимостей
```bash
sudo apt-get install -f 
```
Перезапустить сервис:
```bash
    sudo systemctl daemon-reload
    sudo systemctl restart Testing_Task
    sudo systemctl status Testing_Task
```