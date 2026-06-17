#pragma once

#include "log_common.h"

namespace Logger{

class LogMsg {
public:
    LogMsg(SourceLocation source, LogLevel level, StringView msg);
    LogMsg(LogLevel level, StringView msg);

    LogMsg(const LogMsg& other) = default;
    LogMsg& operator=(const LogMsg& other) = default;


    SourceLocation source_location_;
    LogLevel level_;
    StringView msg_;
};
}