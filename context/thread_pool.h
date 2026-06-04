#pragma once 

#include <vector>
#include <queue>
#include <atomic>
#include <memory>

#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>

class MthreadPool {
public:
    using Task = std::function<void()>;

    explicit MthreadPool(const uint32_t count = 1) {
        is_running_.store(false);
        is_shutdown_.store(false);
        count_.store(count);
    }

    ~MthreadPool() {
        Stop();
    }

    MthreadPool(const MthreadPool& other) = delete;
    MthreadPool& operator=(const MthreadPool& other) = delete;

    bool Stop();
    bool Start();
    bool AddTask(Task task);

    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) -> std::shared_ptr<std::future<std::invoke_result_t<F, Args...>>> {
        using result_type = std::invoke_result_t<F, Args...>;
        auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<result_type()>>(func);
        auto task_future_ptr = std::make_shared<std::future<result_type>>(task_ptr->get_future());

        Task task = [task_ptr]() {
            (*task_ptr)();
        };

        if (AddTask(task) == false) {
            return nullptr;
        }
        return task_future_ptr;
    }


    template<typename F, typename... Args>
    bool RunTask(F&& f, Args&&... args) {
        Task task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        return AddTask(task);
    }

    template<typename F, typename... Args>
    auto RunRetTask(F&& f, Args&&... args) {
        return Submit(std::forward<F>(f), std::forward<Args>(args)...);
    }



private:
    bool CreateThread();


private:
    using ThreadPtr = std::shared_ptr<std::thread>;

private:
    std::queue<Task> tasks_;
    std::vector<ThreadPtr> work_threads_;

    std::mutex task_mutex_;
    std::condition_variable task_cv_;

    std::atomic<bool> is_shutdown_;
    std::atomic<bool> is_running_;
    std::atomic<uint32_t> count_;

};