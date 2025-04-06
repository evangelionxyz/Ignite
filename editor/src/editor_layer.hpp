#pragma once
#include <ipanel.hpp>
#include <ignite/ignite.hpp>

class ShaderFactory;
class DeviceManager;
class ImGuiRenderPass;
class SceneRenderPass;

class IgniteEditorLayer final : public Layer
{
public:
    IgniteEditorLayer(const std::string &name);

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnEvent(Event& e) override;
    void OnRender(nvrhi::IFramebuffer *framebuffer) override;
    void OnGuiRender() override;

private:
    Ref<IPanel> m_ScenePanel;
    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;
    nvrhi::GraphicsPipelineHandle m_Pipeline;
    nvrhi::CommandListHandle m_CommandList;
    DeviceManager *m_DeviceManager;
};
