#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"

#include "states.hpp"

namespace ignite
{
    class ShaderFactory;
    class DeviceManager;

    class ScenePanel;

    class EditorLayer final : public Layer
    {
    private:
        struct EditorData
        {
            bool debugMode = false;
            bool developerMode = false;
            State sceneState = State_SceneEdit;
        };

    public:
        EditorLayer(const std::string &name);

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(float deltaTime) override;
        void OnEvent(Event& e) override;
        bool OnKeyPressedEvent(KeyPressedEvent &event);
        void OnRender(nvrhi::IFramebuffer *framebuffer) override;
        void OnGuiRender() override;

        Scene *GetActiveScene() { return m_ActiveScene.get(); }
        EditorData GetState() { return m_Data; }

    private:
        void NewScene();
        void OnScenePlay();
        void OnSceneStop();
        
        Ref<ScenePanel> m_ScenePanel;
        
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;

        Ref<Texture> m_Texture;
        EditorData m_Data;
        
        nvrhi::CommandListHandle m_CommandList;
        DeviceManager *m_DeviceManager;
    };
}