#pragma once

#include "log_msg.h"

#include <memory>
#include <atomic>

#include <vector>
#include <initializer_list>



namespace Logger{
class LogSink;
using LogSinkPtr = std::shared_ptr<LogSink>;
using LogSinkPtrList = std::initializer_list<LogSinkPtr>;

class LogHandle {
public:
    explicit LogHandle(LogSinkPtr sink);
    explicit LogHandle(LogSinkPtrList sinks);

    template<typename T>
    LogHandle(T begin, T end) : LogHandle(LogSinkPtrList(begin, end)) {}

    void SetLevel(LogLevel level);
    LogLevel GetLevel();

    void CreateLogMsg(SourceLocation soucre, LogLevel level, StringView msg);

protected:
    bool ShouldLog(LogLevel level) {
        return level > min_lever_ && !log_sinks_.empty();
    }

    void SendLogMsg(LogMsg& log_msg);

private:
    std::atomic<LogLevel> min_lever_{LogLevel::kInfo};
    std::vector<LogSinkPtr> log_sinks_;

};
}