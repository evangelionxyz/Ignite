#pragma once

#include "ignite/core/base.hpp"

#ifdef PLATFORM_WINDOWS
    #include <dxgi.h>
#endif

#ifdef IGNITE_WITH_DX12
    #include <d3d12.h>
#endif

#if IGNITE_WITH_VULKAN
    #define VK_NO_PROTOTYPES
    #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
    #include <vulkan/vulkan.hpp>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <nvrhi/nvrhi.h>
#include <optional>
#include <array>
#include <functional>

#include "ignite/core/base.hpp"
#include "ignite/core/types.hpp"

namespace ignite
{
    struct DefaultMessageCallback final : public nvrhi::IMessageCallback
    {
        static DefaultMessageCallback &GetInstance();
        void message(nvrhi::MessageSeverity severity, const char *messageText) override;
    };

    struct InstanceParameters
    {
        bool enableDebugRuntime = false;
        bool enableWarningAsErrors = false;
        bool enableGPUValidation = false;
        bool headlessDevice = false;
        bool logBufferLifetime = false;
        bool enableHeapDirectlyIndexed = false;
        bool enablePerMonitorDPI = false;

#ifdef IGNITE_WITH_VULKAN
        std::string vulkanLibraryName;
        std::vector<std::string> requiredVulkanInstanceExtensions;
        std::vector<std::string> requiredVulkanLayers;
        std::vector<std::string> optionalVulaknInstanceExtensions;
        std::vector<std::string> optionalVulkanLayers;
#endif

    };

    struct DeviceCreationParameters : public InstanceParameters
    {
        bool startMaximized = false;
        bool startFullscreen = false;
        bool startBorderless = false;
        bool allowModeSwitch = false;
        int windowPosX = -1; // -1 means use default placement
        int windowPosY = -1;
        u32 backBufferWidth = 1080;
        u32 backBufferHeight = 640;
        u32 refreshRate = 0;
        u32 swapChainBufferCount = 3;
        nvrhi::Format swapChainFormat = nvrhi::Format::SRGBA8_UNORM;
        u32 swapChainSampleCount = 1;
        u32 swapChainSampleQuality = 0;
        u32 maxFramesInFlight = 2;
        bool enableNvrhiValidationLayer = false;
        bool vsyncEnable = false;
        bool enableRayTracingExtensions = false; // for vulkan
        bool enableComputeQueue = false;
        bool enableCopyQueue = false;
        int adapterIndex = -1;
        bool supportExplicitDisplayScaling = false;
        bool resizeWindowWithDisplayScale = false;
        nvrhi::IMessageCallback *messageCallback = nullptr;

#ifdef PLATFORM_WINDOWS
        DXGI_USAGE swapChainUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
#endif

#ifdef IGNITE_WITH_VULKAN
        std::vector<std::string> requiredVulkanDeviceExtensions;
        std::vector<std::string> optionalVulkanDeviceExtensions;
        std::vector<size_t> ignoreVulkanValidationMessageLocations;
        std::function<void(VkDeviceCreateInfo &)> deviceCreateInfoCallback;
        void *physicalDeviceFeatures2Extensions = nullptr;
#endif
    };

    struct AdapterInfo
    {
        std::string name;
        u32 vendorID = 0;
        u32 deviceID = 0;
        uint64_t dedicatedVideoMemory = 0;

        std::optional<std::array<uint8_t, 16>> uuid;
        std::optional<std::array<uint8_t, 8>> luid;


#ifdef IGNITE_WITH_DX12
        nvrhi::RefCountPtr<IDXGIAdapter> dxgiAdapter;
#endif

#ifdef IGNITE_WITH_VULKAN
        VkPhysicalDevice vkPhysicalDevice = nullptr;
#endif
    };

    class DeviceManager
    {
    public:
        static DeviceManager *Create(nvrhi::GraphicsAPI api);

        bool CreateInstance(const InstanceParameters &params);

        virtual bool EnumerateAdapters(std::vector<AdapterInfo> &outAdapters) = 0;
        virtual void WaitForIdle() = 0;

