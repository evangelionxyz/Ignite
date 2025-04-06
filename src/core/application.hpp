#pragma once

#include "device_manager.hpp"
#include "types.hpp"
#include "logger.hpp"

#include <filesystem>

struct ApplicationCreateInfo
{
    std::string appName;
    nvrhi::GraphicsAPI graphicsApi;
};

class Application
{
public:
    Application(const ApplicationCreateInfo &createInfo);
    virtual ~Application() = default;

    virtual void Run() {};
    std::string GetAppName() { return m_AppName; }
    const ApplicationCreateInfo &GetCreateInfo() { return m_CreateInfo; }
    
protected:
    std::string m_AppName;
    ApplicationCreateInfo m_CreateInfo;
};
