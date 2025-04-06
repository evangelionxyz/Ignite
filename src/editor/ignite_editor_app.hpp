#include "core/application.hpp"
#include "scene_render_pass.hpp"
#include "imgui_render_pass.hpp"

#include "core/types.hpp"
#include "core/logger.hpp"

class EditorApp : public Application
{
public:
    EditorApp(const ApplicationCreateInfo &createInfo);
    virtual void Run() override;

private:
    void Destroy();
    Ref<SceneRenderPass> m_SceneRenderPass;
    Ref<ImGuiRenderPass> m_GuiRenderPass;

    DeviceManager *m_DeviceManager = nullptr;
    Ref<ShaderFactory> m_ShaderFactory;
    
    DeviceCreationParameters m_DeviceParams;
};