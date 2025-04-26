#include "ignite/core/layer.hpp"
#include "ignite/graphics/shader_factory.hpp"

#include "nvrhi/nvrhi.h"

namespace ignite
{
    class DeviceManager;

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
        nvrhi::CommandListHandle m_CommandList;
        Ref<ShaderFactory> m_ShaderFactory;
    };
}