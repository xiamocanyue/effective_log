#pragma once

#include "formatter/formatter.h"

namespace Logger {

class EffectiveFormatter : public Formatter {
public:
    void Format(const LogMsg& msg, MemoryBuf* buf)  override;

};
}
