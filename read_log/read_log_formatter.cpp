#include "read_log/read_log_formatter.h"

void ReadLogFormatter::SetStandard(std::string& standard) {

}

void ReadLogFormatter::Format(EffectiveMsg& msg, std::string& buf) {
    msg.ParseFromString(buf);

}