#include "log_msg.h"

namespace Logger{

LogMsg::LogMsg(SourceLocationg source, LogLevel level, StringView msg)
        : source_location_(source), level_(level), msg_(msg) {}

LogMsg::LogMsg(LogLevel level, StringView msg) : LogMsg(SourceLocationg{}, level, msg){}

}