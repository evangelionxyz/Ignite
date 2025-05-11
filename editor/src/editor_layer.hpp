#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/graphics/lighting.hpp"
#include "ignite/serializer/serializer.hpp"

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
            bool settingsWindow = false;

            State sceneState = State::SceneEdit;
            nvrhi::RasterFillMode rasterFillMode = nvrhi::RasterFillMode::Solid;
            nvrhi::RasterCullMode rasterCullMode = nvrhi::RasterCullMode::Front;
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
        void SaveScene();
        void SaveSceneAs();
        bool SaveScene(const std::filesystem::path &filepath);
        void OpenScene();
        bool OpenScene(const std::filesystem::path &filepath);
        
        void NewProject();
        void SaveProject();
        void SaveProject(const std::filesystem::path &filepath);
        void SaveProjectAs();
        void OpenProject();
        void OpenProject(const std::filesystem::path &filepath);

        void OnScenePlay();
        void OnSceneStop();
        void OnSceneSimulate();

        void SettingsUI();

        void TraverseMeshes(Model *model, Ref<Mesh> mesh, int traverseIndex = 0);

        // temporary
        void LoadModel(const std::string &filepath);

        Ref<ScenePanel> m_ScenePanel;
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;
        
        Ref<GraphicsPipeline> m_MeshPipeline;

        Ref<GraphicsPipeline> m_EnvPipeline;
        std::vector<Ref<Model>> m_Models;

        MaterialData *m_SelectedMaterial = nullptr;
        Model *m_SelectedModel = nullptr;

        Ref<Environment> m_Environment;
        DebugRenderData m_DebugRenderData;
        EditorData m_Data;

        std::filesystem::path m_CurrentSceneFilePath;

        std::list<std::future<Ref<Model>>> m_PendingLoadModels;

        nvrhi::BufferHandle m_DebugRenderBuffer;
        
        nvrhi::CommandListHandle m_CommandList;
        nvrhi::IDevice *m_Device = nullptr;
    };
}