        bool IsUpdateDPIScaleFactor();
        void GetDPIScaleInfo(float &x, float &y) const;

    public:
        // device specific methods
        virtual bool CreateInstanceInternal() = 0;
        virtual bool CreateDevice() = 0;
        virtual bool CreateSwapChain() = 0;
        virtual void DestroyDeviceAndSwapChain() = 0;
        virtual void ResizeSwapChain() = 0;
        virtual bool BeginFrame() = 0;
        virtual bool Present() = 0;

        [[nodiscard]] virtual nvrhi::IDevice *GetDevice() const = 0;
        [[nodiscard]] virtual const char *GetRendererString() const = 0;
        [[nodiscard]] virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;

        const DeviceCreationParameters &GetDeviceParams();
        [[nodiscard]] double GetAverageFrameTimeSeconds() const { return m_AverageFrameTime; }
        [[nodiscard]] double GetPreviousFrameTimestamp() const { return m_PreviousFrameTimestamp; }
        void SetFrameTimeUpdateInterval(double seconds) { m_AverageTimeUpdateInterval = seconds; }
        [[nodiscard]] bool IsVsyncEnabled() const { return m_DeviceParams.vsyncEnable; }
        virtual void ReportLiveObjects() { };
        void SetEnableRenderDuringWindowMovement(bool val) { m_EnableRenderDuringWindowMovement = val; }

        [[nodiscard]] GLFWwindow *GetWindow() const { return m_Window; }
        [[nodiscard]] u32 GetFrameIndex() const { return m_FrameIndex; }

        virtual nvrhi::ITexture *GetCurrentBackBuffer() = 0;
        virtual nvrhi::ITexture *GetBackBuffer(u32 index) = 0;
        virtual u32 GetCurrentBackBufferIndex() = 0;
        virtual u32 GetBackBufferCount() = 0;
        nvrhi::IFramebuffer *GetCurrentFramebuffer();
        nvrhi::IFramebuffer *GetFramebuffer(u32 index);

        virtual void Destroy();
        virtual ~DeviceManager() = default;

        virtual bool IsVulkanInstanceExtensionEnabled(const char *extensionName) const { return false; }
        virtual bool IsVulkanDeviceExtensionEnabled(const char *extensionName) const { return false; }
        virtual bool IsVulkanLayerEnabled(const char *layerName) const { return false; }
        virtual void GetEnabledVulkanInstanceExtensions(std::vector<std::string> &extensions) const {}
        virtual void GetEnabledVulkanDeviceExtensions(std::vector<std::string> &extensions) const {}
        virtual void GetEnabledVulkanLayers(std::vector<std::string> &layers) const {}

    protected:
        DeviceManager();

        void CreateBackBuffers();

        bool m_SkipRenderOnFirstFrame = false;
        bool m_WindowVisible = false;
        bool m_WindowIsInFocus = true;

        DeviceCreationParameters m_DeviceParams;
        GLFWwindow *m_Window = nullptr;
        bool m_EnableRenderDuringWindowMovement = false;

        bool m_IsNvidia = false;
        double m_PreviousFrameTimestamp = 0.0f;

        // current DPI scale info (update when window moves)
        float m_DPIScaleFactorX = 1.0f;
        float m_DPIScaleFactorY = 1.0f;
        float m_PrevDPIScaleFactorX = 1.0f;
        float m_PrevDPIScaleFactorY = 1.0f;

        bool m_RequestedVSync = false;
        bool m_InstanceCreated = false;

        double m_AverageFrameTime = 0.0f;
        double m_AverageTimeUpdateInterval = 0.5;
        double m_FrameTimeSum = 0.0;
        int m_NumberOfAccumulatedFrames = 0;

        u32 m_FrameIndex = 0;

        std::vector<nvrhi::FramebufferHandle> m_SwapChainFramebuffers;

        friend class Window;

    private:
        static DeviceManager *CreateD3D12();
        static DeviceManager *CreateVK();

        std::string m_WindowTitle;
    };
}