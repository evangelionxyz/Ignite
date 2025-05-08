#include "editor_layer.hpp"
#include "panels/scene_panel.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/mesh_factory.hpp"

#include <glm/glm.hpp>
#include <nvrhi/utils.h>

#include "ignite/core/command.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/graphics/texture.hpp"

namespace ignite
{
    struct MeshRenderData
    {
        nvrhi::ShaderHandle vertexShader;
        nvrhi::ShaderHandle pixelShader;
        nvrhi::InputLayoutHandle inputLayout;
        nvrhi::GraphicsPipelineHandle pipeline;
    };

    MeshRenderData *meshData = nullptr;

    EditorLayer::EditorLayer(const std::string &name)
        : Layer(name)
    {
    }

    void EditorLayer::OnAttach()
    {
        Layer::OnAttach();

        m_Device = Application::GetDeviceManager()->GetDevice();

        meshData = new MeshRenderData();

        // create shaders
        VPShader *shaders = Renderer::GetDefaultShader("default_mesh");
        meshData->vertexShader = shaders->vertex;
        meshData->pixelShader = shaders->pixel;
        LOG_ASSERT(meshData->vertexShader && meshData->pixelShader, "Failed to create shaders");

        const auto attributes = VertexMesh::GetAttributes();
        meshData->inputLayout = m_Device->createInputLayout(attributes.data(), attributes.size(), meshData->vertexShader);
        LOG_ASSERT(meshData->inputLayout, "Failed to create input layout");

        m_Model = CreateRef<Model>(m_Device, "resources/scene.glb");

        // write buffer with command list
        m_CommandList = m_Device->createCommandList();
        Renderer2D::InitQuadData(m_Device, m_CommandList);

        m_CommandList->open();
        m_Model->WriteBuffer(m_CommandList);
        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);

        m_ScenePanel = CreateRef<ScenePanel>("Scene Panel", this);
        m_ScenePanel->CreateRenderTarget(m_Device);

