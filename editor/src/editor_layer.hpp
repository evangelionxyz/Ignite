#pragma once

#include "ipanel.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ignite/core/layer.hpp"
#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

class ShaderFactory;
class DeviceManager;
class ImGuiRenderPass;
class SceneRenderPass;

class ScenePanel;

struct RenderData
{
    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingSetHandle bindingSet;
    nvrhi::GraphicsPipelineDesc psoDesc;
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    nvrhi::BufferHandle constantBuffer;
    nvrhi::BufferDesc vertexBufferDesc;
    nvrhi::TextureHandle texture;
    nvrhi::SamplerHandle sampler;
};

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
    Ref<ScenePanel> m_ScenePanel;
    RenderData sceneGfx;
    nvrhi::CommandListHandle m_CommandList;
    DeviceManager *m_DeviceManager;
};
