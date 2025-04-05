#include <cstdio>
#include <iomanip>
#include <thread>
#include <sstream>
#include <nvrhi/utils.h>

#ifdef PLATFORM_WINDOWS
#include <ShellScalingApi.h>
#pragma comment(lib, "shcore.lib")
#endif

#include "device_manager.hpp"

#include "logger.hpp"

#include <glm/glm.hpp>

class JoyStickManager
{
public:
    static JoyStickManager &GetInstance()
    {
        static JoyStickManager instance;
        return instance;
    }

    void UpdateAllJoysticks(const std::list<IRenderPass *> &renderePasses);
    void EraseDisconnectedJoysticks();
    void EnumerateJoysticks();
    void ConnectJoystick(int id);
    void DisconnectJoystick(int id);

private:
    JoyStickManager() { }
    void UpdateJoystick(int j, const std::list<IRenderPass *> &renderPasses);

    std::list<int> m_JoystickIDs, m_RemovedJoysticks;
};

static void GLFW_ErrorCallback(int error, const char *description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
    exit(1);
}

static void GLFW_WindowIconifyCallback(GLFWwindow *window, int iconified)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->WindowIconifyCallback(iconified);
}

static void GLFW_WindowFocusCallback(GLFWwindow *window, int focused)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->WindowFocusCallback(focused);
}

static void GLFW_WindowRefreshCallback(GLFWwindow *window)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->WindowRefreshCallback();
}

static void GLFW_WindowCloseCallback(GLFWwindow *window)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->WindowCloseCallback();
}

static void GLFW_WindowPosCallback(GLFWwindow *window, int xpos, int ypos)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->WindowPosCallback(xpos, ypos);
}

static void GLFW_KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->KeyboardUpdate(key, scancode, action, mods);
}

static void GLFW_CharModsCallback(GLFWwindow *window, unsigned int unicode, int mods)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->KeyboardCharInput(unicode, mods);
}

static void GLFW_MousePosCallback(GLFWwindow *window, double xpos, double ypos)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->MousePosUpdate(xpos, ypos);
}

static void GLFW_MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->MouseButtonUpdate(button, action, mods);
}

static void GLFW_MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    DeviceManager *manager = reinterpret_cast<DeviceManager *>(glfwGetWindowUserPointer(window));
    manager->MouseScrollUpdate(xoffset, yoffset);
}

static void GLFW_JoystickConnectionCallback(int id, int connectStatus)
{
    if (connectStatus == GLFW_CONNECTED)
        JoyStickManager::GetInstance().ConnectJoystick(id);
    else if (connectStatus == GLFW_DISCONNECTED)
        JoyStickManager::GetInstance().DisconnectJoystick(id);
}

static const struct 
{
    nvrhi::Format format;
    u32 redBits;
    u32 greenBits;
    u32 blueBits;
    u32 alphaBits;
    u32 depthBits;
    u32 stencilBits;
} formatInfo[] = 
{
    { nvrhi::Format::UNKNOWN,            0,  0,  0,  0,  0,  0, },
    { nvrhi::Format::R8_UINT,            8,  0,  0,  0,  0,  0, },
    { nvrhi::Format::RG8_UINT,           8,  8,  0,  0,  0,  0, },
    { nvrhi::Format::RG8_UNORM,          8,  8,  0,  0,  0,  0, },
    { nvrhi::Format::R16_UINT,          16,  0,  0,  0,  0,  0, },
    { nvrhi::Format::R16_UNORM,         16,  0,  0,  0,  0,  0, },
    { nvrhi::Format::R16_FLOAT,         16,  0,  0,  0,  0,  0, },
    { nvrhi::Format::RGBA8_UNORM,        8,  8,  8,  8,  0,  0, },
    { nvrhi::Format::RGBA8_SNORM,        8,  8,  8,  8,  0,  0, },
    { nvrhi::Format::BGRA8_UNORM,        8,  8,  8,  8,  0,  0, },
    { nvrhi::Format::SRGBA8_UNORM,       8,  8,  8,  8,  0,  0, },
    { nvrhi::Format::SBGRA8_UNORM,       8,  8,  8,  8,  0,  0, },
    { nvrhi::Format::R10G10B10A2_UNORM, 10, 10, 10,  2,  0,  0, },
    { nvrhi::Format::R11G11B10_FLOAT,   11, 11, 10,  0,  0,  0, },
    { nvrhi::Format::RG16_UINT,         16, 16,  0,  0,  0,  0, },
    { nvrhi::Format::RG16_FLOAT,        16, 16,  0,  0,  0,  0, },
    { nvrhi::Format::R32_UINT,          32,  0,  0,  0,  0,  0, },
    { nvrhi::Format::R32_FLOAT,         32,  0,  0,  0,  0,  0, },
    { nvrhi::Format::RGBA16_FLOAT,      16, 16, 16, 16,  0,  0, },
    { nvrhi::Format::RGBA16_UNORM,      16, 16, 16, 16,  0,  0, },
    { nvrhi::Format::RGBA16_SNORM,      16, 16, 16, 16,  0,  0, },
    { nvrhi::Format::RG32_UINT,         32, 32,  0,  0,  0,  0, },
    { nvrhi::Format::RG32_FLOAT,        32, 32,  0,  0,  0,  0, },
    { nvrhi::Format::RGB32_UINT,        32, 32, 32,  0,  0,  0, },
    { nvrhi::Format::RGB32_FLOAT,       32, 32, 32,  0,  0,  0, },
    { nvrhi::Format::RGBA32_UINT,       32, 32, 32, 32,  0,  0, },
    { nvrhi::Format::RGBA32_FLOAT,      32, 32, 32, 32,  0,  0, }
};

