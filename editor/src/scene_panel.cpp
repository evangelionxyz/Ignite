#include "scene_panel.hpp"

ScenePanel::ScenePanel(const char *windowTitle)
    : IPanel(windowTitle)
{
}

void ScenePanel::OnGuiRender()
{
    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin(m_WindowTitle.c_str(), &m_IsOpen, windowFlags);
    ImGui::DockSpace(ImGui::GetID(m_WindowTitle.c_str()), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    ImGui::ShowDemoWindow();

    RenderHierarchy();
    RenderInspector();
    RenderViewport();
}

void ScenePanel::Destroy()
{
}

void ScenePanel::RenderHierarchy()
{
    ImGui::Begin("Hierarchy");
    ImGui::Text("Hierarchy");
    ImGui::End();
}

void ScenePanel::RenderInspector()
{
    ImGui::Begin("Inspector");
    ImGui::Text("Inspector");
    ImGui::End();
}

void ScenePanel::RenderViewport()
{
    ImGui::Begin("Viewport");
    ImGui::Text("Viewport");
    ImGui::End();
}
