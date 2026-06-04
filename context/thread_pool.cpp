#include "thread_pool.h"

bool MthreadPool::Stop() {
    if (is_shutdown_.load()) {
        return true;
    }
    is_shutdown_.store(true);
    task_cv_.notify_all();

    for (auto& threadptr : work_threads_) {
        if (threadptr != nullptr && threadptr->joinable()) {
            threadptr->join();
        }
    }
    work_threads_.clear();
    is_running_.store(false);
    return true;
}

bool MthreadPool::Start() {
    if (is_running_.load()) {
        return true;
    }
    if (is_shutdown_.load()) {
        return false;
    }

    auto size = count_.load();
    for (auto i = 0; i < size; i++) {
        if (CreateThread() == false) {
            return false;
        }
    }

    is_running_.store(true);
    return true;
}

bool MthreadPool::CreateThread() {
    auto func = [this]() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(task_mutex_);
                task_cv_.wait(lock, [this](){ return !tasks_.empty() || is_shutdown_.load();});
                //立即停，不管任务队列中剩余的任务。
                if (is_shutdown_.load()) {
                    break;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();

        }
    };
    ThreadPtr thread_ptr = std::make_shared<std::thread>(func); 

    std::unique_lock<std::mutex>(task_mutex_);
    work_threads_.push_back(std::move(thread_ptr));
    return true;
}

bool MthreadPool::AddTask(Task task) {
    if (is_running_.load() == false) {
        return false;
    }
    if (is_shutdown_.load()) {
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(task_mutex_);
        tasks_.push(std::move(task));
    }
    task_cv_.notify_one();
    return true;
}