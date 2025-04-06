#pragma once

#include "device/device_manager.hpp"
#include "layer.hpp"
#include "layer_stack.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "core/input/event.hpp"
#include "graphics/window.hpp"
#include "imgui/imgui_layer.hpp"
#include "input/app_event.hpp"

#include <filesystem>

class ShaderFactory;

struct ApplicationCommandLineArgs
{
    i32 Count = 0;
    char **Args = nullptr;

    const char *operator[](int index) const
    {
        LOG_ASSERT(index < Count, "Invalid index");
        return Args[index];
    }
};

struct ApplicationCreateInfo
{
    ApplicationCommandLineArgs CommandLineArgs;
    std::string Name = "Ignite";
    std::string IconPath = " ";
    std::string WorkingDirectory;
    nvrhi::GraphicsAPI GraphicsAPI = nvrhi::GraphicsAPI::D3D12;

    u32 Width = 1280;
    u32 Height = 640;
    bool Maximized = false;
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
    bool OnWindowResizeCallback(WindowResizeEvent &event);

    std::string GetAppName() { return m_CreateInfo.Name; }
    const ApplicationCreateInfo &GetCreateInfo() { return m_CreateInfo; }

    static Application *GetInstance();
    static DeviceManager *GetDeviceManager();
    static Ref<ShaderFactory> GetShaderFactory();
    static void SetWindowTitle(const std::string &title);

private:
    void UpdateAverageTimeTime(f64 elapsedTime);

protected:
    ApplicationCreateInfo m_CreateInfo;
    Scope<Window> m_Window;
    Ref<ShaderFactory> m_ShaderFactory;
    LayerStack m_LayerStack;
    Ref<ImGuiLayer> m_ImGuiLayer;

    f64 m_PreviousTime = 0.0;
    f64 m_FrameTimeSum = 0.0;
    f64 m_AverageFrameTime = 0.0;
    const f64 m_AverageTimeUpdateInterval = 0.5;
    i32 m_NumberOfAccumulatedFrames = 0;
    i32 m_FrameIndex = 0;

};

Application *CreateApplication(const ApplicationCommandLineArgs args);