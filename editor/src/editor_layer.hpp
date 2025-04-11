#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"

class ShaderFactory;
class DeviceManager;

namespace ignite
{
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

    class EditorLayer final : public Layer
    {
    public:
        EditorLayer(const std::string &name);

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float deltaTime) override;
        void OnEvent(Event& e) override;
        void OnRender(nvrhi::IFramebuffer *framebuffer) override;
        void OnGuiRender() override;

    private:
        Ref<ScenePanel> m_ScenePanel;
        Ref<Scene> m_ActiveScene;

        RenderData sceneGfx;
        nvrhi::CommandListHandle m_CommandList;
        DeviceManager *m_DeviceManager;
    };
}