#pragma once

#include "render_target.hpp"

#include "ipanel.hpp"
#include <imgui.h>

class Event;
class MouseScrolledEvent;
class Camera;

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

    Camera *GetViewportCamera() { return m_ViewportCamera.get(); }
    
private:
    void RenderHierarchy();
    void RenderInspector();

    Scope<Camera> m_ViewportCamera;
    Ref<RenderTarget> m_RenderTarget;
};