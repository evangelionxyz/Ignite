#include "editor_layer.hpp"
#include "scene_panel.hpp"
#include "ignite/graphics/renderer_2d.hpp"

#include <glm/glm.hpp>
#include <nvrhi/utils.h>

#include "ignite/scene/camera.hpp"
#include "ignite/graphics/texture.hpp"

#include <queue>

namespace ignite
{
    EditorLayer::EditorLayer(const std::string &name)
        : Layer(name)
    {
    }

    void EditorLayer::OnAttach()
    {
        Layer::OnAttach();

        m_DeviceManager = Application::GetDeviceManager();
        nvrhi::IDevice *device = m_DeviceManager->GetDevice();

        // write buffer with command list
        m_CommandList = device->createCommandList();
        Renderer2D::InitQuadData(m_DeviceManager->GetDevice(), m_CommandList);

        m_Texture = Texture::Create("Resources/textures/test.png");
        m_CommandList->open();
        m_Texture->Write(m_CommandList);
        m_CommandList->close();
        device->executeCommandList(m_CommandList);

        m_ScenePanel = CreateRef<ScenePanel>("Scene Panel", this);
        m_ScenePanel->CreateRenderTarget(device, 1280.0f, 720.0f);

        m_ActiveScene = CreateRef<Scene>("test scene");
        m_ScenePanel->SetActiveScene(m_ActiveScene.get());
        
        m_ActiveScene->OnStart();
    }

    void EditorLayer::OnDetach()
    {
        Layer::OnDetach();
        m_ActiveScene->OnStop();
        m_CommandList = nullptr;
    }

    void EditorLayer::OnUpdate(const f32 deltaTime)
    {
        Layer::OnUpdate(deltaTime);

        m_ActiveScene->OnUpdate(deltaTime);

        // update panels
        m_ScenePanel->OnUpdate(deltaTime);
    }

    void EditorLayer::OnEvent(Event &e)
    {
        Layer::OnEvent(e);
        m_ScenePanel->OnEvent(e);
    }

    void EditorLayer::OnRender(nvrhi::IFramebuffer *framebuffer)
    {
        Layer::OnRender(framebuffer);

        // main scene rendering
        m_CommandList->open();
        m_CommandList->clearTextureFloat(m_ScenePanel->GetRT()->texture,
            nvrhi::AllSubresources, nvrhi::Color(
                m_ScenePanel->GetRT()->clearColor.r,
                m_ScenePanel->GetRT()->clearColor.g,
                m_ScenePanel->GetRT()->clearColor.b,
                1.0f
            )
        );

        nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

        m_ActiveScene->OnRender(m_ScenePanel->GetViewportCamera(), m_ScenePanel->GetRT()->framebuffer);

        m_CommandList->close();
        m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);
    }

    void EditorLayer::OnGuiRender()
    {
        constexpr f32 TITLE_BAR_HEIGHT = 45.0f;

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::Begin("##main_dockspace", nullptr, windowFlags);
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        window->DC.LayoutType = ImGuiLayoutType_Horizontal;
        window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 minPos = viewport->Pos;
        const ImVec2 maxPos = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + TITLE_BAR_HEIGHT);
        draw_list->AddRectFilled(minPos, maxPos, IM_COL32(40, 40, 40, 255));

        // dockspace
        ImGui::SetCursorScreenPos({viewport->Pos.x, viewport->Pos.y + TITLE_BAR_HEIGHT});
        ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        {
            // scene dockspace
            m_ScenePanel->OnGuiRender();
        }

        ImGui::End(); // end dockspace
    }
}