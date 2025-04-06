#include <editor_layer.hpp>
#include <scene_panel.hpp>

#include "nvrhi/utils.h"

IgniteEditorLayer::IgniteEditorLayer(const std::string &name)
    : Layer(name)
{
}

void IgniteEditorLayer::OnAttach()
{
    Layer::OnAttach();

    m_DeviceManager = Application::GetDeviceManager();

    m_VertexShader = Application::GetShaderFactory()->CreateShader("triangle.hlsl", "main_vs", nullptr, nvrhi::ShaderType::Vertex);
    m_PixelShader = Application::GetShaderFactory()->CreateShader("triangle.hlsl", "main_ps", nullptr, nvrhi::ShaderType::Pixel);
    LOG_ASSERT(m_VertexShader && m_PixelShader, "Failed to create shader");


    m_ScenePanel = IPanel::Create<ScenePanel>("Scene Panel");
    m_CommandList = m_DeviceManager->GetDevice()->createCommandList();
}

void IgniteEditorLayer::OnDetach()
{
    Layer::OnDetach();
}

void IgniteEditorLayer::OnUpdate(f32 deltaTime)
{
    Layer::OnUpdate(deltaTime);
}

void IgniteEditorLayer::OnEvent(Event &e)
{
    Layer::OnEvent(e);
}

void IgniteEditorLayer::OnRender(nvrhi::IFramebuffer *framebuffer)
{
    Layer::OnRender(framebuffer);

    if (!m_Pipeline)
    {
        nvrhi::GraphicsPipelineDesc psoDesc;
        psoDesc.VS = m_VertexShader;
        psoDesc.PS = m_PixelShader;
        psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
        psoDesc.renderState.depthStencilState.depthTestEnable = false;
        m_Pipeline = m_DeviceManager->GetDevice()->createGraphicsPipeline(psoDesc, framebuffer);
    }

    m_CommandList->open();
    nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.1f, 0.1f, 0.3f, 1.0f));

    nvrhi::GraphicsState state;
    state.pipeline = m_Pipeline;
    state.framebuffer = framebuffer;
    state.viewport.addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport());

    m_CommandList->setGraphicsState(state);

    nvrhi::DrawArguments args;
    args.vertexCount = 3;
    m_CommandList->draw(args);

    m_CommandList->close();
    m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);
}

void IgniteEditorLayer::OnGuiRender()
{
    m_ScenePanel->OnGuiRender();
}
