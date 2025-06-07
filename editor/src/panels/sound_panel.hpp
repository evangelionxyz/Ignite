#pragma once

#include "ipanel.hpp"

namespace ignite
{
    class SoundPanel : public IPanel
    {
    public:
        SoundPanel(const char *windowTitle);
        
        virtual void OnGuiRender() override;
        virtual void OnUpdate(f32 deltaTime) override;
    };
}
