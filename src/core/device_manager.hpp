#pragma once

#if IGNITE_WITH_DX11 || IGNITE_WITH_DX12
#include <dxgi.h>
#endif

#if IGNITE_WITH_DX11
#include <d3d11.h>
#endif

#if IGNITE_WITH_DX12
#include <d3d12.h>
#endif

#if IGNITE_WITH_VULKAN
#include <vulkan/vulkan.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#include <nvrhi/nvrhi.h>

#include <optional>
#include <array>
#include <functional>
#include <memory>

#include "base.hpp"
#include "types.hpp"

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

#if IGNITE_WITH_VULKAN
    std::string vulkanLibraryName;
    std::vector<std::string> requiredVulkanInstanceExtensions;
    std::vector<std::string? requiredVulkanLayers;
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
    uint32_t backBufferWidth = 1080;
    uint32_t backBufferHeight = 640;
    uint32_t refreshRate = 0;
    uint32_t swapChainBufferCount = 3;
    nvrhi::Format swapChainFormat = nvrhi::Format::SRGBA8_UNORM;
    uint32_t swapChainSampleCount = 1;
    uint32_t swapChainSampleQuality = 0;
    uint32_t maxFramesInFligth = 2;
    bool enableNvrhiValidationLayer = false;
    bool vsyncEnable = false;
    bool enableRayTracingExtensions = false; // for vulkan
    bool enableComputeQueue = false;
    bool enableCopyQueue = false;

    // Index of the adapter (DX11, DX12) or physical device (Vk) on which to initialize the device.
    // Negative values mean automatic detection.
    // The order of indices matches that returned by DeviceManager::EnumerateAdapters.
    int adapterIndex = -1;

     // Set this to true if the application implements UI scaling for DPI explicitly instead of relying
    // on ImGUI's DisplayFramebufferScale. This produces crisp text and lines at any scale
    // but requires considerable changes to applications that rely on the old behavior:
    // all UI sizes and offsets need to be computed as multiples of some scaled parameter,
    // such as ImGui::GetFontSize(). Note that the ImGUI style is automatically reset and scaled in 
    // ImGui_Renderer::DisplayScaleChanged(...).
    //
    // See ImGUI FAQ for more info:
    //   https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-how-should-i-handle-dpi-in-my-application
    bool supportExplicitDisplayScaling = false;

    // Enables automatic resizing of the application window according to the DPI scaling of the monitor
    // that it is located on. When set to true and the app launches on a monitor with >100% scale, 
    // the initial window size will be larger than specified in 'backBufferWidth' and 'backBufferHeight' parameters.
    bool resizeWindowWithDisplayScale = false;

    nvrhi::IMessageCallback *messageCallback = nullptr;
    
#if IGNITE_WITH_DX11 || IGNITE_WITH_DX12
    DXGI_USAGE swapChainUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
#endif

#if IGNITE_WITH_VULKAN
    std::vector<std::string> requiredVulkanDeviceExtensions;
    std::vector<std::string> optionalVulkanDeviceExtensions;
    std::vector<size_t> ignoreVulkanValidationMessageLocations;
#endif
};

class IRenderPass;

struct AdapterInfo
{
    using UUID = std::array<uint8_t, 16>;
    using LUUID = std::array<uint8_t, 8>;

    std::string name;
    uint32_t vendorID = 0;
    uint32_t deviceID = 0;
    uint64_t dedicatedVideoMemory = 0;

    std::optional<UUID> uuid;
    std::optional<LUUID> luuid;
#if IGNITE_WITH_DX11 || IGNITE_WITH_DX12
    nvrhi::RefCountPtr<IDXGIAdapter> dxgiAdapter;
#elif IGNITE_WITH_VULKAN
    VkPhysicalDevice vkPhysicalDevice = nullptr;
#endif
};

class DeviceManager
{
public:
    static DeviceManager *Create(nvrhi::GraphicsAPI api);

    bool CreateHeadlessDevice(const DeviceCreationParameters &params);
    bool CreateWindowDeviceAndSwapChain(const DeviceCreationParameters &params, const char *windowTitle);

