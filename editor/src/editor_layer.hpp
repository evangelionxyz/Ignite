#pragma once

#include <nvrhi/nvrhi.h>
#include "ignite/core/layer.hpp"
#include "ignite/ignite.hpp"
#include "ignite/graphics/scene_renderer.hpp"
#include "ignite/serializer/serializer.hpp"
#include "ignite/project/project.hpp"
#include "states.hpp"
#include <future>

namespace ignite
{
    class ShaderFactory;
    class ScenePanel;
    class ContentBrowserPanel;

    class EditorLayer final : public Layer
    {
    private:
        struct EditorData
        {
            bool debugMode = false;
            bool developerMode = false;
            bool multiSelect = false;
            bool settingsWindow = false;
            bool popupNewProjectModal = false;
            bool assetRegistryWindow = false;
            bool isPickingEntity = false;
            bool takeScreenshot = false;

            uint32_t hoveredEntity = uint32_t(-1);

            ProjectInfo projectCreateInfo;

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
        bool OnMouseButtonPressed(MouseButtonPressedEvent &event);

        void OnRender(nvrhi::IFramebuffer *framebuffer) override;
        void OnGuiRender() override;

        Scene *GetActiveScene() { return m_ActiveScene.get(); }
        Project *GetActiveProject() { return m_ActiveProject.get(); }

        EditorData &GetState() { return m_Data; }

    private:
        void NewScene();
        void SaveScene();
        void SaveSceneAs();
        bool SaveScene(const std::filesystem::path &filepath) const;
        void OpenScene();
        bool OpenScene(const std::filesystem::path &filepath);
        
        void SaveProject();
        void SaveProjectAs();
        void OpenProject();
        bool OpenProject(const std::filesystem::path &filepath);

        void OnScenePlay();
        void OnSceneStop();
        void OnSceneSimulate();

        void SettingsUI();

        Ref<ScenePanel> m_ScenePanel;
        Ref<ContentBrowserPanel> m_ContentBrowserPanel;
        SceneRenderer m_SceneRenderer;

        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;
        Ref<Project> m_ActiveProject;
        EditorData m_Data;

        std::filesystem::path m_CurrentSceneFilePath;

        nvrhi::BufferHandle m_DebugRenderBuffer;
        
        nvrhi::CommandListHandle m_CommandList;
        nvrhi::StagingTextureHandle m_EntityIDStagingTexture;
        nvrhi::StagingTextureHandle m_ScreenshotStagingTexture;
            
        nvrhi::IDevice *m_Device = nullptr;

        friend class ScenePanel;
    };
}