#include "imgui_render_pass.hpp"

#include <imgui.h>

ImGuiRenderPass::ImGuiRenderPass(DeviceManager *deviceManager)
    : ImGui_Renderer(deviceManager)
{
}

void ImGuiRenderPass::Init(Ref<ShaderFactory> shaderFactory)
{
    LoadShaders(shaderFactory);
}

void ImGuiRenderPass::Destroy()
{
    ImGui_Renderer::Destroy();
}

void ImGuiRenderPass::BuildUI()
{
    ImGui::ShowDemoWindow();
}
