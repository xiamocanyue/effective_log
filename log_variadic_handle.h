#pragma once

#include "log_handle.h"
#include <fmt/core.h>
#include <fmt/format.h> 

namespace Logger{
class VariadicLogHandle : public LogHandle {
public:
    using LogHandle::LogHandle;

    template<typename... Args>
    void Log(SourceLocation source, LogLevel level, fmt::format_string<Args...> fmt_str, Args&&... args){
        CreateAndSendLogMsg(source, level, fmt_str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Log(LogLevel level, fmt::format_string<Args...> fmt_str, Args&&... args){
        Log(SourceLocation{__FILE__, __LINE__, __func__}, level, fmt_str, std::forward<Args>(args)...);
    }


private:
    template<typename... Args>
    void CreateAndSendLogMsg(SourceLocation source, LogLevel level, fmt::basic_string_view<char> fmt_str, Args&&... args) {
        if(!ShouldLog(level)) {
            return;
        }
        std::string formatted = fmt::vformat(fmt_str, fmt::make_format_args(args...));

        LogMsg log_msg(source, level, StringView(formatted.data(), formatted.size()));

        LogHandle::SendLogMsg(log_msg);

    }


};
}