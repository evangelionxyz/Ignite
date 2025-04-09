#include "scene_panel.hpp"

ScenePanel::ScenePanel(const char *windowTitle)
    : IPanel(windowTitle)
{
}

void ScenePanel::CreateRenderTarget(nvrhi::IDevice *device, f32 width, f32 height)
{
    m_RenderTarget = CreateRef<RenderTarget>(device, width, height);
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

    ImGui::ColorEdit3("Clear color", &m_RenderTarget->clearColor[0]);

    ImGui::End();
}

void ScenePanel::RenderViewport()
{
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("Viewport", nullptr, windowFlags);

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    m_RenderTarget->width = window->Size.x;
    m_RenderTarget->height = window->Size.y;

    ImTextureID imguiTex = (ImTextureID)m_RenderTarget->texture.Get();
   
    ImGui::Image(imguiTex, window->Size);
    
    ImGui::End();
}