DefaultMessageCallback &DefaultMessageCallback::GetInstance()
{
    static DefaultMessageCallback *instance = nullptr;
    if (!instance)
        instance = new DefaultMessageCallback();
    return *instance;
}

void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char *messageText)
{
    LOG_WARN("{}", messageText);
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
        {
            SetProcessDpiAwareness(PROCESS_DPI_UNAWARE);
        }
#endif
    }

    if (!glfwInit())
        return false;

    m_InstanceCreated = CreateInstanceInternal();
    return m_InstanceCreated;
}

bool DeviceManager::CreateHeadlessDevice(const DeviceCreationParameters &params)
{
    m_DeviceParams = params;
    m_DeviceParams.headlessDevice = true;

    if (!CreateInstance(m_DeviceParams))
        return false;

    return CreateDevice();
}

bool DeviceManager::CreateWindowDeviceAndSwapChain(const DeviceCreationParameters &params, const char *windowTitle)
{
    m_DeviceParams = params;
    m_DeviceParams.headlessDevice = false;
    m_RequestedVSync = params.vsyncEnable;

    if (!CreateInstance(m_DeviceParams))
        return false;

    glfwSetErrorCallback(GLFW_ErrorCallback);

    glfwDefaultWindowHints();

    bool foundFormat = false;
    for (const auto &info : formatInfo)
    {
        if (info.format == params.swapChainFormat)
        {
            glfwWindowHint(GLFW_RED_BITS, info.redBits);
            glfwWindowHint(GLFW_GREEN_BITS, info.greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, info.blueBits);
            glfwWindowHint(GLFW_ALPHA_BITS, info.alphaBits);
            glfwWindowHint(GLFW_DEPTH_BITS, info.depthBits);
            glfwWindowHint(GLFW_STENCIL_BITS, info.stencilBits);
            foundFormat = true;
            break;
        }
    }
    
    LOG_ASSERT(foundFormat, "GLFW format not found\n");

    glfwWindowHint(GLFW_SAMPLES, params.swapChainSampleCount);
    glfwWindowHint(GLFW_REFRESH_RATE, params.refreshRate);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, params.resizeWindowWithDisplayScale);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // ignored for full screen

    if (params.startBorderless)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // borderless window
    }

    m_Window = glfwCreateWindow(params.backBufferWidth, params.backBufferHeight,
        windowTitle ? windowTitle : "",
        params.startFullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr);

    LOG_ASSERT(m_Window, "Failed to create GLFW window\n");

    if (params.startFullscreen)
    {
        glfwSetWindowMonitor(m_Window, glfwGetPrimaryMonitor(),
            0, 0,
            m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight,
            m_DeviceParams.refreshRate);
    }
    else
    {
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
        m_DeviceParams.backBufferWidth = fbWidth;
        m_DeviceParams.backBufferHeight = fbHeight;
    }

    if (windowTitle)
        m_WindowTitle = windowTitle;

    glfwSetWindowUserPointer(m_Window, this);

    if (params.windowPosX != -1 && params.windowPosY != -1)
    {
        glfwSetWindowPos(m_Window, params.windowPosX, params.windowPosY);
    }

    glfwSetWindowPosCallback(m_Window, GLFW_WindowPosCallback);
    glfwSetWindowCloseCallback(m_Window, GLFW_WindowCloseCallback);
    glfwSetWindowRefreshCallback(m_Window, GLFW_WindowRefreshCallback);
    glfwSetWindowFocusCallback(m_Window, GLFW_WindowFocusCallback);
    glfwSetWindowIconifyCallback(m_Window, GLFW_WindowIconifyCallback);
    glfwSetKeyCallback(m_Window, GLFW_KeyCallback);
    glfwSetCharModsCallback(m_Window, GLFW_CharModsCallback);
    glfwSetCursorPosCallback(m_Window, GLFW_MousePosCallback);
    glfwSetMouseButtonCallback(m_Window, GLFW_MouseButtonCallback);
    glfwSetScrollCallback(m_Window, GLFW_MouseScrollCallback);
    glfwSetJoystickCallback(GLFW_JoystickConnectionCallback);


    JoyStickManager::GetInstance().EnumerateJoysticks();

    if (!CreateDevice())
        return false;
    
    if (!CreateSwapChain())
        return false;

    glfwShowWindow(m_Window);

    if (m_DeviceParams.startMaximized)
        glfwMaximizeWindow(m_Window);
    
    // reset the back buffer size state to enforce a resize event
    m_DeviceParams.backBufferWidth = 0;
    m_DeviceParams.backBufferHeight = 0;

    return true;
}

