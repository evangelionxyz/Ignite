#pragma once

#include "ipanel.hpp"
#include "render_target.hpp"

#include "entt/entt.hpp"
#include "ignite/core/uuid.hpp"

namespace ignite
{
    class Scene;
    class Camera;
    class Event;
    class MouseScrolledEvent;
    class MouseMovedEvent;
    class EditorLayer;

    enum CameraMode : u8
    {
        CAMERA_MODE_2D = BIT(0),
        CAMERA_MODE_FLY = BIT(1),
        CAMERA_MODE_PIVOT = BIT(2)
    };

    class ScenePanel final : public IPanel
    {
    public:
        explicit ScenePanel(const char *windowTitle, EditorLayer *editor);

        void CreateRenderTarget(nvrhi::IDevice *device, f32 width, f32 height);

        void OnUpdate(f32 deltaTime) override;
        void OnGuiRender() override;
        void RenderViewport();

        Ref<RenderTarget> GetRT() { return m_RenderTarget; }

        void OnEvent(Event &event);
        bool OnMouseScrolledEvent(MouseScrolledEvent &event);
        bool OnMouseMovedEvent(MouseMovedEvent &event);

        Camera *GetViewportCamera() { return m_ViewportCamera.get(); }

    private:
        void RenderHierarchy();
        void RenderEntityNode(entt::entity entity, UUID uuid, i32 index = 0);
        
        void RenderInspector();

        void RenderSettings();
        void UpdateCameraInput(f32 deltaTime);

        CameraMode m_CameraMode = CAMERA_MODE_2D;
        Scope<Camera> m_ViewportCamera;
        Ref<RenderTarget> m_RenderTarget;
        EditorLayer *m_Editor;

        entt::entity m_SelectedEntity = { entt::null };

        struct UIState
        {
            bool settingsWindow = true;

        } m_State;

        struct CameraData
        {
            f32 moveSpeed = 6.0f;
            const f32 maxMoveSpeed = 200.0f;
            const f32 rotationSpeed = 0.8f;
            glm::vec3 lastPosition = { 0.0f, 0.0f, 0.0f };
        } m_CameraData;

        struct ViewportData
        {
            bool isHovered = false;
            bool isFocused = false;
            f32 width = 0.0f;
            f32 height = 0.0f;
            glm::vec2 relativeMousePos = glm::vec2(0.0f);
        } m_ViewportData;

    };
}
