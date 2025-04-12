#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"

class ShaderFactory;
class DeviceManager;

namespace ignite
{
    class ScenePanel;

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
        Ref<Texture> m_Texture;
        
        nvrhi::CommandListHandle m_CommandList;
        DeviceManager *m_DeviceManager;

        struct State
        {
            bool debugMode = false;
            bool developerMode = false;
        } m_State;
        
        friend class ScenePanel;
    };
}