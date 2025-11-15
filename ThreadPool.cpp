#include "ThreadPool.h"

/**
 * Конструктор пула потоков
 * @param threads количество рабочих потоков для создания
 */
ThreadPool::ThreadPool(size_t threads) : stop_(false) {
    // Создаем указанное количество рабочих потоков
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            // Бесконечный цикл выполнения задач
            while (true) {
                std::function<void()> task;

                {
                    // Блокируем мьютекс для безопасного доступа к очереди задач
                    std::unique_lock<std::mutex> lock(queueMutex_);

                    // Ожидаем появления задачи или сигнала остановки
                    condition_.wait(lock, [this]{
                        return stop_ || !tasks_.empty();
                    });

                    // Если пул остановлен и задач нет - завершаем поток
                    if (stop_ && tasks_.empty()) return;

                    // Извлекаем задачу из очереди
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                // Выполняем задачу вне критической секции
                task();
            }
        });
    }
}

/**
 * Добавление задачи в очередь на выполнение
 * @param task функция для выполнения в одном из рабочих потоков
 */
void ThreadPool::enqueue(std::function<void()> task) {
    {
        // Блокируем мьютекс для безопасного добавления в очередь
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.push(std::move(task));
    }
    // Уведомляем один из ожидающих потоков о новой задаче
    condition_.notify_one();
}

/**
 * Деструктор пула потоков - останавливает все потоки
 */
ThreadPool::~ThreadPool() {
    // Устанавливаем флаг остановки
    stop_ = true;
    // Уведомляем все потоки о необходимости завершения
    condition_.notify_all();
    // Ожидаем завершения всех рабочих потоков
    for (auto &w : workers_) w.join();
}