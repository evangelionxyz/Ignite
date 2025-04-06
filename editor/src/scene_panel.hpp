#pragma once

#include "ipanel.hpp"
#include <imgui.h>

class ScenePanel final : public IPanel
{
public:
    ScenePanel(const char *windowTitle);
    virtual void OnGuiRender() override;
    virtual void Destroy() override;

private:

};