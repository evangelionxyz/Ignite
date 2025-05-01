#include "window.hpp"

#include "ignite/core/logger.hpp"
#include "ignite/core/input/app_event.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_event.hpp"
#include "ignite/core/input/mouse_event.hpp"

#ifdef _WIN32
#include <dwmapi.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Dwmapi.lib") // Link to DWM API
#pragma comment(lib, "shcore.lib")
#endif

namespace ignite
{
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

    static void GLFW_ErrorCallback(i32 error, const char *description)
    {
        LOG_ERROR("GLFW error: {}", description);
        exit(1);
    }

    Window::Window(const char *windowTitle, const DeviceCreationParameters &deviceParams, nvrhi::GraphicsAPI graphicsApi)
        : m_WindowTitle(windowTitle)
    {
        m_DeviceManager = DeviceManager::Create(graphicsApi);

        m_DeviceManager->m_DeviceParams = deviceParams;
#ifdef _DEBUG
        m_DeviceManager->m_DeviceParams.enableDebugRuntime = true;
        m_DeviceManager->m_DeviceParams.enableNvrhiValidationLayer= true;
        glfwSetErrorCallback(GLFW_ErrorCallback);
#endif

        // Create device instance
        bool result = m_DeviceManager->CreateInstance(m_DeviceManager->m_DeviceParams);
        LOG_ASSERT(result, "Failed to create Instance");

        m_DeviceManager->m_DeviceParams.headlessDevice = false;

        result = false;
        for (const auto &info : formatInfo)
        {
            if (info.format == m_DeviceManager->m_DeviceParams.swapChainFormat)
            {
                glfwWindowHint(GLFW_RED_BITS, info.redBits);
                glfwWindowHint(GLFW_GREEN_BITS, info.greenBits);
                glfwWindowHint(GLFW_BLUE_BITS, info.blueBits);
                glfwWindowHint(GLFW_ALPHA_BITS, info.alphaBits);
                glfwWindowHint(GLFW_DEPTH_BITS, info.depthBits);
                glfwWindowHint(GLFW_STENCIL_BITS, info.stencilBits);
                result = true;
                break;
            }
        }
        LOG_ASSERT(result, "GLFW format not found\n");

        glfwWindowHint(GLFW_SAMPLES, m_DeviceManager->m_DeviceParams.swapChainSampleCount);
        glfwWindowHint(GLFW_REFRESH_RATE, m_DeviceManager->m_DeviceParams.refreshRate);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, m_DeviceManager->m_DeviceParams.resizeWindowWithDisplayScale);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // ignored for full screen
        glfwWindowHint(GLFW_DECORATED, !m_DeviceManager->m_DeviceParams.startBorderless); // borderless window

        m_DeviceManager->m_Window = glfwCreateWindow(
            m_DeviceManager->m_DeviceParams.backBufferWidth,
            m_DeviceManager->m_DeviceParams.backBufferHeight,
           windowTitle ? windowTitle : "",
           m_DeviceManager->m_DeviceParams.startFullscreen ? glfwGetPrimaryMonitor() : nullptr,
           nullptr);

        LOG_ASSERT(m_DeviceManager->m_Window, "Failed to create GLFW window\n");

        if (m_DeviceManager->m_DeviceParams.startFullscreen)
        {
            glfwSetWindowMonitor(m_DeviceManager->m_Window, glfwGetPrimaryMonitor(),
                0, 0,
                m_DeviceManager->m_DeviceParams.backBufferWidth, m_DeviceManager->m_DeviceParams.backBufferHeight,
                m_DeviceManager->m_DeviceParams.refreshRate);
        }
        else
        {
            i32 fbWidth = 0, fbHeight = 0;
            glfwGetFramebufferSize(m_DeviceManager->m_Window, &fbWidth, &fbHeight);
            m_DeviceManager->m_DeviceParams.backBufferWidth = fbWidth;
            m_DeviceManager->m_DeviceParams.backBufferHeight = fbHeight;
        }

        glfwSetWindowUserPointer(m_DeviceManager->m_Window, this);

        if (m_DeviceManager->m_DeviceParams.windowPosX != -1 && m_DeviceManager->m_DeviceParams.windowPosY != -1)
            glfwSetWindowPos(m_DeviceManager->m_Window, m_DeviceManager->m_DeviceParams.windowPosX, m_DeviceManager->m_DeviceParams.windowPosY);

#if _WIN32
        HWND hwnd = glfwGetWin32Window(m_DeviceManager->m_Window);
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

        // 7160E8 visual studio purple
        COLORREF rgbRed = 0x00E86071;
        DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));

        // DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
        // DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
