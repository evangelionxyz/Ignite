#include "ignite/core/layer.hpp"

namespace ignite
{
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
        
    };
}