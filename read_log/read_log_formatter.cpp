#include "read_log/read_log_formatter.h"
#include "formatter/default_formatter.h"





/**
 * @brief EffectiveMsg custom pattern
 *
 * %l log_level
 * %D timestamp date
 * %S timestamp seconds
 * %M timestamp milliseconds
 * %p process_id
 * %t thread_id
 * %# line
 * %F file_name
 * %f func_name
 * %v log_info
 *
 * such as: [%l][%D:%S][%p:%t][%F:%f:%#]%v
 */


class FlagFormatter {
public:
    virtual ~FlagFormatter() = default;
    virtual void Format(const EffectiveMsg& msg, std::string& buf) = 0;
};

std::vector<std::unique_ptr<FlagFormatter>> flag_formatters_;


std::string DefaultFormatLogMsg(EffectiveMsg& msg) {
    Logger::LogMsg log_msg(Logger::SourceLocation{msg.file_name(), msg.line(), msg.func_name()}, static_cast<Logger::LogLevel>(msg.level()), msg.log_info());
    Logger::DefaultFormatter defaultformat;
    Logger::MemoryBuf buf;
    defaultformat.Format(log_msg, &buf);
    return buf;
}


class NotNeedFormat : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(str_);
    }
    void AddStr(char c) {
        str_ += c;
    }

private:
    std::string str_;
};

char* GetLevelStr(int32_t level) {
    switch (level)
    {
    case 0:
        return "V";
    case 1:
        return "D";
    case 2:
        return "I";
    case 3:
        return "W";
    case 4:
        return "E";
    case 5:
        return "F";
    default:
        return "U";
    }
}

class LogLevelFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(GetLevelStr(msg.level()));
    }
};

class DayFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        auto time_day = std::chrono::system_clock::time_point(std::chrono::milliseconds(msg.timestamp()));
        std::time_t t = std::chrono::system_clock::to_time_t(time_day);
        char timebuf[64] = {0};
        auto size = std::strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", std::localtime(&t));
        buf.append(timebuf, size);
    }
};

class SecondFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(std::to_string(msg.timestamp() / 1000));
    }
};

class MillSecondFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(std::to_string(msg.timestamp()));
    }
};

class ProcessIdFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(std::to_string(msg.pid()));
    }
};

class ThreadIdFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(std::to_string(msg.tid()));
    }
};

class LineFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(std::to_string(msg.line()));
    }
};

class FileNameFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(msg.file_name());
    }
};

class FunceNameFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(msg.func_name());
    }
};

class LogInfoFormatter : public FlagFormatter {
public:
    void Format(const EffectiveMsg& msg, std::string& buf) override {
        buf.append(msg.log_info());
    }
};

void HandleFlag(char flag) {
    switch (flag)
    {
    case 'l':
        flag_formatters_.push_back(std::make_unique<LogLevelFormatter>());
        break;
    case 'D':
        flag_formatters_.push_back(std::make_unique<DayFormatter>());
        break;
    case 'S':
        flag_formatters_.push_back(std::make_unique<SecondFormatter>());
        break;
    case 'M':
        flag_formatters_.push_back(std::make_unique<MillSecondFormatter>());
        break; 
    case 'p':
        flag_formatters_.push_back(std::make_unique<ProcessIdFormatter>());
        break; 
    case 't':
        flag_formatters_.push_back(std::make_unique<ThreadIdFormatter>());
        break;
    case '#':
        flag_formatters_.push_back(std::make_unique<LineFormatter>());
        break;
    case 'F':
        flag_formatters_.push_back(std::make_unique<FileNameFormatter>());
        break;
    case 'f':
        flag_formatters_.push_back(std::make_unique<FunceNameFormatter>());
        break;
    case 'v':
        flag_formatters_.push_back(std::make_unique<LogInfoFormatter>());
        break;

    default:
        auto formatter = std::make_unique<NotNeedFormat>();
        formatter->AddStr('%');
        formatter->AddStr(flag);
        flag_formatters_.push_back(std::move(formatter));
        break;
    }
}

void ComplieStandard(std::string& standard) {
    std::unique_ptr<NotNeedFormat> user_chars;
    auto end = standard.end();
    flag_formatters_.clear();
    for (auto it = standard.begin(); it != end; it++) {
        if (*it == '%') {
            if (user_chars) {
                flag_formatters_.push_back(std::move(user_chars));
            }
            it++;
            if (it == end) {
                break;
            }
            HandleFlag(*it);
        } else {
            if (!user_chars) {
                user_chars = std::make_unique<NotNeedFormat>();
            }
            user_chars->AddStr(*it);
        }
    }
    if (user_chars) {
        flag_formatters_.push_back(std::move(user_chars));
    }

}

void ReadLogFormatter::SetStandard(std::string& standard) {
    ComplieStandard(standard);
}

void ReadLogFormatter::Format(EffectiveMsg& msg, std::string& buf) {
    if (!flag_formatters_.empty()) {
        for (auto& formatter : flag_formatters_) {
            formatter->Format(msg, buf);
        }
    } else {
        buf.append(DefaultFormatLogMsg(msg));
    }

    buf.append("\n");
}