    // Initializes device independent objects (DXGI factor, Vulkan instance).
    // Calling CreateInstance() is required before EnumerateAdapters(), but optional if we don't use EnumerateAdapters().
    // Note: if you call CreateInstance before Create*Device*(), the values in InstanceParameters must match those
    // in DeviceCreationParameters passed to the device call.
    bool CreateInstance(const InstanceParameters &params);

    // Enumerates adapters or physical devices present in the system.
    // Note: a call to CreateInstance() or Create*Device*() is required before EnumerateAdapters().
    virtual bool EnumerateAdapters(std::vector<AdapterInfo> &outAdapters) = 0;

    void AddRenderPassToFront(IRenderPass *pController);
    void AddRenderPassToBack(IRenderPass *pController);
    void RemoveRenderPass(IRenderPass *pController);

    void RunMessageLoop();

    void GetWindowDimensions(int &width, int &height);

    void GetDPIScaleInfo(float &x, float &y) const
    {
        x = m_DPIScaleFactorX;
        y = m_DPIScaleFactorY;
    }
protected:
    // usefull for apps that require 2 frames worth of simulation data before first render
    // apps should extend the DeviceManager classes, and consturct initialized this to true to opt in to the behavior
    bool m_SkipRenderOnFirstFrame = false;
    bool m_WindowVisible = false;
    bool m_WindowIsInFocus = true;

    DeviceCreationParameters m_DeviceParams;
    GLFWwindow *m_Window = nullptr;
    bool m_EnableRenderDuringWindowMovement = false;
    std::list<IRenderPass *> m_vRenderPasses;
    
    // set to true if running on NVIDIA GPU
    bool m_IsNvidia = false;
    double m_PreviouseFrameTimestamp = 0.0f;
    
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

    uint32_t m_FrameIndex = 0;

    std::vector<nvrhi::FramebufferHandle> m_SwapChainFramebuffers;

    DeviceManager();

    void UpdateWindowSize();
    [[nodiscard]] bool ShouldRenderUnfocused() const;

    void BackBufferResizing();
    void BackBufferResized();
    void DisplayScaleChanged();

    void Animate(double elapsedTime);
    void Render();
    void UpdateAverageFrameTime(double elapsedTime);
    bool AnimateRenderPresent();

    // device specific methods
    virtual bool CreateInstanceInternal() = 0;
    virtual bool CreateDevice() = 0;
    virtual bool CreateSwapChain() = 0;
    virtual void DestroyDeviceAndSwapChain() = 0;
    virtual void ResizeSwapChain() = 0;
    virtual bool BeginFrame() = 0;
    virtual bool Present() = 0;

public:
    [[nodiscard]] virtual nvrhi::IDevice *GetDevice() const = 0;
    [[nodiscard]] virtual const char *GetRendererString() const = 0;
    [[nodiscard]] virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;

    const DeviceCreationParameters &GetDeviceParams();
    [[nodiscard]] double GetAverageFrameTimeSeconds() const { return m_AverageFrameTime; }
    [[nodiscard]] double GetPreviousFrameTimestamp() const { return m_PreviouseFrameTimestamp; }
    void SetFrameTimeUpdateInterval(double seconds) { m_AverageTimeUpdateInterval = seconds; }
    [[nodiscard]] bool IsVsyncEnabled() const { return m_DeviceParams.vsyncEnable; }
    virtual void ReportLiveObjects() { };
    void SetEnableRenderDuringWindowMovement(bool val) { m_EnableRenderDuringWindowMovement = val; }

    // these are public in order to be called from the GLFW callback functions
    void WindowCloseCallback() { }
    void WindowIconifyCallback(int iconify) {}
    void WindowFocusCallback(int focused) { }
    void WindowRefreshCallback() { }
    void WindowPosCallback(int xpos, int ypos);
    
    void KeyboardUpdate(int key, int scancode, int action, int mods);
    void KeyboardCharInput(unsigned int unicode, int mods);
    void MousePosUpdate(double xpos, double ypos);
    void MouseButtonUpdate(int button, int action, int mods);
    void MouseScrollUpdate(double xoffset, double yoffset);

