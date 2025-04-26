#include "ignite/core/layer.hpp"
#include "ignite/graphics/shader_factory.hpp"
#include "ignite/core/logger.hpp"

#include "ignite/graphics/shader.hpp"

namespace ignite
{
    class DeviceManager;

    struct TestData
    {
        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;

        nvrhi::InputLayoutHandle inputLayout;
        nvrhi::BindingLayoutHandle bindingLayout;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;

        nvrhi::GraphicsPipelineDesc pipelineDesc;
        nvrhi::GraphicsPipelineHandle pipeline;

        nvrhi::CommandListHandle commandList;
    };

    class TestLayer : public Layer
    {
    public:
        TestLayer(const std::string &name);

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(f32 deltaTime) override;
        void OnEvent(Event& e) override;
        void OnRender(nvrhi::IFramebuffer *framebuffer) override;
        void OnGuiRender() override;

    private:
        DeviceManager *m_DeviceManager;
        nvrhi::IDevice *m_Device;

        Ref<ShaderFactory> m_ShaderFactory;

        TestData s_data;
    };
}