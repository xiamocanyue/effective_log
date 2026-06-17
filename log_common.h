#pragma once 

#include <fmt/format.h>
#include <string_view>

#define LOGGER_LEVER_TRACE 0
#define LOGGER_LEVER_DEBUG 1
#define LOGGER_LEVER_INFO 2
#define LOGGER_LEVER_WARN 3
#define LOGGER_LEVER_ERROR 4
#define LOGGER_LEVER_CRITICAL 5
#define LOGGER_LEVER_OFF 6

#define LOGGER_ACTIVE_LEVER LOGGER_LEVER_TRACE


namespace Logger {

using StringView = std::string_view;
using MemoryBuf = std::string;


enum class LogLevel {
    kTrace = LOGGER_LEVER_TRACE,
    kDebug = LOGGER_LEVER_DEBUG,
    kInfo = LOGGER_LEVER_INFO,
    kWarn = LOGGER_LEVER_WARN,
    kError = LOGGER_LEVER_ERROR,
    kCritical = LOGGER_LEVER_CRITICAL,
    kOff = LOGGER_LEVER_OFF
};


class SourceLocation {
public: 
    SourceLocation() = default;
    SourceLocation(StringView file_name_in, int32_t line_in, StringView func_name_in) 
            : file_name_(file_name_in), line_(line_in), func_name_(func_name_in) {
        
        if (!file_name_.empty()) {
            size_t index = file_name_.rfind('/');
            if (index != StringView::npos) {
                file_name_ = file_name_.substr(index + 1);
            } else {
                index = file_name_.rfind("//");
                if (index != StringView::npos) {
                    file_name_ = file_name_.substr(index + 1);
                }
            }
        }

    } 

    StringView file_name_;
    int32_t line_;
    StringView func_name_;
};


    
} // namespace Logger
