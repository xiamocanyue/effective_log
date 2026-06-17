#include "formatter/default_formatter.h"

#include <chrono>
#include <ctime>
#include <unistd.h>

namespace Logger {
    
constexpr const char* kLogLevelStr[] = {"trace", "debug", "info", "warn", "error", "critical", "off"};

void DefaultFormatter::Format(const LogMsg& msg, MemoryBuf* buf) {
    auto now_time_point = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now_time_point);
    char timebuf[64] = {0};
    if (std::strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", std::localtime(&t))) {
        buf->append(timebuf, std::strlen(timebuf));
    }
    buf->append("[", 1);
    buf->append(kLogLevelStr[static_cast<int>(msg.level_)]);
    buf->append("] [", 3);
    buf->append(msg.source_location_.file_name_.data(), msg.source_location_.file_name_.size());
    buf->append(":", 1);
    buf->append(std::to_string(msg.source_location_.line_));
    buf->append("] [", 3);
    buf->append(std::to_string(static_cast<size_t>(getpid())));
    buf->append(":", 1);
    buf->append(std::to_string(static_cast<size_t>(gettid())));
    buf->append("]", 1);
    buf->append(msg.msg_.data(), msg.msg_.size());
}


}