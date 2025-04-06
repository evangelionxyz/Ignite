#pragma once

#include "core/application.hpp"
#include "core/logger.hpp"

namespace ignite
{
inline int Main(const int argc, char **argv)
{
    Logger::Init();

    Application *app = CreateApplication({argc, argv});
    app->Run();
    delete app;

    Logger::Shutdown();

    return 0;
}

}

int main(const int argc, char **argv)
{
#ifdef PLATFORM_WINDOWS
    #ifndef SHIPPING_BUILD
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
    #endif
#endif

    int ret = ignite::Main(argc, argv);

#ifdef PLATFORM_WINDOWS
    #ifndef SHIPPING_BUILD
        FreeConsole();
    #endif
#endif

    return ret;
}