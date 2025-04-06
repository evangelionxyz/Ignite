#include "imgui_render_pass.hpp"

#include "ipanel.hpp"
#include "scene_panel.hpp"

#include <imgui.h>

ImGuiRenderPass::ImGuiRenderPass(DeviceManager *deviceManager)
    : ImGui_Renderer(deviceManager)
{
}

void ImGuiRenderPass::Init(Ref<ShaderFactory> shaderFactory)
{
    LoadShaders(shaderFactory);

    m_ScenePanel = new ScenePanel("Scene");
    AddPanel(m_ScenePanel);
}

void ImGuiRenderPass::Destroy()
{
    ImGui_Renderer::Destroy();

    // destroy panels
    for (IPanel *panel : m_Panels)
        delete panel;
    m_Panels.clear();
}

void ImGuiRenderPass::RenderGui()
{
    ImGui::ShowDemoWindow();
    RenderPanels();
}

void ImGuiRenderPass::AddPanel(IPanel *panel)
{
    if (panel)
        m_Panels.push_back(panel);
}

void ImGuiRenderPass::RemovePanel(IPanel *panel)
{
    m_Panels.remove(panel);
    delete panel;
}

void ImGuiRenderPass::RenderPanels()
{
    for (IPanel *panel : m_Panels)
        panel->RenderGui();
}

