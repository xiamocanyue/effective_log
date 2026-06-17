#pragma once 

#include "log_common.h"
#include "log_factory.h"
#include "log_handle.h"

#define EXT_LOGGER_INIT(handle) Logger::LogFactory::GetInstance().SetLogHandle(handle)

#define LOGGER_CALL(handle, level, ...)  if (handle) {handle->Log(Logger::SourceLocationg{__FILE__, __LINE__, __func__}, level, __VA_ARGS__);}


#if LOGGER_ACTIVE_LEVER <= LOGGER_LEVER_TRACE
#define LOG_LOGGER_TRACE(handle, ...) LOGGER_CALL(handle, Logger::LogLevel::kTrace, __VA_ARGS__)
#define EXT_LOG_TRACE(...) LOG_LOGGER_TRACE(Logger::LogFactory::GetInstance().GetLogHandle(), __VA_ARGS__)
#else 
#define LOG_LOGGER_TRACE(handle, ...) (void)0
#define EXT_LOG_TRACE(...) (void)0
#endif

#if LOGGER_ACTIVE_LEVER <= LOGGER_LEVER_TRACE
#define LOG_LOGGER_DEBUG(handle, ...) LOGGER_CALL(handle, Logger::LogLevel::kDebug, __VA_ARGS__)
#define EXT_LOG_DEBUG(...) LOG_LOGGER_DEBUG(Logger::LogFactory::GetInstance().GetLogHandle(), __VA_ARGS__)
#else 
#define LOG_LOGGER_DEBUG(handle, ...) (void)0
#define EXT_LOG_DEBUG(...) (void)0
#endif

#if LOGGER_ACTIVE_LEVER <= LOGGER_LEVER_TRACE
#define LOG_LOGGER_INFO(handle, ...) LOGGER_CALL(handle, Logger::LogLevel::kInfo, __VA_ARGS__)
#define EXT_LOG_INFO(...) LOG_LOGGER_INFO(Logger::LogFactory::GetInstance().GetLogHandle(), __VA_ARGS__)
#else 
#define LOG_LOGGER_INFO(handle, ...) (void)0
#define EXT_LOG_INFO(...) (void)0
#endif

#if LOGGER_ACTIVE_LEVER <= LOGGER_LEVER_TRACE
#define LOG_LOGGER_WARN(handle, ...) LOGGER_CALL(handle, Logger::LogLevel::kWarn, __VA_ARGS__)
#define EXT_LOG_WARN(...) LOG_LOGGER_WARN(Logger::LogFactory::GetInstance().GetLogHandle(), __VA_ARGS__)
#else 
#define LOG_LOGGER_WARN(handle, ...) (void)0
#define EXT_LOG_WARN(...) (void)0
#endif

#if LOGGER_ACTIVE_LEVER <= LOGGER_LEVER_TRACE
#define LOG_LOGGER_ERROR(handle, ...) LOGGER_CALL(handle, Logger::LogLevel::kError, __VA_ARGS__)
#define EXT_LOG_ERROR(...) LOG_LOGGER_ERROR(Logger::LogFactory::GetInstance().GetLogHandle(), __VA_ARGS__)
#else 
#define LOG_LOGGER_ERROR(handle, ...) (void)0
#define EXT_LOG_ERROR(...) (void)0
#endif

#if LOGGER_ACTIVE_LEVER <= LOGGER_LEVER_TRACE
#define LOG_LOGGER_CRITICAL(handle, ...) LOGGER_CALL(handle, Logger::LogLevel::kCritical, __VA_ARGS__)
#define EXT_LOG_CRITICAL(...) LOG_LOGGER_CRITICAL(Logger::LogFactory::GetInstance().GetLogHandle(), __VA_ARGS__)
#else 
#define LOG_LOGGER_CRITICAL(handle, ...) (void)0
#define EXT_LOG_CRITICAL(...) (void)0
#endif

