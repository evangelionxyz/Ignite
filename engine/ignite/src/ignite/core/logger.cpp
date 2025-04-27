#include "logger.hpp"

#include <memory>

struct LoggerImpl
{
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> sink;
};

static LoggerImpl *impl = nullptr;

namespace ignite
{
    void Logger::Init()
    {
        impl = new LoggerImpl();

        spdlog::init_thread_pool(8192, 1);
        impl->sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        impl->sink->set_pattern("%^[%T] %n: %v%$");

        impl->logger = std::make_shared<spdlog::logger>(
            "[IGNITE]",
            impl->sink
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

    spdlog::logger *Logger::GetLogger()
    {
        return impl ? impl->logger.get() : nullptr;
    }
}
