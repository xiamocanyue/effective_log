#pragma once

#include "log_msg.h"


namespace Logger {

class Formatter {
public:
    virtual ~Formatter() {}

    virtual void Format(const LogMsg& msg, MemoryBuf* buf) = 0;
 
};

}