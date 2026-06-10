#pragma once 

#include <memory>

#include "log_msg.h"
#include "formatter/formatter.h"

namespace Logger {

class LogSink {
public:

    virtual ~LogSink(){}

    virtual void Log(LogMsg& msg) = 0;

    virtual void SetFormatter(std::unique_ptr<Formatter> formatter) = 0;

    virtual void Flush() {}

};
}