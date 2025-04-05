#pragma once

#include "base.hpp"
#include "types.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger
{
public:
    static void Init();
    static void Shutdown();
    static spdlog::async_logger* GetLogger();
};

#define LOG_ERROR(...) Logger::GetLogger()->error(__VA_ARGS__)
#define LOG_INFO(...) Logger::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) Logger::GetLogger()->warn(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::GetLogger()->debug(__VA_ARGS__)
#define LOG_TRACE(...) Logger::GetLogger()->trace(__VA_ARGS__)

#ifdef _DEBUG
#define LOG_ASSERT(check, ...)\
    if (!(check)) {\
        LOG_ERROR(__VA_ARGS__);\
        DEBUGBREAK();\
    }
#else
#define LOG_ASSERT(...)
#endif
