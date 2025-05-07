#include "ignite/core/layer.hpp"
#include "ignite/graphics/shader_factory.hpp"
#include "ignite/graphics/render_target.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"

#include "ignite/graphics/shader.hpp"

namespace ignite
{
    class DeviceManager;
    class Camera;

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
        Scope<Camera> m_ViewportCamera;

        Ref<RenderTarget> m_RenderTarget;

        nvrhi::CommandListHandle m_CommandList;
    };
}