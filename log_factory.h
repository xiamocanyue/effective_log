#pragma once 

#include <memory>

namespace Logger {

class VariadicLogHandle;

class LogFactory {
public:

    LogFactory(const LogFactory& other) = delete;
    LogFactory& operator=(const LogFactory& other) = delete;

    static LogFactory& GetInstance() {
        static LogFactory instance;
        return instance;
    }

    VariadicLogHandle* GetLogHandle() {
        return log_handle_.get();
    }

    void SetLogHandle(std::shared_ptr<VariadicLogHandle> new_log_handle) {
        log_handle_ = new_log_handle;
    }



private:
    LogFactory() = default;
    ~LogFactory() = default;

private:
    std::shared_ptr<VariadicLogHandle> log_handle_;

};


}


