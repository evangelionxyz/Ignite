#include "logger.hpp"

#include <memory>

struct LoggerImpl
{
    std::shared_ptr<spdlog::async_logger> logger;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> sink;
};

static LoggerImpl *impl = nullptr;

void Logger::Init()
{
    impl = new LoggerImpl();

    spdlog::init_thread_pool(8192, 1);
    impl->sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    impl->sink->set_pattern("%^[%T] %n: %v%$");

    impl->logger = std::make_shared<spdlog::async_logger>(
        "[IGNITE]",
        impl->sink,
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );
}

void Logger::Shutdown()
{
    if (impl)
    {
        impl->sink->flush();
        impl->logger->flush();
        delete impl;
    }
}

spdlog::async_logger *Logger::GetLogger()
{
    return impl ? impl->logger.get() : nullptr;
}
