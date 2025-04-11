#pragma once

#include "base.hpp"
#include "types.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace ignite
{
    class Logger
    {
    public:
        static void Init();
        static void Shutdown();
        static spdlog::async_logger* GetLogger();
    };
}

namespace fmt {
template<>
struct formatter<std::filesystem::path>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const std::filesystem::path &filepath, FormatContext &ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", filepath.generic_string());
    }
};
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
    return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
    return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
    return os << glm::to_string(quaternion);
}


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
