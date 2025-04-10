#include <cstdio>
#include <iomanip>
#include <thread>
#include <sstream>
#include <nvrhi/utils.h>

#ifdef _WIN32
#include <ShellScalingApi.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib") // Link to DWM API
#pragma comment(lib, "shcore.lib")
#endif
#include "device_manager.hpp"
#include "ignite/core/logger.hpp"
#include <glm/glm.hpp>

DefaultMessageCallback &DefaultMessageCallback::GetInstance()
{
    static DefaultMessageCallback *instance = nullptr;
    if (!instance)
        instance = new DefaultMessageCallback();
    return *instance;
}

void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char *messageText)
{
    switch (severity)
    {
        case nvrhi::MessageSeverity::Info:
        {
            LOG_INFO("NVHRI INFO: {}", messageText);
            break;
        }
        case nvrhi::MessageSeverity::Warning:
        {
            LOG_WARN("NVHRI WARN: {}", messageText);
            break;
        }
        case nvrhi::MessageSeverity::Error:
        {
            LOG_ERROR("NVHRI WARN: {}", messageText);
            break;
        }
        case nvrhi::MessageSeverity::Fatal:
        {
            LOG_ASSERT(false, "NVHRI FATAL: {}", messageText);
            break;
        }
    }
}

bool DeviceManager::CreateInstance(const InstanceParameters &params)
{
    if (m_InstanceCreated)
        return true;

    static_cast<InstanceParameters &>(m_DeviceParams) = params;
    if (!params.headlessDevice)
    {
#ifdef PLATFORM_WINDOWS
        if (!params.enablePerMonitorDPI)
            SetProcessDpiAwareness(PROCESS_DPI_UNAWARE);
#endif
    }

    if (!glfwInit())
    {
        return false;
    }

    return CreateInstanceInternal();
}

bool DeviceManager::IsUpdateDPIScaleFactor()
{
    if (m_PrevDPIScaleFactorX != m_DPIScaleFactorX || m_PrevDPIScaleFactorY != m_DPIScaleFactorY)
    {
        m_PrevDPIScaleFactorX = m_DPIScaleFactorX;
        m_PrevDPIScaleFactorY = m_DPIScaleFactorY;
        return true;
    }
    return false;
}

void DeviceManager::GetDPIScaleInfo(float &x, float &y) const
{
    x = m_DPIScaleFactorX;
    y = m_DPIScaleFactorY;
}

void DeviceManager::CreateBackBuffers()
{
    u32 backBufferCount = GetBackBufferCount();
    m_SwapChainFramebuffers.resize(backBufferCount);
    for (u32 index = 0; index < backBufferCount; ++index)
    {
        m_SwapChainFramebuffers[index] = GetDevice()->createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(GetBackBuffer(index)));
    }
}

const DeviceCreationParameters &DeviceManager::GetDeviceParams()
{
    return m_DeviceParams;
}

DeviceManager::DeviceManager()
{
}

void DeviceManager::Destroy()
{
    m_SwapChainFramebuffers.clear();
    DestroyDeviceAndSwapChain();
    m_InstanceCreated = false;
}

nvrhi::IFramebuffer* DeviceManager::GetCurrentFramebuffer()
{
    return GetFramebuffer(GetCurrentBackBufferIndex());
}

nvrhi::IFramebuffer* DeviceManager::GetFramebuffer(u32 index)
{
    if (index < m_SwapChainFramebuffers.size())
        return m_SwapChainFramebuffers[index];

    LOG_ASSERT(false, "SwapChain framebuffer is empty");
    return nullptr;
}

DeviceManager* DeviceManager::Create(nvrhi::GraphicsAPI api)
{
    switch (api)
    {
#if IGNITE_WITH_DX11
    case nvrhi::GraphicsAPI::D3D11:
        return CreateD3D11();
#elif IGNITE_WITH_DX12
    case nvrhi::GraphicsAPI::D3D12:
        return CreateD3D12();
#elif IGNITE_WITH_VULKAN
    case nvrhi::GraphicsAPI::VULKAN:
        return CreateVK();
#endif
    default:
        LOG_ASSERT(false, "Unsupported Graphics API {}", (u32)api);
        return nullptr;
    }
}
