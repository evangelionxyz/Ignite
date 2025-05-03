#pragma once

#include "layer.hpp"
#include "layer_stack.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "device/device_manager.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/graphics/window.hpp"
#include "ignite/imgui/imgui_layer.hpp"
#include "input/app_event.hpp"
#include "input/input.hpp"
#include "command.hpp"

#include <filesystem>

namespace ignite
{
    class ShaderFactory;
    class Renderer;

    struct ApplicationCommandLineArgs
    {
        i32 count = 0;
        char **args = nullptr;

        const char *operator[](int index) const
        {
            LOG_ASSERT(index < count, "Invalid index");
            return args[index];
        }
    };

    struct ApplicationCreateInfo
    {
        ApplicationCommandLineArgs cmdLineArgs;
        std::string name = "Ignite";
        std::string iconPath = " ";
        std::string workingDirectory;
        nvrhi::GraphicsAPI graphicsApi = nvrhi::GraphicsAPI::VULKAN;

        u32 width = 1280;
        u32 height = 640;
        bool maximized = false;
        bool useGui = true;
    };

    class Application
    {
    public:
        Application(const ApplicationCreateInfo &createInfo);
        virtual ~Application() = default;

        void PushLayer(Layer *layer);
        void PopLayer(Layer *layer);

        void Run();
        void OnEvent(Event &e);

        std::string GetAppName() { return m_CreateInfo.name; }
        const ApplicationCreateInfo &GetCreateInfo() { return m_CreateInfo; }

        Window *GetWindow() { return m_Window.get(); }

        static Application *GetInstance();
        static DeviceManager *GetDeviceManager();
        static CommandManager *GetCommandManager();

        static void SetWindowTitle(const std::string &title);

        static void WindowIconify();
        static void WindowMaximize();
        static void WindowRestore();

    private:
        void UpdateAverageTimeTime(f64 elapsedTime);

    protected:
        ApplicationCreateInfo m_CreateInfo;
        Scope<Window> m_Window;
        Scope<CommandManager> m_CommandManager;
        LayerStack m_LayerStack;
        Ref<ImGuiLayer> m_ImGuiLayer;
        Input m_Input;

        Ref<Renderer> m_Renderer;

        f64 m_PreviousTime = 0.0;
        f64 m_FrameTimeSum = 0.0;
        f64 m_AverageFrameTime = 0.0;
        const f64 m_AverageTimeUpdateInterval = 0.5;
        i32 m_NumberOfAccumulatedFrames = 0;
        i32 m_FrameIndex = 0;
    };

    Application *CreateApplication(ApplicationCommandLineArgs args);
}
