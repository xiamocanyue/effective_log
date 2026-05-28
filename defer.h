#pragma once
#include <functional>

class ExecuteDefer {
public:
    template<typename F>
    ExecuteDefer(F&& f) {
        func_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    }

    ~ExecuteDefer() {
        if (func_) {
            func_();
        }
    }

    ExecuteDefer(const ExecuteDefer&) = delete;
    ExecuteDefer& operator=(const ExecuteDefer&) = delete;
    ExecuteDefer(ExecuteDefer&&) = delete;
    ExecuteDefer& operator=(ExecuteDefer&&) = delete;


private:
    std::function<void()> func_;
};

#define DEFER_CONCAT_INSERT(a, b) a##b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_INSERT(a, b)
#define MAKE_DEFER(line) ExecuteDefer DEFER_CONCAT(defer, line) = [&]()

#undef DEFER
#define DEFER MAKE_DEFER(__LINE__)