#endif

        result = m_DeviceManager->CreateDevice();
        LOG_ASSERT(result, "Failed to create Device Instance\n");

        result = m_DeviceManager->CreateSwapChain();
        LOG_ASSERT(result, "Failed to create Swap Chain\n");

        glfwShowWindow(m_DeviceManager->m_Window);
        m_DeviceManager->m_WindowVisible = true;
        m_DeviceManager->m_WindowIsInFocus = true;

        m_DeviceManager->ResizeSwapChain();
        m_DeviceManager->CreateBackBuffers();

        if (m_DeviceManager->m_DeviceParams.startMaximized)
            glfwMaximizeWindow(m_DeviceManager->m_Window);

        m_DeviceManager->m_DeviceParams.backBufferWidth = 0;
        m_DeviceManager->m_DeviceParams.backBufferHeight = 0;
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::Destroy()
    {
        if (m_DeviceManager->m_Window)
        {
            glfwDestroyWindow(m_DeviceManager->m_Window);
            m_DeviceManager->m_Window = nullptr;
        }
        glfwTerminate();
    }

    void Window::SetTitle(const std::string &title) const
    {
        glfwSetWindowTitle(m_DeviceManager->m_Window, title.c_str());
    }

    void Window::Iconify() const
    {
        glfwIconifyWindow(m_DeviceManager->m_Window);
    }

    void Window::Maximize() const
    {
        glfwMaximizeWindow(m_DeviceManager->m_Window);
    }

    void Window::Restore() const
    {
        glfwRestoreWindow(m_DeviceManager->m_Window);
    }

    void Window::SetCallbacks() const
    {
        glfwSetWindowPosCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 xpos, i32 ypos)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            win.m_DeviceManager->m_DeviceParams.windowPosX = xpos;
            win.m_DeviceManager->m_DeviceParams.windowPosY = ypos;

            if (win.m_DeviceManager->m_DeviceParams.enablePerMonitorDPI)
            {
#ifdef _WIN32
                HWND hwnd = glfwGetWin32Window(window);
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                u32 dpiX, dpiY;
                GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
                win.m_DeviceManager->m_DPIScaleFactorX = dpiX / 96.f;
                win.m_DeviceManager->m_DPIScaleFactorY = dpiY / 96.f;
#else
            GLFWmonitor *monitor = glfwGetWindowMonitor(window);
            if (!monitor)
                monitor = glfwGetPrimaryMonitor();
            glfwGetMonitorContentScale(monitor, &win.m_DeviceManager->m_DPIScaleFactorX, &win.m_DeviceManager->m_DPIScaleFactorY);
#endif
            }

            // render during window movement
            /*if (m_EnableRenderDuringWindowMovement && m_SwapChainFramebuffers.size() > 0)
            {
                AnimateRenderPresent();
            }*/
        });

        glfwSetFramebufferSizeCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 width, i32 height)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            FramebufferResizeEvent event(width, height);
            win.m_Callback(event);

            // window is not minimized, and the size has changed
            if (width > 0 && height > 0)
            {
                win.m_DeviceManager->m_DeviceParams.backBufferWidth = width;
                win.m_DeviceManager->m_DeviceParams.backBufferHeight = height;

                win.m_DeviceManager->ResizeSwapChain();
                win.m_DeviceManager->CreateBackBuffers();
            }
        });

        glfwSetWindowSizeCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 width, i32 height)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            WindowResizeEvent event(width, height);
            win.m_Callback(event);

            if (width == 0 || height == 0)
            {
                win.m_DeviceManager->m_WindowVisible = false;
                return;
            }

            win.m_DeviceManager->m_WindowVisible = true;
            win.m_DeviceManager->m_WindowIsInFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) == 1;
        });

        glfwSetWindowCloseCallback(m_DeviceManager->m_Window, [](GLFWwindow* window)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            WindowCloseEvent event;
            win.m_Callback(event);
        });

        glfwSetKeyCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            switch (action)
            {
            case GLFW_PRESS:
            {
                KeyPressedEvent event(key, 0);
                win.m_Callback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                KeyReleasedEvent event(key);
                win.m_Callback(event);
                break;
            }
            case GLFW_REPEAT:
            {
                KeyPressedEvent event(key, 1);
                win.m_Callback(event);
                break;
            }
            default: break;
            }
        });

        glfwSetCharCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, u32 keycode)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            KeyTypedEvent event(keycode);
            win.m_Callback(event);
        });

        glfwSetMouseButtonCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 button, i32 action, i32 mods)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            switch (action)
            {
            case GLFW_PRESS:
            {
                MouseButtonPressedEvent event(button);
                win.m_Callback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                MouseButtonReleasedEvent event(button);
                win.m_Callback(event);
                break;
            }
            default: break;
            }
        });

        glfwSetScrollCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, f64 x_offset, f64 y_offset)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            MouseScrolledEvent event(static_cast<float>(x_offset), static_cast<float>(y_offset));
            win.m_Callback(event);
        });

        glfwSetCursorPosCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, f64 xPos, f64 yPos)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
            win.m_Callback(event);
        });

        glfwSetDropCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 pathCount, const char* paths[])
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            std::vector<std::filesystem::path> filepaths(pathCount);

            LOG_INFO("Paths: ");
            for (i32 i = 0; i < pathCount; i++)
            {
                filepaths[i] = paths[i];
                LOG_INFO(" {}", filepaths[i].generic_string().c_str());
            }

            WindowDropEvent event(std::move(filepaths));
            win.m_Callback(event);
        });

        glfwSetWindowMaximizeCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 maximized)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            WindowMaximizedEvent event(maximized ? true : false);
            win.m_Callback(event);
        });

        glfwSetWindowIconifyCallback(m_DeviceManager->m_Window, [](GLFWwindow* window, i32 iconified)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            WindowMinimizedEvent event(iconified ? true : false);
            win.m_Callback(event);
        });

        glfwSetWindowCloseCallback(m_DeviceManager->m_Window, [](GLFWwindow* window)
        {
            Window &win = *static_cast<Window *>(glfwGetWindowUserPointer(window));
            WindowCloseEvent event;
            win.m_Callback(event);
        });
    }

    void Window::SetEventCallback(const std::function<void(Event &)> &callback)
    {
        m_Callback = callback;
        SetCallbacks();
    }
}