void DeviceManager::AddRenderPassToFront(IRenderPass *pRenderPass)
{
    m_vRenderPasses.remove(pRenderPass);
    m_vRenderPasses.push_front(pRenderPass);

    pRenderPass->BackBufferResizing();
    pRenderPass->BackBufferResized(
        m_DeviceParams.backBufferWidth,
        m_DeviceParams.backBufferHeight,
        m_DeviceParams.swapChainSampleCount
    );
}

void DeviceManager::AddRenderPassToBack(IRenderPass *pRenderPass)
{
    m_vRenderPasses.remove(pRenderPass);
    m_vRenderPasses.push_back(pRenderPass);

    pRenderPass->BackBufferResizing();
    pRenderPass->BackBufferResized(
        m_DeviceParams.backBufferWidth,
        m_DeviceParams.backBufferHeight,
        m_DeviceParams.swapChainSampleCount
    );
}

void DeviceManager::RemoveRenderPass(IRenderPass *pRenderPass)
{
    m_vRenderPasses.remove(pRenderPass);
}

void DeviceManager::BackBufferResizing()
{
    m_SwapChainFramebuffers.clear();
    for (auto &it : m_vRenderPasses)
    {
        it->BackBufferResizing();
    }
}

void DeviceManager::BackBufferResized()
{
    for (auto it : m_vRenderPasses)
    {
        it->BackBufferResized(m_DeviceParams.backBufferWidth,
            m_DeviceParams.backBufferHeight,
            m_DeviceParams.swapChainSampleCount
        );
    }

    u32 backBufferCount = GetBackBufferCount();
    m_SwapChainFramebuffers.resize(backBufferCount);

    for (u32 index = 0; index < backBufferCount; ++index)
    {
        m_SwapChainFramebuffers[index] = GetDevice()->createFramebuffer(
            nvrhi::FramebufferDesc().addColorAttachment(GetBackBuffer(index))
        );
    }
}

void DeviceManager::DisplayScaleChanged()
{
    for(auto it : m_vRenderPasses)
    {
        it->DisplayScaleChanged(m_DPIScaleFactorX, m_DPIScaleFactorY);
    }
}

void DeviceManager::Animate(double elapsedTime)
{
    for(auto it : m_vRenderPasses)
    {
        it->Animate(float(elapsedTime));
        it->SetLateWarpOptions();
    }
}


void DeviceManager::Render()
{    
    nvrhi::IFramebuffer* framebuffer = m_SwapChainFramebuffers[GetCurrentBackBufferIndex()];

    for (auto it : m_vRenderPasses)
    {
        it->Render(framebuffer);
    }
}

