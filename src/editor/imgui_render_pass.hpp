#pragma once

#include "core/imgui/imgui_renderer.hpp"
#include "core/types.hpp"

class ImGuiRenderPass : public ImGui_Renderer
{
public:
    ImGuiRenderPass(DeviceManager *deviceManager);
    void Init(Ref<ShaderFactory> shaderFactory);
    virtual void Destroy() override;
    virtual void BuildUI() override;

private:
    nvrhi::CommandListHandle m_CommandList;
};