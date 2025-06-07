#include "sound_panel.hpp"

namespace ignite
{
    SoundPanel::SoundPanel(const char *windowTitle)
        : IPanel(windowTitle)
    {
    }

    void SoundPanel::OnGuiRender()
    {
        ImGui::Begin("Sound");

        ImGui::End();
    }

    void SoundPanel::OnUpdate(f32 deltaTime)
    {
    }
}
