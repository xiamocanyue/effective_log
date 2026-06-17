#pragma once 

#include "msg_protobuf/effective_msg.pb.h"

#include <string>

class ReadLogFormatter {
public:
    void SetStandard(std::string& standard);
    void Format(EffectiveMsg& msg, std::string& buf);
};