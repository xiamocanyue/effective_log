#pragma once

#include "executor.h"

class Context {
public:
    static Context* GetInstance() {
        static Context* instance = new Context;
        return instance;
    }
    Context(const Context& other) = delete;
    Context& operator=(const Context& other) = delete;

    Executor* GetExecutor() {
        return &executor_;
    }

    Executor::TaskRunnerTag AddRunner(Executor::ExecutorContext::TaskRunnerTag tag) {
        return executor_.AddRunner(tag);
    }



private:
    Context() = default;

private:
    Executor executor_;

};

#define CONTEXT Context::GetInstance()
#define EXECUTOR CONTEXT->GetExecutor()
#define ADD_TASK_RUNNER(tag) CONTEXT->AddRunner(tag)
#define POST_TASK(tag, task) EXECUTOR->PostTask(tag, task)
#define POST_AND_RETURN(tag, task) EXECUTOR->PostTaskAndGetResult(tag, task)
#define WAIT_TASK_IDLE(tag) EXECUTOR->PostTaskAndGetResult(tag, []() {})->wait()
#define POST_REPEATED_TASK(repeat_count, delay_time, tag, task) \
    EXECUTOR->PostRepeatDelayTask(repeat_count, delay_time, tag, task)
#define CANCEL_REPEATED_TASK(id) EXECUTOR->CancelRepeatTimeTask(id)