#pragma once

#include "ipanel.hpp"
#include <imgui.h>

class ScenePanel final : public IPanel
{
public:
    explicit ScenePanel(const char *windowTitle);

    void OnGuiRender() override;
     void Destroy() override;
private:

    void RenderHierarchy();
    void RenderInspector();
    void RenderViewport();
};