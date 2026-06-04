#include "log_msg.h"

namespace Logger{

LogMsg::LogMsg(SourceLocationg source, LogLever lever, StringView msg)
        : source_location_(source), lever_(lever), msg_(msg) {}

LogMsg::LogMsg(LogLever lever, StringView msg) : LogMsg(SourceLocationg{}, lever, msg){}

}