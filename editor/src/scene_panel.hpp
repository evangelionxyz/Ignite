#pragma once

#include "ipanel.hpp"
#include "render_target.hpp"

namespace ignite
{
    class Scene;
    class Camera;
    class Event;
    class MouseScrolledEvent;
    class MouseMovedEvent;

    enum CameraMode : u8
    {
        CAMERA_MODE_2D = BIT(0),
        CAMERA_MODE_FLY = BIT(1),
        CAMERA_MODE_PIVOT = BIT(2)
    };

    class ScenePanel final : public IPanel
    {
    public:
        explicit ScenePanel(const char *windowTitle);

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
        void RenderInspector();

        CameraMode m_CameraMode = CAMERA_MODE_2D;

        Scope<Camera> m_ViewportCamera;
        Ref<RenderTarget> m_RenderTarget;

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
