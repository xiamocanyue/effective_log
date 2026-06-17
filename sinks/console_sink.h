#pragma once

#include "sinks/sink.h"

namespace Logger{

class ConsoleSink : public LogSink{
public:
    void Log(LogMsg& msg) override;
    void SetFormatter(std::unique_ptr<Formatter> formatter) override;

private:
    std::unique_ptr<Formatter> formatter_;

};  

}
