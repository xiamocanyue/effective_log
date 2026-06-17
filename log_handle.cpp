#include "log_handle.h"
#include "sinks/sink.h"


namespace Logger{

LogHandle::LogHandle(LogSinkPtr sink) {
    log_sinks_.push_back(std::move(sink));
}

LogHandle::LogHandle(LogSinkPtrList sinks) {
    for (auto& sink : sinks) {
        log_sinks_.push_back(std::move(sink));
    }
}

void LogHandle::SetLevel(LogLevel level) {
    min_lever_ = level;
}

LogLevel LogHandle::GetLevel() {
    return min_lever_;
}

void LogHandle::CreateLogMsg(SourceLocation source, LogLevel level, StringView msg) {
    if (!ShouldLog(level)) {
        return;
    }
    LogMsg log_msg(source, level, msg);
    SendLogMsg(log_msg);
}

void LogHandle::SendLogMsg(LogMsg& log_msg) {
    for (auto& sink : log_sinks_) {
        sink->Log(log_msg);
    }
}



}