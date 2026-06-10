#pragma once
#include "thread_pool.h"

#include <unordered_map>
#include <queue>
#include <chrono>
#include <unordered_set>

constexpr uint32_t kFalseTag = 0;
constexpr uint32_t kNoRepeatId = 0;


class Executor {
public:
    using Task = MthreadPool::Task;
    using Runner = MthreadPool;
    using TaskRunnerTag = uint32_t;

    ~Executor();

    class ExecutorContext {
    public:
        using TaskRunnerTag = uint32_t;
        using TaskRunnerPtr = std::unique_ptr<Runner>;
    
        TaskRunnerTag AddRunner(TaskRunnerTag tag);
        Runner* GetRunner(TaskRunnerTag tag);

    private:
        TaskRunnerTag GetNewTag();

    private:
        std::mutex mutex_;
        std::unordered_map<TaskRunnerTag, TaskRunnerPtr> map_;
        TaskRunnerTag tag_{1};
    };





    class ExecutorTime {
    private: 
        struct TimeTask;
    public:
        using TimePoint = std::chrono::high_resolution_clock::time_point;
        using TimeDur = std::chrono::high_resolution_clock::duration;
        using RepeatId = uint32_t;

        ExecutorTime();
        bool Start();
        bool Stop();
        bool AddTimeTask(TimeDur delay_time, Task task);
        RepeatId AddRepeatTimeTask(uint32_t repeat_count, TimeDur delay_time, Task task);
        bool CancelRepeatTimeTask(RepeatId id);
    private:
        struct TimeTask {
            TimePoint future_start_time;
            TimeDur delay_time;
            Task task;
            RepeatId id;
            uint32_t repeat_count;
        };

        friend bool operator<(const TimeTask& left, const TimeTask& right) {
            return left.future_start_time > right.future_start_time;
        }      
        
        RepeatId GetNewRepeatId() {
            return ++common_id_;
        }
        TimeTask CreatRepeatTimeTask(uint32_t repeat_count, TimeDur delay_time, RepeatId id ,Task task);
        void CheckAndAddNext(TimeTask task);

    private:
        MthreadPool time_thread{1};
        std::priority_queue<TimeTask> queue_;
        std::unordered_set<RepeatId> repeat_id_set_;
        std::condition_variable time_task_cv_;
        std::mutex time_task_mutex_;

        RepeatId common_id_{1};
        std::atomic<bool> is_running;
        std::atomic<bool> is_shutdown;
    };



    template<typename F, typename... Args>
    bool PostTask(ExecutorContext::TaskRunnerTag tag, F&& f, Args&&... args) {
        Runner* runner = executor_context_.GetRunner(tag);
        if (runner == nullptr) {
            return false;
        }
        return runner->RunTask(std::forward<F>(f), std::forward<Args>(args)...);
    }

    template<typename F, typename... Args>
    auto PostTaskAndGetResult(ExecutorContext::TaskRunnerTag tag, F&& f, Args&&... args)
                           -> std::shared_ptr<std::future<std::invoke_result_t<F, Args...>>> {
        Runner* runner = executor_context_.GetRunner(tag);
        if (runner == nullptr) {
            return nullptr;
        }
        return runner->RunRetTask(std::forward<F>(f), std::forward<Args>(args)...);
        
    }

    template<typename F, typename... Args>
    bool PostDelayTask(ExecutorTime::TimeDur delay_time, ExecutorContext::TaskRunnerTag tag, F&& f, Args&&... args) {

       if (!executor_time_.Start()) {
            return false;
        }

        Task task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto func = [this, tag, task]() {
            this->PostTask(tag, task);
        };
        return executor_time_.AddTimeTask(delay_time, func); 
    }

    template<typename F, typename... Args>
    ExecutorTime::RepeatId PostRepeatDelayTask(uint32_t repeat_count ,ExecutorTime::TimeDur delay_time, ExecutorContext::TaskRunnerTag tag, F&& f, Args&&... args) {
        
        if (!executor_time_.Start()) {
            return kNoRepeatId;
        }

        Task task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto func = [this, tag, task]() {
            this->PostTask(tag, task);
        };
        return executor_time_.AddRepeatTimeTask(repeat_count, delay_time, func); 
    }

    bool CancelRepeatTask(ExecutorTime::RepeatId id) {
        return executor_time_.CancelRepeatTimeTask(id);
    }

    ExecutorContext::TaskRunnerTag AddRunner(ExecutorContext::TaskRunnerTag tag) {
        return executor_context_.AddRunner(tag);
    }


private:
    ExecutorContext executor_context_;
    ExecutorTime executor_time_;

};