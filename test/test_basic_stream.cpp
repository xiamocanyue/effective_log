#include <iostream>

#include "../log_factory.h"
#include "../sinks/console_sink.h"
#include "../formatter/default_formatter.h"
#include "../log_variadic_handle.h"

int main() {
    auto sink = std::make_shared<Logger::ConsoleSink>();
    sink->SetFormatter(std::make_unique<Logger::DefaultFormatter>());

    auto handle = std::make_shared<Logger::VariadicLogHandle>(sink);
    
    Logger::LogFactory::GetInstance().SetLogHandle(handle);

    Logger::SourceLocationg source(__FILE__, __LINE__, __func__);

    Logger::LogFactory::GetInstance().GetLogHandle()->Log(source, Logger::LogLevel::kError, "hello my log {}", "你好");


    
}
