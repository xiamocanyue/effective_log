#pragma once

#include "log_handle.h"
#include <fmt/core.h>
#include <fmt/format.h> 

namespace Logger{
class VariadicLogHandle : public LogHandle {
public:
    using LogHandle::LogHandle;

    template<typename... Args>
    void Log(SourceLocationg source, LogLevel level, fmt::format_string<Args...> fmt_str, Args&&... args){
        CreateAndSendLogMsg(source, level, fmt_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(LogLevel level, fmt::format_string<Args...> fmt_str, Args&&... args){
        Log(SourceLocationg{__FILE__, __LINE__, __func__}, level, fmt_str, std::forward<Args>(args)...);
    }


private:
    template<typename... Args>
    void CreateAndSendLogMsg(SourceLocationg source, LogLevel level, fmt::basic_string_view<char> fmt_str, Args&&... args) {
        if(!ShouldLog(level)) {
            return;
        }
        fmt::memory_buffer buf;
        fmt::detail::vformat_to(buf, fmt_str, fmt::make_format_args(std::forward<Args>(args)...));

        LogMsg log_msg(source, level, StringView(buf.data(), buf.size()));

        LogHandle::SendLogMsg(log_msg);

    }


};
}