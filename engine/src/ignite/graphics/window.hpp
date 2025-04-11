#pragma once

#include <glfw/glfw3.h>

#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <glfw/glfw3native.h>

#include "ignite/core/device/device_manager.hpp"
#include "ignite/core/input/event.hpp"

#include "ignite/core/types.hpp"

namespace ignite
{
    class Window
    {
    public:
        explicit Window(const char *windowTitle, const DeviceCreationParameters &createInfo, nvrhi::GraphicsAPI graphicsApi);

        [[nodiscard]] GLFWwindow *GetWindowHandle() const { return m_DeviceManager->m_Window; }
        [[nodiscard]] bool IsLooping() const { return glfwWindowShouldClose(m_DeviceManager->m_Window) == 0; };

        void PollEvents();
        void Destroy();

        std::string &GetTitle() { return m_WindowTitle; }
        void SetEventCallback(const std::function<void(Event&)>& callback);
        [[nodiscard]] DeviceManager *GetDeviceManager() const { return m_DeviceManager; }

        [[nodiscard]] bool IsVisible() const { return m_DeviceManager->m_WindowVisible; }
        [[nodiscard]] bool IsInFocus() const { return m_DeviceManager->m_WindowIsInFocus; }

        void SetTitle(const std::string &title) const;

        void Iconify() const;
        void Maximize() const;
        void Restore() const;

    private:
        void SetCallbacks() const;
        DeviceManager *m_DeviceManager;
        std::string m_WindowTitle;

        std::function<void(Event&)> m_Callback;
    };
}