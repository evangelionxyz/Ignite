#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/graphics/lighting.hpp"

#include "states.hpp"

#include <mutex>
#include <thread>
#include <future>

namespace ignite
{
    class ShaderFactory;
    class DeviceManager;
    class ScenePanel;

    struct DebugRenderData
    {
        int renderIndex = 0;
    };

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

        void TraverseMeshes(Model *model, Ref<Mesh> mesh, int traverseIndex = 0);

        // temporary
        void LoadModel(const std::string &filepath);

        Ref<ScenePanel> m_ScenePanel;
        
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;

        std::vector<Ref<Model>> m_Models;
        
        Ref<GraphicsPipeline> m_MeshPipeline;
        Ref<GraphicsPipeline> m_EnvPipeline;

        MaterialData *m_SelectedMaterial = nullptr;
        Model *m_SelectedModel = nullptr;

        DebugRenderData m_DebugRenderData;
        Ref<Environment> m_Environment;
        EditorData m_Data;

        std::list<std::future<Ref<Model>>> m_PendingLoadModels;

        nvrhi::BufferHandle m_DebugRenderBuffer;
        
        nvrhi::CommandListHandle m_CommandList;
        nvrhi::IDevice *m_Device = nullptr;
    };
}