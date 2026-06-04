#pragma once

#include "log_common.h"

namespace Logger{



class LogMsg {
public:
    LogMsg(SourceLocationg source, LogLever lever, StringView msg);
    LogMsg(LogLever lever, StringView msg);

    LogMsg(const LogMsg& other) = default;
    LogMsg& operator=(const LogMsg& other) = default;

private:
    SourceLocationg source_location_;
    LogLever lever_;
    StringView msg_;
};
}