    [[nodiscard]] GLFWwindow *GetWindow() const { return m_Window; }
    [[nodiscard]] uint32_t GetFrameIndex() const { return m_FrameIndex; }

    virtual nvrhi::ITexture *GetCurrentBackBuffer() = 0;
    virtual nvrhi::ITexture *GetBackBuffer(uint32_t index) = 0;
    virtual uint32_t GetCurrentBackBufferIndex() = 0;
    virtual uint32_t GetBackBufferCount() = 0;
    nvrhi::IFramebuffer *GetCurrentFramebuffer();
    nvrhi::IFramebuffer *GetFramebuffer(uint32_t index);

    virtual void Shutdown();
    virtual ~DeviceManager() = default;

    void SetWindowTitle(const char *title);
    void SetInformativeWindowTitle(const char *applicationName, bool includeFramerate = true, const char *extraInfo = nullptr);
    const char *GetWindowTitle();

    virtual bool IsVulkanInstanceExtensionEnabled(const char *extensionName) const { return false; }
    virtual bool IsVulkanDeviceExtensionEnabled(const char *extensionName) const { return false; }
    virtual bool IsVulkanLayerEnabled(const char *layerName) const { return false; }
    virtual void GetEnabledVulkanInstanceExtensions(std::vector<std::string> &extensions) const {}
    virtual void GetEnabledVulkanDeviceExtensions(std::vector<std::string> &extensions) const {}
    virtual void GetEnabledVulkanLayers(std::vector<std::string> &layers) const {}

    // GetFrameIndex cannot be used inside of these callbacks, hence the additional passing of frameID
    // Refer to AnimatedRenderPresent implementation for more details
    struct PipelineCallbacks
    {
        std::function<void(DeviceManager &, uint32_t)> beforeFrame = nullptr;
        std::function<void(DeviceManager &, uint32_t)> beforeAnimate = nullptr;
        std::function<void(DeviceManager &, uint32_t)> afterAnimate = nullptr;
        std::function<void(DeviceManager &, uint32_t)> beforeRender = nullptr;
        std::function<void(DeviceManager &, uint32_t)> afterRender = nullptr;
        std::function<void(DeviceManager &, uint32_t)> beforePresent = nullptr;
        std::function<void(DeviceManager &, uint32_t)> afterPresent = nullptr;
    } m_Callbacks;

private:
    static DeviceManager *CreateD3D11();
    static DeviceManager *CreateD3D12();
    static DeviceManager *CreateVK();

    std::string m_WindowTitle;
}; 

class IRenderPass
{
public:
    explicit IRenderPass(DeviceManager *deviceManager)
        : m_DeviceManager(deviceManager)
    {
    }

    virtual ~IRenderPass() = default;
    virtual void SetLateWarpOptions() { }
    virtual bool ShouldRenderUnfocused() { return false; }
    virtual void Render(nvrhi::IFramebuffer *framebuffer) { }
    virtual void Animate(float fElapsedTimeSeconds) { }
    virtual void BackBufferResizing() { }
    virtual void BackBufferResized(const uint32_t width, const uint32_t heigth, const uint32_t sampleCount) { }

    // Called before Animate() when a DPI change was created
    virtual void DisplayScaleChanged(float scaleX, float scaleY) { }

    virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) { return false; }
    virtual bool KeyboardCharInput(unsigned int unicode, int modes) { return false; }
    virtual bool MousePosUpdate(double xpos, double ypos) { return false; }
    virtual bool MouseScrollUpdate(double xoffset, double yoffset) { return false; }
    virtual bool MouseButtonUpdate(int button, int action, int mods) { return false; }
    virtual bool JoystickButtonUpdate(int button, bool pressed) { return false; }
    virtual bool JoystickAxisUpdate(int axis, float value) { return false; }

    [[nodiscard]] DeviceManager *GetDeviceManager() const { return m_DeviceManager; }
    [[nodiscard]] nvrhi::IDevice *GetDevice() const { return m_DeviceManager->GetDevice(); }
    [[nodiscard]] uint32_t GetFrameIndex() const { return m_DeviceManager->GetFrameIndex(); }
private:
    DeviceManager *m_DeviceManager;
};