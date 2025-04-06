#include "ignite_editor_app.hpp"
#include "core/vfs/vfs.hpp"
#include "renderer/shader_factory.hpp"

EditorApp::EditorApp(const ApplicationCreateInfo &createInfo)
    : Application(createInfo)
{
    m_DeviceManager = DeviceManager::Create(createInfo.graphicsApi);
#ifdef _DEBUG
    m_DeviceParams.enableDebugRuntime = true;
    m_DeviceParams.enableNvrhiValidationLayer= true;
#endif

    if (!m_DeviceManager->CreateWindowDeviceAndSwapChain(m_DeviceParams, createInfo.appName.c_str()))
    {
        LOG_ASSERT(false, "Cannot initalize a graphics device with the requested parameters");
        return;
    }

    std::filesystem::path shaderPath = "resources/shaders/dxil";
    
    auto nativeFS = CreateRef<vfs::NativeFileSystem>();
    m_ShaderFactory = CreateRef<ShaderFactory>(m_DeviceManager->GetDevice(), nativeFS, shaderPath);

    m_SceneRenderPass = CreateRef<SceneRenderPass>(m_DeviceManager);
    m_SceneRenderPass->Init(m_ShaderFactory);

    m_GuiRenderPass = CreateRef<ImGuiRenderPass>(m_DeviceManager);
    m_GuiRenderPass->Init(m_ShaderFactory);
}

void EditorApp::Run()
{
    m_DeviceManager->AddRenderPassToBack(m_SceneRenderPass.get());
    m_DeviceManager->AddRenderPassToBack(m_GuiRenderPass.get());

    m_DeviceManager->RunMessageLoop();

    Destroy();
}

void EditorApp::Destroy()
{
    if (m_DeviceManager)
    {
        m_DeviceManager->RemoveRenderPass(m_GuiRenderPass.get());
        m_DeviceManager->RemoveRenderPass(m_SceneRenderPass.get());

        m_DeviceManager->Destroy();
        delete m_DeviceManager;
    }
}