#include "scene_panel.hpp"

ScenePanel::ScenePanel(const char *windowTitle)
    : IPanel(windowTitle)
{
}

void ScenePanel::OnGuiRender()
{
    ImGui::ShowDemoWindow();

    ImGui::Begin(m_WindowTitle.c_str(), &m_IsOpen);
    ImGui::End();
}

void ScenePanel::Destroy()
{
}