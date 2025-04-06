#pragma once

#include "core/imgui/imgui_renderer.hpp"
#include "core/types.hpp"

#include "ipanel.hpp"
#include "scene_panel.hpp"

class ImGuiRenderPass : public ImGui_Renderer
{
public:
    ImGuiRenderPass(DeviceManager *deviceManager);
    void Init(Ref<ShaderFactory> shaderFactory);

    virtual void Destroy() override;
    virtual void RenderGui() override;

    void AddPanel(IPanel *panel);
    void RemovePanel(IPanel *panel);
    
    void RenderPanels();

private:
    ScenePanel *m_ScenePanel = nullptr;
    std::list<IPanel *> m_Panels;
};