#include "formatter/effective_formatter.h"
#include "msg_protobuf/effective_msg.pb.h"

#include <chrono>
#include <unistd.h>

namespace Logger {

void EffectiveFormatter::Format(const LogMsg& msg, MemoryBuf* buf) {
    EffectiveMsg effective_msg;
    effective_msg.set_level(static_cast<int>(msg.level_));
    auto time_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    effective_msg.set_timestamp(time_duration.count());
    effective_msg.set_pid(static_cast<size_t>(getpid()));
    effective_msg.set_tid(static_cast<size_t>(gettid()));
    effective_msg.set_line(msg.source_location_.line_);
    effective_msg.set_file_name(msg.source_location_.file_name_);
    effective_msg.set_func_name(msg.source_location_.func_name_);
    effective_msg.set_log_info(msg.msg_);

    size_t len = effective_msg.ByteSizeLong();
    buf->resize(len);
    effective_msg.SerializeToArray(buf->data(), len);
}


}