#pragma once

#include "ipanel.hpp"

#include "ignite/graphics/render_target.hpp"
#include "ignite/scene/entity.hpp"
#include "ignite/core/uuid.hpp"
#include "ignite/core/base.hpp"
#include "ignite/core/types.hpp"

#include <string>
#include <glm/fwd.hpp>
#include <nvrhi/nvrhi.h>

#include "ignite/imgui/gizmo.hpp"

namespace ignite
{
    class Scene;
    class Camera;
    class Event;
    class MouseScrolledEvent;
    class MouseMovedEvent;
    class JoystickConnectionEvent;
    class EditorLayer;

    class ScenePanel final : public IPanel
    {
    public:
        explicit ScenePanel(const char *windowTitle, EditorLayer *editor);
        
        void SetActiveScene(Scene *scene, bool reset = false);
        void CreateRenderTarget(nvrhi::IDevice *device);

        void OnUpdate(f32 deltaTime) override;
        void OnGuiRender() override;
        void RenderViewport();

        Ref<RenderTarget> GetRT() const { return m_RenderTarget; }

        void OnEvent(Event &event);
        bool OnMouseScrolledEvent(MouseScrolledEvent &event);
        bool OnMouseMovedEvent(MouseMovedEvent &event);
        bool OnJoystickConnectionEvent(JoystickConnectionEvent &event);

        void SetGizmoOperation(ImGuizmo::OPERATION op);
        void SetGizmoMode(ImGuizmo::MODE mode);
        bool IsGizmoBeingUse() { return m_Data.isGizmoBeingUse; }
        bool IsViewportHovered() { return m_ViewportData.isHovered; }
        
        Camera *GetViewportCamera() const { return m_ViewportCamera.get(); }

        const glm::vec2 &GetViewportMousePos() const { return m_ViewportData.mousePos; }

        void RenderHierarchy();
        Entity ShowEntityContextMenu();
        void RenderEntityNode(Entity entity, UUID uuid, i32 index = 0);
        
        void RenderInspector();

        void CameraSettingsUI();
        void UpdateCameraInput(f32 deltaTime);
        void DestroyEntity(Entity entity);
        
        void ClearMultiSelectEntity();

        Entity SetSelectedEntity(Entity entity);
        Entity GetSelectedEntity();
        
        void DuplicateSelectedEntity();

        template<typename T, typename UIFunction>
        void RenderComponent(const std::string &name, Entity entity, UIFunction uiFunction, bool allowedToRemove = true);

    private:
        struct Data
        {
            bool settingsWindow = true;
            bool isGizmoBeingUse = false;
        } m_Data;

        Scope<Camera> m_ViewportCamera;
        Ref<RenderTarget> m_RenderTarget;
        EditorLayer *m_Editor;

        Scene *m_Scene = nullptr;
        Gizmo m_Gizmo;

        std::vector<UUID> m_SelectedEntityIDs;

        Entity m_SelectedEntity{};

        static UUID m_TrackingSelectedEntity;

        struct CameraData
        {
            f32 moveSpeed = 6.0f;
            const f32 maxMoveSpeed = 500.0f;
            const f32 rotationSpeed = 0.8f;
            glm::vec3 lastPosition = { 0.0f, 0.0f, 0.0f };
        } m_CameraData;

        struct ViewportData
        {
            bool isHovered = false;
            bool isFocused = false;
            Rect rect = { 0, 0, 1, 1 };
            glm::vec2 mousePos = glm::vec2(0.0f);
        } m_ViewportData;

        
    };
}
