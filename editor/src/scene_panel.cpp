#include "scene_panel.hpp"

#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_event.hpp"
#include "ignite/core/input/mouse_event.hpp"

ScenePanel::ScenePanel(const char *windowTitle)
    : IPanel(windowTitle)
{
    Application *app = Application::GetInstance();

    m_ViewportCamera = CreateScope<Camera>("ScenePanel-Editor Camera");
    m_ViewportCamera->CreateOrthographic(app->GetCreateInfo().Width, app->GetCreateInfo().Height, 1.0f, 0.1f, 350.0f);
}

void ScenePanel::CreateRenderTarget(nvrhi::IDevice *device, f32 width, f32 height)
{
    m_RenderTarget = CreateRef<RenderTarget>(device, width, height);
}

void ScenePanel::OnGuiRender()
{
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin(m_WindowTitle.c_str(), &m_IsOpen, windowFlags);
    ImGui::DockSpace(ImGui::GetID(m_WindowTitle.c_str()), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    ImGui::ShowDemoWindow();

    RenderHierarchy();
    RenderInspector();
    RenderViewport();
}

void ScenePanel::OnUpdate(f32 deltaTime)
{
    // update camera
    {
        m_ViewportCamera->UpdateProjectionMatrix();
        m_ViewportCamera->UpdateViewMatrix();
    }
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

    ImGui::PushID("CameraID");

    glm::vec3 cameraPosition = m_ViewportCamera->GetPosition();
    if (ImGui::DragFloat3("Position", &cameraPosition[0], 0.025f))
    {
        m_ViewportCamera->SetPosition(cameraPosition);
    }

    ImGui::Text("Camera");

    f32 cameraZoom = m_ViewportCamera->GetZoom();
    if (ImGui::DragFloat("Zoom", &cameraZoom, 0.025f))
    {
        m_ViewportCamera->SetZoom(cameraZoom);
    }
    ImGui::PopID();
    

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

void ScenePanel::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseScrolledEvent>(BIND_CLASS_EVENT_FN(ScenePanel::OnMouseScrolledEvent));
}

bool ScenePanel::OnMouseScrolledEvent(MouseScrolledEvent &event)
{
    // LOG_INFO("Zoom: {} {} ", m_Zoom, delta);

    // switch (m_ProjectionType)
    // {
    // case Type::Perspective:
    //     break;
    // case Type::Orthographic:
    //     m_Zoom -= delta;
    //     m_Zoom = glm::clamp(m_Zoom, 1.0f, 100.0f);
    //     break;
    // }

    return false;
}