        NewScene();
    }

    void EditorLayer::OnDetach()
    {
        Layer::OnDetach();
        m_CommandList = nullptr;
        m_CommandList = nullptr;

        if (meshData)
        {
            delete meshData;
        }
    }

    void EditorLayer::OnUpdate(const f32 deltaTime)
    {
        Layer::OnUpdate(deltaTime);

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

        m_Model->OnUpdate(deltaTime);

        switch (m_Data.sceneState)
        {
        case State_ScenePlay:
        {
            m_ActiveScene->OnUpdateRuntimeSimulate(deltaTime);
            break;
        }
        case State_SceneEdit:
        {
            m_ActiveScene->OnUpdateEdit(deltaTime);
            break;
        }
        }

        // update panels
        m_ScenePanel->OnUpdate(deltaTime);
    }

    void EditorLayer::OnEvent(Event &e)
    {
        Layer::OnEvent(e);
        m_ScenePanel->OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_CLASS_EVENT_FN(EditorLayer::OnKeyPressedEvent));
    }

    bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent &event)
    {
        bool control = Input::IsKeyPressed(KEY_LEFT_CONTROL);
        bool shift = Input::IsKeyPressed(KEY_LEFT_SHIFT);

        switch (event.GetKeyCode())
        {
        case Key::T:
        {
            if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
            break;
        }
        case Key::R:
        {
            if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::ROTATE);
            break;
        }
        case Key::S:
        {
            if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::SCALE);
            break;
        }
        case Key::F5:
        {
            (m_Data.sceneState == State_SceneEdit || m_Data.sceneState == State_SceneSimulate)
                ? OnScenePlay()
                : OnSceneStop();

            break;
        }
        case Key::F6:
        {
            (m_Data.sceneState == State_SceneEdit || m_Data.sceneState == State_ScenePlay)
                ? OnScenePlay()
                : OnSceneStop();
            break;
        }
        case Key::D:
        {
            if (control)
                m_ScenePanel->DuplicateSelectedEntity();
            break;
        }
        case Key::Z:
        {
            if (control)
            {
                if (shift)
                    Application::GetCommandManager()->Redo();
                else
                    Application::GetCommandManager()->Undo();
            }
            break;
        }
        }
        return false;
    }

    void EditorLayer::OnRender(nvrhi::IFramebuffer *mainFramebuffer)
    {
        Layer::OnRender(mainFramebuffer);

        // create render target framebuffer
        const uint32_t &backBufferIndex = Application::GetDeviceManager()->GetCurrentBackBufferIndex();
        static uint32_t backBufferCount = Application::GetDeviceManager()->GetBackBufferCount();
        const nvrhi::Viewport &mainViewport = mainFramebuffer->getFramebufferInfo().getViewport();
        uint32_t width = (uint32_t)mainViewport.width();
        uint32_t height = (uint32_t)mainViewport.height();

        // m_ScenePanel->GetRT()->CreateFramebuffers(backBufferCount, backBufferIndex, { width, height });
        m_ScenePanel->GetRT()->CreateSingleFramebuffer({ width, height });

        nvrhi::IFramebuffer *viewportFramebuffer = m_ScenePanel->GetRT()->GetCurrentFramebuffer();
        nvrhi::Viewport viewport = viewportFramebuffer->getFramebufferInfo().getViewport();

        // mesh pipeline
        if (meshData->pipeline == nullptr)
        {
            // create graphics pipeline
            nvrhi::BlendState blendState;
            blendState.targets[0].setBlendEnable(true);

            auto depthStencilState = nvrhi::DepthStencilState()
                .setDepthWriteEnable(true)
                .setDepthTestEnable(true)
                .setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual); // use 1.0 for far depth

            auto rasterState = nvrhi::RasterState()
                .setCullFront()
                .setFrontCounterClockwise(false)
                .setMultisampleEnable(false);

            auto renderState = nvrhi::RenderState()
                .setRasterState(rasterState)
                .setDepthStencilState(depthStencilState)
                .setBlendState(blendState);

            auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
                .setVertexShader(meshData->vertexShader)
                .setPixelShader(meshData->pixelShader)
                .setInputLayout(meshData->inputLayout)
                .addBindingLayout(m_Model->bindingLayout)
                .setRenderState(renderState)
                .setPrimType(nvrhi::PrimitiveType::TriangleList);

            // create with viewportFramebuffer (the same framebuffer to render)
            meshData->pipeline = m_Device->createGraphicsPipeline(pipelineDesc, viewportFramebuffer);
            LOG_ASSERT(meshData->pipeline, "Failed to create graphics pipeline");
        }

        // main scene rendering
        m_CommandList->open();

        // clear main framebuffer color attachment
        nvrhi::utils::ClearColorAttachment(m_CommandList, mainFramebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

        m_ScenePanel->GetRT()->ClearColorAttachment(m_CommandList);

        float farDepth = 1.0f; // LessOrEqual
        m_CommandList->clearDepthStencilTexture(m_ScenePanel->GetRT()->GetDepthAttachment(), nvrhi::AllSubresources, true, farDepth, true, 0);

        m_ActiveScene->OnRenderRuntimeSimulate(m_ScenePanel->GetViewportCamera(), viewportFramebuffer);
        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);


        m_CommandList->open();
        m_Model->Render(m_CommandList, m_ScenePanel->GetRT()->GetCurrentFramebuffer(), meshData->pipeline, m_ScenePanel->GetViewportCamera());
        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void EditorLayer::OnGuiRender()
    {
        constexpr f32 TITLE_BAR_HEIGHT = 45.0f;

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        f32 totalTitlebarHeight = viewport->Pos.y + TITLE_BAR_HEIGHT;

        ImGui::Begin("##main_dockspace", nullptr, windowFlags);
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        window->DC.LayoutType = ImGuiLayoutType_Horizontal;
        window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 minPos = viewport->Pos;
        const ImVec2 maxPos = ImVec2(viewport->Pos.x + viewport->Size.x, totalTitlebarHeight);
        draw_list->AddRectFilled(minPos, maxPos, IM_COL32(30, 30, 30, 255));

        // play stop and simulate button
        ImVec2 buttonSize = {40.0f, 25.0f};
        ImVec2 buttonCursorPos = { viewport->Pos.x, totalTitlebarHeight / 2.0f - buttonSize.y / 2.0f};
        ImGui::SetCursorScreenPos(buttonCursorPos);

        bool sceneStatePlaying = m_Data.sceneState == State_ScenePlay;
        if (ImGui::Button(sceneStatePlaying ? "Stop" : "Play", buttonSize))
        {
            if (sceneStatePlaying)
                OnSceneStop();
            else 
                OnScenePlay();
        }

        // dockspace
        ImGui::SetCursorScreenPos({viewport->Pos.x, totalTitlebarHeight});
        ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        {
            // scene dockspace
            m_ScenePanel->OnGuiRender();

            ImGui::Begin("Meshes List");

            for (auto &mesh : m_Model->GetMeshes())
            {
                TraverseMeshes(mesh, 0);
            }

            ImGui::End();
        }

        ImGui::End(); // end dockspace
    }

    void EditorLayer::NewScene()
    {
        // create editor scene
        m_EditorScene = CreateRef<Scene>("test scene");

        // pass to active scene
        m_ActiveScene = m_EditorScene;
        m_ScenePanel->SetActiveScene(m_ActiveScene.get(), true); // reset selected entity
    }

    void EditorLayer::OnScenePlay()
    {
        m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::NONE);

        if (m_Data.sceneState != State_SceneEdit)
            OnSceneStop();

        m_Data.sceneState = State_ScenePlay;

        // copy initial components to new scene
        m_ActiveScene = SceneManager::Copy(m_EditorScene);
        m_ActiveScene->OnStart();

        m_ScenePanel->SetActiveScene(m_ActiveScene.get());
    }

    void EditorLayer::OnSceneStop()
    {
        m_Data.sceneState = State_SceneEdit;
        
        m_ActiveScene->OnStop();
        m_ActiveScene = m_EditorScene;

        m_ScenePanel->SetActiveScene(m_EditorScene.get());
    }

    void  EditorLayer::OnSceneSimulate()
    {
        if (m_Data.sceneState != State_SceneEdit)
            OnSceneStop();

        m_Data.sceneState = State_SceneSimulate;
    }

    void EditorLayer::TraverseMeshes(Ref<Mesh> mesh, int traverseIndex)
    {
        if (mesh->parentID != -1 && traverseIndex == 0)
            return;

        ImGuiTreeNodeFlags flags = (mesh->children.empty()) ? ImGuiTreeNodeFlags_Leaf : 0 | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        bool opened = ImGui::TreeNodeEx(mesh->name.c_str(), flags, mesh->name.c_str());

        if (opened)
        {
            for (i32 child : mesh->children)
            {
                TraverseMeshes(m_Model->GetMeshes()[child], ++traverseIndex);
            }

            ImGui::TreePop();
        }
    }

}
