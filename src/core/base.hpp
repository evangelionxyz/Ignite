#pragma once

#ifdef _WIN32
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS
    #endif
#elif __linux__ || __GNUG__
    #ifndef PLATFORM_LINUX
        #define PLATFORM_LINUX
    #endif
#endif

#ifdef SHARED_BUILD
    #ifdef _WIN32
        #define IGNITE_API __declspec(dllexport)
    #else
        #define IGNITE_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define IGNITE_API __declspec(dllimport)
    #else
        #define IGNITE_API
    #endif
#endif

#ifdef DEBUG_BUILD
    #define ENABLE_ASSERTS
    #ifdef _WIN32
        #define DEBUGBREAK() __debugbreak()
    #elif __linux__
        #define DEBUGBREAK() __builtin_trap();
    #endif
#else
#define DEBUGBREAK()
#endif

#ifndef RELEASE_BUILD
    #define ENABLE_VERIFY
#endif

#define EXPAND_MACRO(x)
#define STRINGIFY_MACRO(x) #x

#define BIT(x)(1 << x)
#define BIND_EVENT_FN(fn) [](auto&&... args) -> decltype(auto) { return fn(std::forward<decltype(args)>(args)...); }
#define BIND_CLASS_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
