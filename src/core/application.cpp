#include "application.hpp"
#include "renderer/shader_factory.hpp"
#include <nvrhi/utils.h>

Application::Application(DeviceManager *deviceManager)
    : IRenderPass(deviceManager)
{
}

bool Application::Init()
{
    std::filesystem::path shaderPath = "resources/shaders/dxil";

    auto nativeFS = CreateRef<vfs::NativeFileSystem>();
    ShaderFactory shaderFactory(GetDevice(), nativeFS, shaderPath);
    m_VertexShader = shaderFactory.CreateShader("triangle.hlsl", "main_vs", nullptr, nvrhi::ShaderType::Vertex);
    m_PixelShader = shaderFactory.CreateShader("triangle.hlsl", "main_ps", nullptr, nvrhi::ShaderType::Pixel);

    if (!m_VertexShader || !m_PixelShader)
    {
        return false;
    }

    m_CommandList = GetDevice()->createCommandList();

    return true;
}

void Application::Run()
{
}

void Application::Shutdown()
{
    m_VertexShader = nullptr;
    m_PixelShader = nullptr;
    m_CommandList = nullptr;
    m_Pipeline = nullptr;
}


void Application::BackBufferResizing()
{
    m_Pipeline = nullptr;
}

void Application::Animate(f32 elapsedTimeSeconds)
{
    GetDeviceManager()->SetInformativeWindowTitle("Ignite Editor");
}

void Application::Render(nvrhi::IFramebuffer *framebuffer)
{
    if (!m_Pipeline)
    {
        nvrhi::GraphicsPipelineDesc psoDesc;
        psoDesc.VS = m_VertexShader;
        psoDesc.PS = m_PixelShader;
        psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
        psoDesc.renderState.depthStencilState.depthTestEnable = false;

        m_Pipeline = GetDevice()->createGraphicsPipeline(psoDesc, framebuffer);
    }

    m_CommandList->open();

    nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(1.0f, 0.0f, 0.0f, 1.0f));
    nvrhi::GraphicsState state;
    state.pipeline = m_Pipeline;
    state.framebuffer = framebuffer;
    state.viewport.addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport());

    m_CommandList->setGraphicsState(state);

    nvrhi::DrawArguments args;
    args.vertexCount = 3;
    m_CommandList->draw(args);

    m_CommandList->close();

    GetDevice()->executeCommandList(m_CommandList);
}

void Application::RenderSplashScreen(nvrhi::IFramebuffer *framebuffer)
{

}




