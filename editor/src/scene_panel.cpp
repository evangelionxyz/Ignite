#include "scene_panel.hpp"

#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_event.hpp"
#include "ignite/core/input/mouse_event.hpp"

#include "entt/entt.hpp"

namespace ignite
{
    ScenePanel::ScenePanel(const char *windowTitle)
        : IPanel(windowTitle)
    {
        Application *app = Application::GetInstance();

        m_ViewportCamera = CreateScope<Camera>("ScenePanel-Editor Camera");
        m_ViewportCamera->CreateOrthographic(app->GetCreateInfo().width, app->GetCreateInfo().height, 8.0f, 0.1f, 350.0f);
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

        ImGui::DragFloat3("Position", &m_ViewportCamera->position[0], 0.025f);

        ImGui::Text("Camera");

        ImGui::DragFloat("Zoom", &m_ViewportCamera->zoom, 0.025f);
        ImGui::PopID();

        ImGui::ColorEdit3("Clear color", &m_RenderTarget->clearColor[0]);
        ImGui::End();
    }

    void ScenePanel::RenderViewport()
    {
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("Viewport", nullptr, windowFlags);

        const ImGuiWindow *window = ImGui::GetCurrentWindow();
        m_ViewportData.isFocused = ImGui::IsWindowFocused();
        m_ViewportData.isHovered = ImGui::IsWindowHovered();
        m_ViewportData.width = window->Size.x;
        m_ViewportData.height = window->Size.y;

        m_RenderTarget->width = window->Size.x;
        m_RenderTarget->height = window->Size.y;

        m_ViewportCamera->SetSize(m_RenderTarget->width, m_RenderTarget->height);

        const ImTextureID imguiTex = reinterpret_cast<ImTextureID>(m_RenderTarget->texture.Get());
        ImGui::Image(imguiTex, window->Size);

        ImGui::End();
    }

    void ScenePanel::OnEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_CLASS_EVENT_FN(ScenePanel::OnMouseScrolledEvent));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_CLASS_EVENT_FN(ScenePanel::OnMouseMovedEvent));
    }

    bool ScenePanel::OnMouseScrolledEvent(MouseScrolledEvent &event)
    {
        if (m_ViewportData.isHovered)
        {
            const f32 dx = event.GetXOffset(), dy = event.GetYOffset();
            switch (m_ViewportCamera->projectionType)
            {
                case Camera::Type::Perspective:
                {
                    break;
                }
                case Camera::Type::Orthographic:
                    default:
                {
                    const f32 scaleFactor = std::max(m_ViewportData.width, m_ViewportData.height);
                    const f32 zoomSqrt = glm::sqrt(m_ViewportCamera->zoom * m_ViewportCamera->zoom);

                    if (Input::IsKeyPressed(KEY_LEFT_ALT))
                    {
                        m_ViewportCamera->zoom -= dy * zoomSqrt * 0.1f;
                        m_ViewportCamera->zoom = glm::clamp(m_ViewportCamera->zoom, 1.0f, 100.0f);
                    }
                    else
                    {
                        const f32 moveSpeed = 50.0f / scaleFactor;
                        const f32 mulFactor = moveSpeed * m_ViewportCamera->GetAspectRatio() * zoomSqrt;
                        if (Input::IsKeyPressed(KEY_LEFT_SHIFT))
                        {
                            m_ViewportCamera->position += m_ViewportCamera->GetRightDirection() * dy * mulFactor;
                            m_ViewportCamera->position -= m_ViewportCamera->GetUpDirection() * dx * mulFactor;
                        }
                        else
                        {
                            m_ViewportCamera->position += m_ViewportCamera->GetRightDirection() * dx * mulFactor;
                            m_ViewportCamera->position -= m_ViewportCamera->GetUpDirection() * dy * mulFactor;
                        }
                    }
                    break;
                }
            }

            if (m_ViewportData.isFocused)
            {
            }
        }

        return false;
    }

    bool ScenePanel::OnMouseMovedEvent(MouseMovedEvent &event)
    {
        if (m_ViewportData.isFocused && m_ViewportData.isHovered)
        {

        }

        return false;
    }
}