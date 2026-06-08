#pragma once

#include "formatter.h"

namespace Logger {

class DefaultFormatter : public Formatter {
public:
    void Format(const LogMsg& msg, MemoryBuf* buf) override;

};
}