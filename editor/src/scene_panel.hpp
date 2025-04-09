#pragma once

#include "render_target.hpp"

#include "ipanel.hpp"
#include <imgui.h>

class ScenePanel final : public IPanel
{
public:
    explicit ScenePanel(const char *windowTitle);

    void CreateRenderTarget(nvrhi::IDevice *device, f32 width, f32 height);

    void OnGuiRender() override;
    void RenderViewport();
    void Destroy() override;

    Ref<RenderTarget> GetRT() { return m_RenderTarget; }
    
private:

    void RenderHierarchy();
    void RenderInspector();

    Ref<RenderTarget> m_RenderTarget;
};