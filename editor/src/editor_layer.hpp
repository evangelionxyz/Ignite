#pragma once
#include <ipanel.hpp>
#include <ignite/ignite.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class ShaderFactory;
class DeviceManager;
class ImGuiRenderPass;
class SceneRenderPass;

struct QuadVertex
{
    glm::vec2 position;
    glm::vec4 color;
};

struct RenderData
{
    nvrhi::ShaderHandle vertexShader;
    nvrhi::ShaderHandle pixelShader;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::GraphicsPipelineDesc psoDesc;
    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    nvrhi::BufferDesc vertexBufferDesc;
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
    nvrhi::BufferHandle CreateGeometryBuffer(nvrhi::ICommandList* commandList, const void* data, uint64_t dataSize);

    Ref<IPanel> m_ScenePanel;
    RenderData mainGfx;
    RenderData sceneGfx;

    nvrhi::CommandListHandle m_CommandList;
    DeviceManager *m_DeviceManager;

    glm::vec3 m_ClearColor {0.1f, 0.1f, 0.1f};
};
