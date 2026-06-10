#include "executor.h"

Executor::ExecutorContext::TaskRunnerTag Executor::ExecutorContext::GetNewTag() {
    while (true) {
        if (map_.find(tag_) == map_.end()) {
            return tag_;
        }
        tag_++;
    }
}

Executor::ExecutorContext::TaskRunnerTag Executor::ExecutorContext::AddRunner(Executor::ExecutorContext::TaskRunnerTag tag) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (map_.find(tag) != map_.end()) {
        tag = GetNewTag();
    }
    TaskRunnerPtr runner_ptr = std::make_unique<Runner>(1);
    bool runner_start = runner_ptr->Start();
    if (!runner_start) {
        return kFalseTag;
    }
    map_.insert({tag, std::move(runner_ptr)});
    return tag;
}

Executor::Runner* Executor::ExecutorContext::GetRunner(Executor::ExecutorContext::TaskRunnerTag tag) {
    Runner* runner_ptr;
    std::lock_guard<std::mutex> lock(mutex_);
    if (map_.find(tag) == map_.end()) {
        return nullptr;
    }
    runner_ptr = map_[tag].get();
    return runner_ptr;
}




Executor::ExecutorTime::ExecutorTime() {
    is_running.store(false);
    is_shutdown.store(false);
}

bool Executor::ExecutorTime::Start() {
    std::unique_lock<std::mutex> lock(time_task_mutex_);
    if (is_running.load()) {
        return true;

    }
    if (is_shutdown.load()) {
        return false;
    }
    is_running.store(true);
    time_thread.Start();
    auto func = [this]() {
        while (true) {
            std::unique_lock<std::mutex> lock(time_task_mutex_);
            time_task_cv_.wait(lock, [this](){return !queue_.empty() || is_shutdown.load();});
            if (is_shutdown.load()) {
                break;
            }
            if (queue_.empty()) {
                continue;
            }
            auto now_time_point = std::chrono::high_resolution_clock::now();
            TimeTask will_do_task = queue_.top();
            if (will_do_task.future_start_time > now_time_point) {
                time_task_cv_.wait_until(lock, will_do_task.future_start_time);
                continue;
            }
            queue_.pop();
            if (will_do_task.id != kNoRepeatId) {
                if (repeat_id_set_.find(will_do_task.id) == repeat_id_set_.end()) {
                    continue;
                }                
            }
            lock.unlock();
            will_do_task.task();
            CheckAndAddNext(will_do_task);
  
        }
    };
    lock.unlock();
    time_thread.RunTask(func);
    return true;
}

bool Executor::ExecutorTime::Stop() {
    if (is_shutdown.load()) {
        return true;
    }
    is_shutdown.store(true);
    time_task_cv_.notify_all();
    time_thread.Stop();
    std::lock_guard<std::mutex> lock(time_task_mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
    repeat_id_set_.clear();
    is_running.store(false);
    return true;
}

bool Executor::ExecutorTime::AddTimeTask(TimeDur delay_time, Task task ) {
    std::lock_guard<std::mutex> lock(time_task_mutex_);    
    if (!is_running.load()) {
        return false;
    }
    if (is_shutdown.load()) {
        return false;
    }
    TimeTask time_task = CreatRepeatTimeTask(0, delay_time, kNoRepeatId, task);
    queue_.push(time_task);
    time_task_cv_.notify_one();
    return true;
}


Executor::ExecutorTime::TimeTask Executor::ExecutorTime::CreatRepeatTimeTask(uint32_t repeat_count, TimeDur delay_time, RepeatId id ,Task task) {
    TimeTask time_task;
    time_task.task = task;
    time_task.delay_time = delay_time;
    auto now_time_point = std::chrono::high_resolution_clock::now();
    time_task.future_start_time = now_time_point + delay_time;
    time_task.repeat_count = repeat_count;
    time_task.id = id;
    return time_task;
}



Executor::ExecutorTime::RepeatId Executor::ExecutorTime::AddRepeatTimeTask(uint32_t repeat_count, TimeDur delay_time, Task task) {
    std::lock_guard<std::mutex> lock(time_task_mutex_);
    if (!is_running.load()) {
        return kNoRepeatId;
    }
    if (is_shutdown.load()) {
        return kNoRepeatId;
    }
    TimeTask time_task = CreatRepeatTimeTask(repeat_count, delay_time, GetNewRepeatId(), task);
    repeat_id_set_.insert(time_task.id);
    queue_.push(time_task);
    time_task_cv_.notify_one();
    return time_task.id;
}


bool Executor::ExecutorTime::CancelRepeatTimeTask(RepeatId id) {
    std::lock_guard<std::mutex> lock(time_task_mutex_);
    return repeat_id_set_.erase(id);
}

void Executor::ExecutorTime::CheckAndAddNext(TimeTask task) {
    std::lock_guard<std::mutex> lock(time_task_mutex_);
    if (task.id != kNoRepeatId) {
        if (repeat_id_set_.find(task.id) != repeat_id_set_.end()) {
            if (task.repeat_count == 0) {
                repeat_id_set_.erase(task.id);
                return ;
            }
            TimeTask new_task = CreatRepeatTimeTask(task.repeat_count - 1, task.delay_time, task.id, task.task);
            queue_.push(new_task);
            time_task_cv_.notify_one();
            return ;
        }
    }
}





Executor::~Executor() {
    executor_time_.Stop();
    
}