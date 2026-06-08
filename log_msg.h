#pragma once

#include "log_common.h"

namespace Logger{

class LogMsg {
public:
    LogMsg(SourceLocationg source, LogLevel level, StringView msg);
    LogMsg(LogLevel level, StringView msg);

    LogMsg(const LogMsg& other) = default;
    LogMsg& operator=(const LogMsg& other) = default;


    SourceLocationg source_location_;
    LogLevel level_;
    StringView msg_;
};
}