#include "application.hpp"

#include "ignite/graphics/shader_factory.hpp"
#include "input/app_event.hpp"
#include "ignite/imgui/imgui_layer.hpp"
#include "nvrhi/utils.h"
#include "ignite/graphics/renderer.hpp"

namespace ignite
{
    static Application *s_Instance = nullptr;

    Application::Application(const ApplicationCreateInfo &createInfo)
        : m_CreateInfo(createInfo)
    {
        s_Instance = this;

        DeviceCreationParameters deviceCreateInfo;
        deviceCreateInfo.backBufferWidth = createInfo.width;
        deviceCreateInfo.backBufferHeight = createInfo.height;
        deviceCreateInfo.startMaximized = createInfo.maximized;

        m_Window = CreateScope<Window>(
            createInfo.name.c_str(),
            deviceCreateInfo,
            createInfo.graphicsApi
        );

        m_Window->SetEventCallback(BIND_CLASS_EVENT_FN(Application::OnEvent));
        m_Input = Input(m_Window->GetWindowHandle());

        // create global shader factory
        std::filesystem::path shaderPath = "resources/shaders/dxil";
        Ref<vfs::NativeFileSystem> nativeFS = CreateRef<vfs::NativeFileSystem>();
        m_ShaderFactory = CreateRef<ShaderFactory>(GetDeviceManager()->GetDevice(), nativeFS, shaderPath);

        m_ImGuiLayer = CreateScope<ImGuiLayer>(GetDeviceManager());
        m_ImGuiLayer->Init(GetShaderFactory());

        Renderer::Init(m_Window->GetDeviceManager());
    }

    Application *Application::GetInstance()
    {
        LOG_ASSERT(s_Instance, "Application has not been created!");
        return s_Instance;
    }

    DeviceManager * Application::GetDeviceManager()
    {
        return GetInstance()->m_Window->GetDeviceManager();
    }

    Ref<ShaderFactory> Application::GetShaderFactory()
    {
        return GetInstance()->m_ShaderFactory;
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

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PopLayer(Layer *layer)
    {
        m_LayerStack.PopLayer(layer);
    }

    void Application::Run()
    {
        DeviceManager *deviceManager = GetDeviceManager();

        while (m_Window->IsLooping())
        {
            m_Window->PollEvents();

            const f64 currTime = glfwGetTime();
            const f64 deltaTime = currTime - m_PreviousTime;

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
                    (*layer)->OnUpdate(static_cast<f32>(deltaTime));

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
                            m_ImGuiLayer->BeginFrame();
                            layer->OnGuiRender();
                            m_ImGuiLayer->EndFrame(framebuffer);
                        }
                        if (!deviceManager->Present())
                            continue;
                    }
                }
            }
            UpdateAverageTimeTime(deltaTime);
            // set previous time
            m_PreviousTime = currTime;
            ++m_FrameIndex;
        }

        for (auto layer = m_LayerStack.rbegin(); layer != m_LayerStack.rend(); ++layer)
            (*layer)->OnDetach();

        if (m_ImGuiLayer)
            m_ImGuiLayer->OnDetach();

        Renderer::Shutdown();
        m_Window->Destroy();
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
}
