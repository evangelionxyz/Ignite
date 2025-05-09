#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"
#include "ignite/graphics/environment.hpp"

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
            bool multiSelect = false;
            State sceneState = State_SceneEdit;
        };

    public:
        EditorLayer(const std::string &name);

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(f32 deltaTime) override;
        void OnEvent(Event& e) override;
        bool OnKeyPressedEvent(KeyPressedEvent &event);
        void OnRender(nvrhi::IFramebuffer *framebuffer) override;
        void OnGuiRender() override;

        Scene *GetActiveScene() { return m_ActiveScene.get(); }
        EditorData GetState() const { return m_Data; }

    private:
        void NewScene();
        void OnScenePlay();
        void OnSceneStop();
        void OnSceneSimulate();

        void TraverseMeshes(Ref<Mesh> mesh, int traverseIndex = 0);

        Ref<ScenePanel> m_ScenePanel;
        
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;
        Ref<Model> m_Model;
        Ref<GraphicsPipeline> m_MeshPipeline;
        Ref<GraphicsPipeline> m_EnvPipeline;
        Environment m_Env;
        EditorData m_Data;
        
        nvrhi::CommandListHandle m_CommandList;
        nvrhi::IDevice *m_Device = nullptr;
    };
}