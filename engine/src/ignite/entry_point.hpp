#pragma once

#include "core/application.hpp"
#include "core/logger.hpp"

inline int Main(const int argc, char **argv)
{
    ignite::Logger::Init();
    ignite::Application *app = ignite::CreateApplication({argc, argv});
    app->Run();
    delete app;
    ignite::Logger::Shutdown();
    return 0;
}

int main(const int argc, char **argv)
{
    return Main(argc, argv);
}