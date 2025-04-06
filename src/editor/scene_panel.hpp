#pragma once

#include "ipanel.hpp"

class ScenePanel final : public IPanel
{
public:
    ScenePanel(const char *windowTitle);
    virtual void RenderGui() override;
    virtual void Destroy() override;

private:

};