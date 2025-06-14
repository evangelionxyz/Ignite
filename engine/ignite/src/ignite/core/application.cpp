#include "application.hpp"

#include "ignite/graphics/shader_factory.hpp"
#include "input/app_event.hpp"
#include "ignite/imgui/imgui_layer.hpp"
#include "ignite/graphics/renderer.hpp"
#include "ignite/audio/fmod_audio.hpp"
#include "ignite/physics/jolt/jolt_physics.hpp"

#include <nvrhi/utils.h>

namespace ignite
{
    static Application *s_JoltInstance = nullptr;

    Application::Application(const ApplicationCreateInfo &createInfo)
        : m_CreateInfo(createInfo)
    {
        s_JoltInstance = this;

        if (m_CreateInfo.cmdLineArgs.count > 1)
        {
            for (i32 i = 0; i < m_CreateInfo.cmdLineArgs.count; ++i)
            {
                if (strcmp(createInfo.cmdLineArgs.args[i], "-dx12") == 0)
                {
                    m_CreateInfo.graphicsApi = nvrhi::GraphicsAPI::D3D12;
                }
            }
        }

        m_CommandManager = CreateScope<CommandManager>();

        DeviceCreationParameters deviceCreateInfo;
        deviceCreateInfo.backBufferWidth = m_CreateInfo.width;
        deviceCreateInfo.backBufferHeight = m_CreateInfo.height;
        deviceCreateInfo.startMaximized = m_CreateInfo.maximized;

        m_Window = CreateScope<Window>(
            m_CreateInfo.name.c_str(),
            deviceCreateInfo,
            m_CreateInfo.graphicsApi
        );

        m_Window->SetEventCallback(BIND_CLASS_EVENT_FN(Application::OnEvent));
        m_Input = Input(m_Window->GetWindowHandle());

        m_Renderer = CreateRef<Renderer>(m_Window->GetDeviceManager(), m_CreateInfo.graphicsApi);

        if (createInfo.useGui)
        {
            m_ImGuiLayer = CreateScope<ImGuiLayer>(GetDeviceManager());
            m_ImGuiLayer->Init();
        }

        FmodAudio::Init();
        JoltPhysics::Init();
    }

    Application *Application::GetInstance()
    {
        LOG_ASSERT(s_JoltInstance, "Application has not been created!");
        return s_JoltInstance;
    }

    DeviceManager * Application::GetDeviceManager()
    {
        return GetInstance()->m_Window->GetDeviceManager();
    }

    void Application::SetWindowTitle(const std::string &title)
    {
        GetInstance()->m_Window->SetTitle(title);
    }

    void Application::UpdateAverageTimeTime(f64 elapsedTime)
    {
        m_FrameTimeSum += elapsedTime;
        m_NumberOfAccumulatedFrames++;

        if (m_FrameTimeSum >= m_AverageTimeUpdateInterval && m_NumberOfAccumulatedFrames > 0)
        {
            m_AverageFrameTime = m_FrameTimeSum / static_cast<f64>(m_NumberOfAccumulatedFrames);
            m_NumberOfAccumulatedFrames = 0;
            m_FrameTimeSum = 0.0;
        }
    }

    void Application::ProcessMainThreadSubmissons()
    {
        while (!m_ThreadFuncs.empty())
        {
            auto func = m_ThreadFuncs.front();
            if (func())
            {
                m_ThreadFuncs.pop();
            }
            else
            {
                break;
            }
        }
    }

    void Application::PushLayer(Layer *layer)
    {
        layer->OnAttach();
        m_LayerStack.PushLayer(layer);
    }

    void Application::PopLayer(Layer *layer)
    {
        layer->OnDetach();
        m_LayerStack.PopLayer(layer);
    }

    void Application::Run()
    {
        DeviceManager *deviceManager = GetDeviceManager();

        while (m_Window->IsLooping())
        {
            m_Window->PollEvents();

            const f64 currTime = glfwGetTime();
            m_DeltaTime = static_cast<float>(currTime - m_PreviousTime);

            ProcessMainThreadSubmissons();

            FmodAudio::Update(m_DeltaTime);

            // update window title
            if (m_AverageFrameTime > 0)
            {
                std::stringstream ss;
                ss << m_CreateInfo.name;
                ss << " (" << nvrhi::utils::GraphicsAPIToString(deviceManager->GetDevice()->getGraphicsAPI());
                if (deviceManager->GetDeviceParams().enableDebugRuntime)
                {
                    if (m_CreateInfo.graphicsApi == nvrhi::GraphicsAPI::VULKAN)
                        ss << ", VulkanValidationLayer";
                    else
                        ss << ", DebugRuntime";
                }

                if (deviceManager->GetDeviceParams().enableNvrhiValidationLayer)
                    ss << ", NvrhiValidationLayer";
                ss << ")";

                const f64 fps = 1.0 / m_AverageFrameTime;
                const i32 precision = (fps <= 20.0) ? 1 : 0;
                ss << " - " << std::fixed << std::setprecision(precision) << fps << " FPS ";
                m_Window->SetTitle(ss.str().c_str());
            }

            if (m_Window->IsVisible() && m_Window->IsInFocus())
            {
                // update system (physics etc..)
                for (auto layer = m_LayerStack.rbegin(); layer != m_LayerStack.rend(); ++layer)
                    (*layer)->OnUpdate(static_cast<f32>(m_DeltaTime));

                // render to main framebuffer
                // begin render frame
                if (m_FrameIndex > 0)
                {
                    if (deviceManager->BeginFrame())
                    {
                        nvrhi::IFramebuffer *framebuffer = deviceManager->GetCurrentFramebuffer();

                        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
                        {
                            Layer *layer = *it;
                            layer->OnRender(framebuffer);

                            // ImGui rendering
                            if (m_CreateInfo.useGui)
                            {
                                m_ImGuiLayer->BeginFrame();
                                layer->OnGuiRender();
                                m_ImGuiLayer->EndFrame(framebuffer);
                            }
                        }

                        if (!deviceManager->Present())
                            continue;
                    }
                }
            }

            // call this at lease once per frame!
            deviceManager->GetDevice()->runGarbageCollection();

            UpdateAverageTimeTime(m_DeltaTime);
            // set previous time
            m_PreviousTime = currTime;
            ++m_FrameIndex;
        }

        deviceManager->GetDevice()->waitForIdle();

        if (m_ImGuiLayer)
        {
            m_ImGuiLayer->OnDetach();
            m_ImGuiLayer.reset();
        }

        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            (*it)->OnDetach();
            delete *it;
        }

        // destroy renderer first
        m_Renderer.reset();

        // destroy device
        deviceManager->Destroy();
        m_Window->Destroy();

        JoltPhysics::Shutdown();
        FmodAudio::Shutdown();
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }

    void Application::WindowIconify()
    {
        GetInstance()->m_Window->Iconify();
    }

    void Application::WindowMaximize()
    {
        GetInstance()->m_Window->Maximize();
    }

    void Application::WindowRestore()
    {
        GetInstance()->m_Window->Restore();
    }

    void Application::SubmitToMainThread(const std::function<bool()> func)
    {
        GetInstance()->m_ThreadFuncs.push(func);
    }

    CommandManager *Application::GetCommandManager()
    {
        return GetInstance()->m_CommandManager.get();
    }

    nvrhi::IDevice* Application::GetRenderDevice()
    {
        return GetInstance()->m_Window->GetDeviceManager()->GetDevice();
    }

    f32 Application::GetDeltaTime()
    {
        return GetInstance()->m_DeltaTime;
    }

}
