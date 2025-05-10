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
    EditorLayer::EditorLayer(const std::string &name)
        : Layer(name)
    {
    }

    void EditorLayer::OnAttach()
    {
        Layer::OnAttach();

        m_Device = Application::GetDeviceManager()->GetDevice();
        m_CommandList = m_Device->createCommandList();

        // create mesh graphics pipeline
        {
            GraphicsPipelineParams params;
            params.enableBlend = true;
            params.depthWrite = true;
            params.depthTest = true;
            params.recompileShader = true;
            params.cullMode = nvrhi::RasterCullMode::Front;
            params.vertexShaderFilepath = "default_mesh.vertex.hlsl";
            params.pixelShaderFilepath = "default_mesh.pixel.hlsl";

            auto attributes = VertexMesh::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());
            pci.bindingLayoutDesc = VertexMesh::GetBindingLayoutDesc();

            m_MeshPipeline = GraphicsPipeline::Create(m_Device, params, pci);
        }

        // create skybox graphics pipeline
        {
            GraphicsPipelineParams params;
            params.enableBlend = true;
            params.depthWrite = true;
            params.depthTest = true;
            params.recompileShader = true;
            params.cullMode = nvrhi::RasterCullMode::Front;
            params.comparison = nvrhi::ComparisonFunc::Always;
            params.vertexShaderFilepath = "skybox.vertex.hlsl";
            params.pixelShaderFilepath = "skybox.pixel.hlsl";

            auto attribute = Environment::GetAttribute();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = &attribute;
            pci.attributeCount = 1;
            pci.bindingLayoutDesc = Environment::GetBindingLayoutDesc();

            m_EnvPipeline = GraphicsPipeline::Create(m_Device, params, pci);

            // create env
            // order +X, -X, +Y, -Y, +Z, -Z.
            EnvironmentCreateInfo eci;
            eci.rightFilepath = "resources/skybox/right.jpg";
            eci.leftFilepath = "resources/skybox/left.jpg";
            eci.topFilepath = "resources/skybox/top.jpg";
            eci.bottomFilepath = "resources/skybox/bottom.jpg";
            eci.frontFilepath = "resources/skybox/front.jpg";
            eci.backFilepath = "resources/skybox/back.jpg";

            m_Env = Environment(m_Device, m_CommandList, eci, m_EnvPipeline->GetBindingLayout());
        }


        m_Helmet = Model::Create(m_Device, m_MeshPipeline->GetBindingLayout(), "resources/models/DamagedHelmet.gltf");
        m_Scene = Model::Create(m_Device, m_MeshPipeline->GetBindingLayout(), "resources/scene.glb");

        // write buffer with command list
        Renderer2D::InitQuadData(m_Device, m_CommandList);

        m_CommandList->open();
        m_Helmet->WriteBuffer(m_CommandList);
        m_Scene->WriteBuffer(m_CommandList);
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
    }

    void EditorLayer::OnUpdate(const f32 deltaTime)
    {
        Layer::OnUpdate(deltaTime);

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

        m_Helmet->OnUpdate(deltaTime);
        m_Scene->OnUpdate(deltaTime);

       /* static float t = 0.0f;
        t += deltaTime * 0.5f;

        m_Helmet->transform = glm::rotate(t, glm::vec3 { 0.0f, 1.0f, 0.0f });*/

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

        // create pipelines
        m_EnvPipeline->Create(m_Device, viewportFramebuffer);
        m_MeshPipeline->Create(m_Device, viewportFramebuffer);

        // main scene rendering
        m_CommandList->open();

        // clear main framebuffer color attachment
        nvrhi::utils::ClearColorAttachment(m_CommandList, mainFramebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

        m_ScenePanel->GetRT()->ClearColorAttachment(m_CommandList);

        float farDepth = 1.0f; // LessOrEqual
        m_CommandList->clearDepthStencilTexture(m_ScenePanel->GetRT()->GetDepthAttachment(), nvrhi::AllSubresources, true, farDepth, true, 0);

        m_ActiveScene->OnRenderRuntimeSimulate(m_ScenePanel->GetViewportCamera(), viewportFramebuffer);

        // render environment
        m_Env.Render(m_CommandList, viewportFramebuffer, m_EnvPipeline->GetHandle(), m_ScenePanel->GetViewportCamera());

        // render objects
        m_Helmet->Render(m_CommandList, viewportFramebuffer, m_MeshPipeline->GetHandle(), m_ScenePanel->GetViewportCamera(), &m_Env);
        m_Scene->Render(m_CommandList, viewportFramebuffer, m_MeshPipeline->GetHandle(), m_ScenePanel->GetViewportCamera(), &m_Env);


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

            ImGui::Begin("Lighting & Material");
            
            static glm::vec2 sunAngles = { 45.0f, 45.0f }; // pitch (elevation), yaw (azimuth)

            if (ImGui::DragFloat2("Sun Angles (Pitch/Yaw)", &sunAngles.x, 0.25f))
            {
                float pitch = glm::radians(sunAngles.x); // elevation
                float yaw = glm::radians(sunAngles.y); // azimuth

                glm::vec3 dir;
                dir.x = cos(pitch) * sin(yaw);
                dir.y = sin(pitch);
                dir.z = cos(pitch) * cos(yaw);

                m_Env.dirLight.direction = glm::vec4(glm::normalize(dir), 0.0f);
            }

            ImGui::ColorEdit4("Color", &m_Env.dirLight.color.x);
            ImGui::DragFloat("Intensity", &m_Env.dirLight.intensity, 0.0005f, 0.01f, 100.0f);
            ImGui::DragFloat("Angular Size", &m_Env.dirLight.angularSize, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Ambient", &m_Env.dirLight.ambientIntensity, 0.005f, 0.01f, 100.0f);
            
            ImGui::DragFloat("Exposure", &m_Env.params.exposure, 0.005f, 0.1f, 10.0f);
            ImGui::DragFloat("Gamma", &m_Env.params.gamma, 0.005f, 0.1f, 10.0f);

            if (m_SelectedMaterial)
            {
                ImGui::Separator();

                ImGui::ColorEdit4("Base Color", &m_SelectedMaterial->baseColor.x);
                ImGui::ColorEdit4("Diffuse Color", &m_SelectedMaterial->diffuseColor.x);
                ImGui::DragFloat("Metallic", &m_SelectedMaterial->metallic, 0.005f, 0.0f, 100.0f);
                ImGui::DragFloat("Rougness", &m_SelectedMaterial->roughness, 0.005f, 0.0f, 100.0f);
                ImGui::DragFloat("Emissive", &m_SelectedMaterial->emissive, 0.005f, 0.0f, 100.0f);
            }

            ImGui::End();

            ImGui::Begin("Models");

            ImGui::PushID("helmet");
            for (auto &mesh : m_Helmet->GetMeshes())
            {
                TraverseMeshes(m_Helmet.get(), mesh, 0);
            }
            ImGui::PopID();

            ImGui::Separator();
            ImGui::PushID("scene");
            for (auto &mesh : m_Scene->GetMeshes())
            {
                TraverseMeshes(m_Scene.get(), mesh, 0);
            }
            ImGui::PopID();

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

    void EditorLayer::TraverseMeshes(Model *model, Ref<Mesh> mesh, int traverseIndex)
    {
        if (mesh->parentID != -1 && traverseIndex == 0)
            return;

        ImGuiTreeNodeFlags flags = (mesh->children.empty()) ? ImGuiTreeNodeFlags_Leaf : 0 | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        bool opened = ImGui::TreeNodeEx(mesh->name.c_str(), flags, mesh->name.c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_SelectedMaterial = &mesh->material.data;
        }

        if (opened)
        {
            for (i32 child : mesh->children)
            {
                TraverseMeshes(model, model->GetMeshes()[child], ++traverseIndex);
            }

            ImGui::TreePop();
        }
    }

}
