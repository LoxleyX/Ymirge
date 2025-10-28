#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queueMutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });

                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }

                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }

    condition_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::parallelFor(size_t start, size_t end,
                             std::function<void(size_t)> func,
                             size_t grainSize) {
    if (start >= end) return;

    size_t numTasks = (end - start + grainSize - 1) / grainSize;
    std::vector<std::future<void>> futures;
    futures.reserve(numTasks);

    for (size_t i = start; i < end; i += grainSize) {
        size_t chunkEnd = std::min(i + grainSize, end);

        futures.push_back(enqueue([func, i, chunkEnd] {
            for (size_t idx = i; idx < chunkEnd; ++idx) {
                func(idx);
            }
        }));
    }

    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.get();
    }
}