void DeviceManager::UpdateAverageFrameTime(double elapsedTime)
{
    m_FrameTimeSum += elapsedTime;
    m_NumberOfAccumulatedFrames += 1;
    
    if (m_FrameTimeSum > m_AverageTimeUpdateInterval && m_NumberOfAccumulatedFrames > 0)
    {
        m_AverageFrameTime = m_FrameTimeSum / double(m_NumberOfAccumulatedFrames);
        m_NumberOfAccumulatedFrames = 0;
        m_FrameTimeSum = 0.0;
    }
}

bool DeviceManager::ShouldRenderUnfocused() const
{
    for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
    {
        bool ret = (*it)->ShouldRenderUnfocused();
        if (ret)
            return true;
    }

    return false;
}

void DeviceManager::RunMessageLoop()
{
    m_PreviouseFrameTimestamp = glfwGetTime();

    while (!glfwWindowShouldClose(m_Window))
    {
        if (m_Callbacks.beforeFrame) m_Callbacks.beforeFrame(*this, m_FrameIndex);
        glfwPollEvents();
        UpdateWindowSize();

        bool presentSuccess = AnimateRenderPresent();
        if (!presentSuccess)
        {
            break;
        }
    }
}

bool DeviceManager::AnimateRenderPresent()
{
    double currTime = glfwGetTime();
    double elapsedTime = currTime - m_PreviouseFrameTimestamp;

    JoyStickManager::GetInstance().EraseDisconnectedJoysticks();
    JoyStickManager::GetInstance().UpdateAllJoysticks(m_vRenderPasses);

    if (m_WindowVisible && (m_WindowIsInFocus || ShouldRenderUnfocused()))
    {
        if (m_PrevDPIScaleFactorX != m_DPIScaleFactorX || m_PrevDPIScaleFactorY != m_DPIScaleFactorY)
        {
            DisplayScaleChanged();
            m_PrevDPIScaleFactorX = m_DPIScaleFactorX;
            m_PrevDPIScaleFactorY = m_DPIScaleFactorY;
        }

        if (m_Callbacks.beforeAnimate) m_Callbacks.beforeAnimate(*this, m_FrameIndex);
        Animate(elapsedTime);

        if (m_Callbacks.afterAnimate) m_Callbacks.afterAnimate(*this, m_FrameIndex);

        if (m_FrameIndex > 0 || !m_SkipRenderOnFirstFrame)
        {
            if (BeginFrame())
            {
                u32 frameIndex = m_FrameIndex;
                if (m_SkipRenderOnFirstFrame)
                {
                    frameIndex--;
                }

                if (m_Callbacks.beforeRender) m_Callbacks.beforeRender(*this, frameIndex);
                Render();
                if (m_Callbacks.afterRender) m_Callbacks.afterRender(*this, frameIndex);

                if (m_Callbacks.beforePresent) m_Callbacks.beforePresent(*this, frameIndex);
                bool presentSuccess = Present();
                if (m_Callbacks.afterPresent) m_Callbacks.afterPresent(*this, frameIndex);

                if (!presentSuccess)
                    return false;
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(0));
    GetDevice()->runGarbageCollection();

    UpdateAverageFrameTime(elapsedTime);
    m_PreviouseFrameTimestamp = currTime;

    ++m_FrameIndex;
    return true;
}

void DeviceManager::GetWindowDimensions(int &width, int &height)
{
    width = m_DeviceParams.backBufferWidth;
    height = m_DeviceParams.backBufferHeight;
}

const DeviceCreationParameters &DeviceManager::GetDeviceParams()
{
    return m_DeviceParams;
}

DeviceManager::DeviceManager()
{
}

void DeviceManager::UpdateWindowSize()
{
    int width, height;
    glfwGetWindowSize(m_Window, &width, &height);

    if (width == 0 || height == 0)
    {
        m_WindowVisible = false;
        return;
    }

    m_WindowVisible = true;

    m_WindowIsInFocus = glfwGetWindowAttrib(m_Window, GLFW_FOCUSED) == 1;

    if (int(m_DeviceParams.backBufferWidth) != width
        || int(m_DeviceParams.backBufferHeight) != height
        || (m_DeviceParams.vsyncEnable != m_RequestedVSync && GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN))
    {
        // window is not minimized, and the size has changed

        BackBufferResizing();

        m_DeviceParams.backBufferWidth = width;
        m_DeviceParams.backBufferHeight = height;
        m_DeviceParams.vsyncEnable = m_RequestedVSync;

        ResizeSwapChain();
        BackBufferResized();
    }

    m_DeviceParams.vsyncEnable = m_RequestedVSync;
}

void DeviceManager::WindowPosCallback(int x, int y)
{
    if (m_DeviceParams.enablePerMonitorDPI)
    {
#ifdef PLATFORM_WINDOWS
        HWND hwnd = glfwGetWin32Window(m_Window);
        auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

        u32 dpiX, dpiY;
        GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        m_DPIScaleFactorX = dpiX / 96.f;
        m_DPIScaleFactorY = dpiY / 96.f;
#else
    GLFWmonitor *monitor = glfwGetWindowMonitor(m_Window);
    if (!monitor)
        monitor = glfwGetPrimaryMonitor();
    glfwGetMonitorContentScale(monitor, &m_DPIScaleFactorX, &m_DPIScaleFactorY);
#endif
    }

    if (m_EnableRenderDuringWindowMovement && m_SwapChainFramebuffers.size() > 0)
    {
        if (m_Callbacks.beforeFrame) m_Callbacks.beforeFrame(*this, m_FrameIndex);
        AnimateRenderPresent();
    }
}

void DeviceManager::KeyboardUpdate(int key, int scancode, int action, int mods)
{
    if (key == -1)
        return;
    for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
    {
        bool ret = (*it)->KeyboardUpdate(key, scancode, action, mods);
        if (ret)
            break;
    }
}

void DeviceManager::KeyboardCharInput(unsigned int unicode, int mods)
{
    for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
    {
        bool ret = (*it)->KeyboardCharInput(unicode, mods);
        if (ret)
            break;
    }
}

void DeviceManager::MousePosUpdate(double xpos, double ypos)
{
    if (!m_DeviceParams.supportExplicitDisplayScaling)
    {
        xpos /= m_DPIScaleFactorX;
        ypos /= m_DPIScaleFactorY;
    }
    
    for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
    {
        bool ret = (*it)->MousePosUpdate(xpos, ypos);
        if (ret)
            break;
    }
}

void DeviceManager::MouseButtonUpdate(int button, int action, int mods)
{
    for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
    {
        bool ret = (*it)->MouseButtonUpdate(button, action, mods);
        if (ret)
            break;
    }
}

void DeviceManager::MouseScrollUpdate(double xoffset, double yoffset)
{
    for (auto it = m_vRenderPasses.crbegin(); it != m_vRenderPasses.crend(); it++)
    {
        bool ret = (*it)->MouseScrollUpdate(xoffset, yoffset);
        if (ret)
            break;
    }
}

void JoyStickManager::EnumerateJoysticks()
{
    // The glfw header says nothing about what values to expect for joystick IDs. Empirically, having connected two
    // simultaneously, glfw just seems to number them starting at 0.
    for (int i = 0; i != 10; ++i)
    {
        if (glfwJoystickPresent(i))
        {
            m_JoystickIDs.push_back(i);
        }
    }
}


void JoyStickManager::EraseDisconnectedJoysticks()
{
    while (!m_RemovedJoysticks.empty())
    {
        auto id = m_RemovedJoysticks.back();
        m_RemovedJoysticks.pop_back();

        auto it = std::find(m_JoystickIDs.begin(), m_JoystickIDs.end(), id);
        if (it != m_JoystickIDs.end())
            m_JoystickIDs.erase(it);
    }
}

void JoyStickManager::ConnectJoystick(int id)
{
    m_JoystickIDs.push_back(id);
}

void JoyStickManager::DisconnectJoystick(int id)
{
    // This fn can be called from inside glfwGetJoystickAxes below (similarly for buttons, I guess).
    // We can't call m_JoystickIDs.erase() here and now. Save them for later. Forunately, glfw docs
    // say that you can query a joystick ID that isn't present.
    m_RemovedJoysticks.push_back(id);
}

void JoyStickManager::UpdateAllJoysticks(const std::list<IRenderPass*>& passes)
{
    for (auto j = m_JoystickIDs.begin(); j != m_JoystickIDs.end(); ++j)
        UpdateJoystick(*j, passes);
}

static void ApplyDeadZone(glm::vec2 v, const float deadZone = 0.1f)
{
    v *= glm::max(glm::length(v) - deadZone, 0.f) / (1.f - deadZone);
}

void JoyStickManager::UpdateJoystick(int j, const std::list<IRenderPass *> &renderPasses)
{
    GLFWgamepadstate gamepadState;
    glfwGetGamepadState(j, &gamepadState);

    f32 *axisValues = gamepadState.axes;

    auto updateAxis = [&] (int axis, float axisVal)
    {
        for (auto it = renderPasses.crbegin(); it != renderPasses.crend(); ++it)
        {
            bool ret = (*it)->JoystickAxisUpdate(axis, axisVal);
            if (ret)
                break;
        }
    };

    {
        glm::vec2 v{axisValues[GLFW_GAMEPAD_AXIS_LEFT_X], axisValues[GLFW_GAMEPAD_AXIS_LEFT_Y]};
        ApplyDeadZone(v);
        updateAxis(GLFW_GAMEPAD_AXIS_LEFT_X, v.x);
        updateAxis(GLFW_GAMEPAD_AXIS_LEFT_Y, v.y);
    }

    {
        glm::vec2 v{axisValues[GLFW_GAMEPAD_AXIS_RIGHT_X], axisValues[GLFW_GAMEPAD_AXIS_RIGHT_Y]};
        ApplyDeadZone(v);
        updateAxis(GLFW_GAMEPAD_AXIS_RIGHT_X, v.x);
        updateAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y, v.y);
    }

    updateAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, axisValues[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
    updateAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, axisValues[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);

    for (i32 b = 0; b != GLFW_GAMEPAD_BUTTON_LAST; ++b)
    {
        auto buttonVal = gamepadState.buttons[b];
        for (auto it = renderPasses.crbegin(); it != renderPasses.crend(); ++it)
        {
            bool ret = (*it)->JoystickButtonUpdate(b, buttonVal == GLFW_PRESS);
            if (ret)
                break;
        }
    }
}

void DeviceManager::Shutdown()
{
    m_SwapChainFramebuffers.clear();

    DestroyDeviceAndSwapChain();

    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }

    glfwTerminate();

    m_InstanceCreated = false;
}

nvrhi::IFramebuffer* DeviceManager::GetCurrentFramebuffer()
{
    return GetFramebuffer(GetCurrentBackBufferIndex());
}

nvrhi::IFramebuffer* DeviceManager::GetFramebuffer(uint32_t index)
{
    if (index < m_SwapChainFramebuffers.size())
        return m_SwapChainFramebuffers[index];

    return nullptr;
}

void DeviceManager::SetWindowTitle(const char* title)
{
    assert(title);
    if (m_WindowTitle == title)
        return;

    glfwSetWindowTitle(m_Window, title);

    m_WindowTitle = title;
}

void DeviceManager::SetInformativeWindowTitle(const char* applicationName, bool includeFramerate, const char* extraInfo)
{
    std::stringstream ss;
    ss << applicationName;
    ss << " (" << nvrhi::utils::GraphicsAPIToString(GetDevice()->getGraphicsAPI());

    if (m_DeviceParams.enableDebugRuntime)
    {
        if (GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN)
            ss << ", VulkanValidationLayer";
        else
            ss << ", DebugRuntime";
    }

    if (m_DeviceParams.enableNvrhiValidationLayer)
    {
        ss << ", NvrhiValidationLayer";
    }

    ss << ")";

    double frameTime = GetAverageFrameTimeSeconds();
    if (includeFramerate && frameTime > 0)
    {
        double const fps = 1.0 / frameTime;
        int const precision = (fps <= 20.0) ? 1 : 0;
        ss << " - " << std::fixed << std::setprecision(precision) << fps << " FPS ";
    }

    if (extraInfo)
        ss << extraInfo;

    SetWindowTitle(ss.str().c_str());
}

const char *DeviceManager::GetWindowTitle()
{
    return m_WindowTitle.c_str();
}

DeviceManager* DeviceManager::Create(nvrhi::GraphicsAPI api)
{
    switch (api)
    {
#if IGNITE_WITH_DX11
    case nvrhi::GraphicsAPI::D3D11:
        return CreateD3D11();
#endif
#if IGNITE_WITH_DX12
    case nvrhi::GraphicsAPI::D3D12:
        return CreateD3D12();
#endif
#if IGNITE_WITH_VULKAN
    case nvrhi::GraphicsAPI::VULKAN:
        return CreateVK();
#endif
    default:
        LOG_ASSERT(false, "DeviceManager::Create: Unsuperted Graphics API {}", (u32)api);
        return nullptr;
    }
}
