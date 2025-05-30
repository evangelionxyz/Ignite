#include "editor_layer.hpp"
#include "panels/scene_panel.hpp"
#include "ignite/core/platform_utils.hpp"
#include "ignite/core/command.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/graphics/texture.hpp"
#include "ignite/animation/animation_system.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/mesh_factory.hpp"
#include "ignite/imgui/gui_function.hpp"
#include "ignite/graphics/model.hpp"
#include "ignite/asset/asset.hpp"
#include "ignite/asset/asset_importer.hpp"

#include <glm/glm.hpp>
#include <nvrhi/utils.h>

#ifdef _WIN32
#   include <dwmapi.h>
#   include <ShellScalingApi.h>
#endif


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

        {
            // create debug render
            nvrhi::BufferDesc dbDesc;
            dbDesc.byteSize = sizeof(DebugRenderData);
            dbDesc.isConstantBuffer = true;
            dbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
            dbDesc.keepInitialState = true;

            m_DebugRenderBuffer = m_Device->createBuffer(dbDesc);
            LOG_ASSERT(m_DebugRenderBuffer, "Failed to create debug render constant buffer!");
        }

        // create mesh graphics pipeline
        {
            GraphicsPipelineParams params;
            params.enableBlend = true;
            params.depthWrite = true;
            params.depthTest = true;
            params.recompileShader = true;
            params.fillMode = nvrhi::RasterFillMode::Solid;
            params.cullMode = nvrhi::RasterCullMode::Front;

            auto attributes = VertexMesh::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());
            pci.bindingLayoutDesc = VertexMesh::GetBindingLayoutDesc();

            m_MeshPipeline = GraphicsPipeline::Create(params, &pci);
            m_MeshPipeline->AddShader("default_mesh.vertex.hlsl", nvrhi::ShaderType::Vertex)
                .AddShader("default_mesh.pixel.hlsl", nvrhi::ShaderType::Pixel)
                .CompileShaders();
        }

        // create skybox graphics pipeline
        {
            GraphicsPipelineParams params;
            params.enableBlend = true;
            params.depthWrite = true;
            params.depthTest = true;
            params.recompileShader = false;
            params.cullMode = nvrhi::RasterCullMode::Front;
            params.comparison = nvrhi::ComparisonFunc::Always;

            auto attribute = Environment::GetAttribute();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = &attribute;
            pci.attributeCount = 1;
            pci.bindingLayoutDesc = Environment::GetBindingLayoutDesc();

            m_EnvPipeline = GraphicsPipeline::Create(params, &pci);
            m_EnvPipeline->AddShader("skybox.vertex.hlsl", nvrhi::ShaderType::Vertex)
                .AddShader("skybox.pixel.hlsl", nvrhi::ShaderType::Pixel)
                .CompileShaders();

            // create env
            EnvironmentCreateInfo envCI;
            envCI.device = m_Device;
            envCI.commandList = m_CommandList;

            m_Environment = Environment::Create(envCI);
            m_Environment->LoadTexture("resources/hdr/rogland_clear_night_4k.hdr", m_EnvPipeline->GetBindingLayout());
            m_Environment->WriteTexture();

            m_Environment->SetSunDirection(50.0f, -27.0f);
        }

        // write buffer with command list
        Renderer2D::InitQuadData(m_Device, m_CommandList);

        m_ScenePanel = CreateRef<ScenePanel>("Scene Panel", this);
        m_ScenePanel->CreateRenderTarget(m_Device);

        NewScene();
    }

    void EditorLayer::OnDetach()
    {
        Layer::OnDetach();
        m_CommandList = nullptr;
    }

    void EditorLayer::OnUpdate(f32 deltaTime)
    {
        Layer::OnUpdate(deltaTime);

        ModelImporter::SyncMainThread(m_CommandList, m_Device);

        float timeInSeconds = static_cast<float>(glfwGetTime());

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

        for (auto &model : m_Models)
        {
            if (!model)
                continue;

            AnimationSystem::UpdateAnimation(model, timeInSeconds);
            // model->OnUpdate(deltaTime);
        }

        switch (m_Data.sceneState)
        {
            case State::ScenePlay:
            {
                m_ActiveScene->OnUpdateRuntimeSimulate(deltaTime);
                break;
            }
            case State::SceneEdit:
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

        bool imguiWantTextInput = ImGui::GetIO().WantTextInput;
        if (imguiWantTextInput)
            return false;

        switch (event.GetKeyCode())
        {
            case Key::S:
            {
                if (control)
                {
                    if (shift)
                        SaveSceneAs();
                    else
                        SaveScene();
                }
                break;
            }
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
            case Key::E:
            {
                if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                    m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::SCALE);
                break;
            }
            case Key::F5:
            {
                (m_Data.sceneState == State::SceneEdit || m_Data.sceneState == State::SceneSimulate)
                    ? OnScenePlay()
                    : OnSceneStop();

                break;
            }
            case Key::F6:
            {
                (m_Data.sceneState == State::SceneEdit || m_Data.sceneState == State::ScenePlay)
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

        // m_ScenePanel->GetRT()->CreateFramebuffers(backBufferCount, backBufferIndex);
        m_ScenePanel->GetRT()->CreateSingleFramebuffer();

        nvrhi::IFramebuffer *viewportFramebuffer = m_ScenePanel->GetRT()->GetCurrentFramebuffer();
        nvrhi::Viewport viewport = viewportFramebuffer->getFramebufferInfo().getViewport();

        // create pipelines
        m_EnvPipeline->Create(viewportFramebuffer);
        m_MeshPipeline->Create(viewportFramebuffer);

        // main scene rendering
        m_CommandList->open();

        // clear main framebuffer color attachment
        nvrhi::utils::ClearColorAttachment(m_CommandList, mainFramebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

        m_ScenePanel->GetRT()->ClearColorAttachment(m_CommandList);

        float farDepth = 1.0f; // LessOrEqual
        m_CommandList->clearDepthStencilTexture(m_ScenePanel->GetRT()->GetDepthAttachment(), nvrhi::AllSubresources, true, farDepth, true, 0);

        // render environment
        m_Environment->Render(viewportFramebuffer, m_EnvPipeline, m_ScenePanel->GetViewportCamera());

        // render objects
        for (auto &model : m_Models)
        {
            if (!model)
                continue;

            model->Render(m_CommandList, viewportFramebuffer, m_MeshPipeline);
        }

        m_ActiveScene->OnRenderRuntimeSimulate(m_ScenePanel->GetViewportCamera(), viewportFramebuffer);

        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void EditorLayer::OnGuiRender()
    {
        constexpr f32 TITLE_BAR_HEIGHT = 60.0f;

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
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const ImVec2 minPos = viewport->Pos;
        const ImVec2 maxPos = ImVec2(viewport->Pos.x + viewport->Size.x, totalTitlebarHeight);
        drawList->AddRectFilled(minPos, maxPos, IM_COL32(30, 30, 30, 255));


        // =========== Menubar ===========
        const glm::vec2 menuBarButtonSize = { 45.0f, 20.0f };
        const auto decorateBt = UIButtonDecorate()
            .Fill( { 0.32f, 0.0f, 0.0f, 1.0f }, { 0.62f, 0.0f, 0.0f, 1.0f })
            .Outline( { 0.62f, 0.0f, 0.0f, 1.0f }, { 0.92f, 0.0f, 0.0f, 1.0f });

        // ===== MENUBAR BUTTONS ==== 
        UI::DrawHorizontalButtonList(menuBarButtonSize,
            { 
                UIButton("File", decorateBt, []() { ImGui::OpenPopup("MenuBar_File"); }),
                UIButton("Edit", decorateBt, []() { ImGui::OpenPopup("MenuBar_Edit"); }),
                UIButton("View", decorateBt, []() {  ImGui::OpenPopup("MenuBar_View"); })
            },
            2.0f, // GAP
            { 12.0f, 2.0f }, // Padding
            { 0.0f, 0.0f } // Margin
        );
        
        // ==== TOOLBAR BUTTONS ====

        bool sceneStatePlaying = m_Data.sceneState == State::ScenePlay;
        
        glm::vec4 toolbarColor = glm::vec4{ 0.32f, 0.0f, 0.0f, 1.0f };
        glm::vec4 toolbarHoverColor = glm::vec4{ 0.62f, 0.0f, 0.0f, 1.0f };
        glm::vec4 toolbarOutlineColor = glm::vec4 { 0.62f, 0.0f, 0.0f, 1.0f };
        glm::vec4 toolbarOutlineHoverColor = glm::vec4{ 0.92f, 0.0f, 0.0f, 1.0f };

        if (sceneStatePlaying)
        {
            toolbarColor = glm::vec4 { 0.0f, 0.32f, 0.0f, 1.0f };
            toolbarHoverColor = glm::vec4 { 0.0f, 0.62f, 0.0f, 1.0f };
            toolbarOutlineColor = glm::vec4 { 0.0f, 0.62f, 0.0f, 1.0f };
            toolbarOutlineHoverColor = glm::vec4 { 0.00f, 92.0f, 0.0f, 1.0f };
        }


        if (UI::DrawButton(UIButton(sceneStatePlaying ? "Stop" : "Play",
            UIButtonDecorate()
            .Fill(toolbarColor, toolbarHoverColor)
            .Outline(toolbarOutlineColor, toolbarOutlineHoverColor)), menuBarButtonSize, { 12.0f, 2.0f }, Margin(40.0f, 3.0f, 0.0f, 3.0f)))
        {
            if (sceneStatePlaying)
            {
                OnSceneStop();
#if _WIN32
                HWND hwnd = glfwGetWin32Window(Application::GetInstance()->GetDeviceManager()->GetWindow());
                COLORREF rgbRed = 0x00E86071;
                DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif
            }
            else
            {
                OnScenePlay();
#if _WIN32
                HWND hwnd = glfwGetWin32Window(Application::GetInstance()->GetDeviceManager()->GetWindow());
                COLORREF rgbRed = 0x000000AB;
                DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif
            }
        }

        if (UI::DrawButton(UIButton("Simulate", decorateBt), menuBarButtonSize, {12.0f, 2.0f }, Margin(3.0f, 0.0f)))
        {
        }


        if (ImGui::BeginPopup("MenuBar_File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
                NewScene();
            }
            else if (ImGui::MenuItem("Open Scene"))
            {
                OpenScene();
            }
            if (ImGui::MenuItem("Save Scene"))
            {
                SaveScene();
            }
            else if (ImGui::MenuItem("Save Scene As"))
            {
                SaveSceneAs();
            }
            
            ImGui::Separator();

            if (ImGui::MenuItem("New Project"))
            {

            }
            else if (ImGui::MenuItem("Save Project"))
            {

            }
            else if (ImGui::MenuItem("Save Project As"))
            {

            }
            else if (ImGui::MenuItem("Close Project"))
            {

            }

            ImGui::EndPopup();
        }

        // dockspace
        ImGui::SetCursorScreenPos({viewport->Pos.x, totalTitlebarHeight});
        ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        {
            // scene dockspace
            m_ScenePanel->OnGuiRender();
            ImGui::Begin("Models");

            if (ImGui::Button("Add GLTF/GLB"))
            {
                std::vector<std::string> filepaths = FileDialogs::OpenFiles("GLTF/GLB Files (*.gltf;*.glb)\0*.gltf;*.glb\0All Files (*.*)\0*.*\0");
                if (!filepaths.empty())
                {
                    ModelCreateInfo modelCI;
                    modelCI.device = m_Device;
                    modelCI.debugBuffer = m_DebugRenderBuffer;
                    modelCI.cameraBuffer = m_Environment->GetCameraBuffer();
                    modelCI.lightBuffer = m_Environment->GetDirLightBuffer();
                    modelCI.envBuffer = m_Environment->GetParamsBuffer();

                    ModelImporter::LoadAsync(&m_Models, filepaths, modelCI, m_Environment, m_MeshPipeline);
                }
            }

            int index = 0;
            for (auto it = m_Models.begin(); it != m_Models.end(); )
            {
                auto &model = *it;
                if (!model)
                {
                    ++it;
                    continue;
                }

                bool requestToDelete = false;

                const std::string modelName = model->GetFilepath().stem().generic_string();
                bool opened = ImGui::TreeNodeEx((void *)(uint64_t *) &model, ImGuiTreeNodeFlags_OpenOnArrow, "%s", modelName.c_str());

                if (ImGui::IsItemClicked())
                {
                    m_SelectedModel = model.get();
                }

                if (ImGui::BeginPopupContextItem(modelName.c_str()))
                {
                    if (ImGui::MenuItem("Delete"))
                    {
                        requestToDelete = true;
                        m_SelectedModel = nullptr;
                    }

                    ImGui::EndPopup();
                }

                if (opened)
                {
                    glm::vec3 scale, translation, skew;
                    glm::vec4 perspective;
                    glm::quat orientation;
                    glm::decompose(model->transform, scale, orientation, translation, skew, perspective);

                    ImGui::DragFloat3("Translation", &translation.x, 0.025f);

                    glm::vec3 eulerAngle = glm::degrees(glm::eulerAngles(orientation));
                    if (ImGui::DragFloat3("Rotation", &eulerAngle.x, 0.025f))
                        orientation = glm::radians(eulerAngle);

                    ImGui::DragFloat3("Scale", &scale.x, 0.025f);

                    model->transform = glm::recompose(scale, orientation, translation, skew, perspective);

                    if (!requestToDelete)
                    {
                        if (ImGui::TreeNode("Meshes"))
                        {
                            for (const Ref<Mesh> &mesh : model->meshes)
                            {
                                bool opened = ImGui::TreeNode(mesh->name.c_str());

                                if (ImGui::BeginDragDropSource())
                                {
                                    ImGui::SetDragDropPayload("MESH_ITEM", &mesh, sizeof(Ref<Mesh>));
                                    ImGui::EndDragDropSource();
                                }

                                if (opened)
                                {
                                    ImGui::TreePop();
                                }
                            }

                            ImGui::TreePop();
                        }

                        if (!model->animations.empty() && ImGui::TreeNode("Animation"))
                        {
                            for (size_t animIdx = 0; animIdx < model->animations.size(); ++animIdx)
                            {
                                const Ref<SkeletalAnimation> &anim = model->animations[animIdx];
                                if (ImGui::TreeNodeEx(anim->name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", anim->name.c_str()))
                                {
                                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                    {
                                        AnimationSystem::PlayAnimation(model, animIdx);
                                    }
                                    ImGui::TreePop();
                                }
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Skeleton"))
                        {
                            for (auto &joint : model->skeleton.joints)
                            {
                                if (ImGui::TreeNodeEx(joint.name.c_str(), ImGuiTreeNodeFlags_None, "%s: %d", joint.name.c_str(), joint.id))
                                {
                                    ImGui::Text("Parent: %d", joint.parentJointId);

                                    ImGui::Text("Inv Bind Pose");
                                    ImGui::InputFloat4("BP 0", &joint.inverseBindPose[0].x);
                                    ImGui::InputFloat4("BP 1", &joint.inverseBindPose[1].x);
                                    ImGui::InputFloat4("BP 2", &joint.inverseBindPose[2].x);
                                    ImGui::InputFloat4("BP 3", &joint.inverseBindPose[3].x);

                                    ImGui::Text("Local Transform");
                                    ImGui::InputFloat4("LT 0", &joint.localTransform[0].x);
                                    ImGui::InputFloat4("LT 1", &joint.localTransform[1].x);
                                    ImGui::InputFloat4("LT 2", &joint.localTransform[2].x);
                                    ImGui::InputFloat4("LT 3", &joint.localTransform[3].x);

                                    ImGui::Text("World Transform");
                                    ImGui::InputFloat4("WT 0", &joint.globalTransform[0].x);
                                    ImGui::InputFloat4("WT 1", &joint.globalTransform[1].x);
                                    ImGui::InputFloat4("WT 2", &joint.globalTransform[2].x);
                                    ImGui::InputFloat4("WT 3", &joint.globalTransform[3].x);

                                    ImGui::TreePop();
                                }
                            }

                            ImGui::TreePop();
                        }

                    }

                    ImGui::TreePop();
                }

                if (requestToDelete)
                {
                    it = m_Models.erase(it);
                }
                else
                {
                    ++it;
                    ++index;
                }
            }

            ImGui::End();

            ImGui::Begin("Model Hierarchy");
            if (m_SelectedModel)
            {
                for (const auto &node : m_SelectedModel->nodes)
                    TraverseNodes(m_SelectedModel, node, 0);
            }
            ImGui::End();

            ImGui::Begin("Material");
            if (m_SelectedMaterial)
            {
                ImGui::ColorEdit4("Base Color", &m_SelectedMaterial->baseColor.x);
                ImGui::DragFloat("Metallic", &m_SelectedMaterial->metallic, 0.005f, 0.0f, 1.0f);
                ImGui::DragFloat("Rougness", &m_SelectedMaterial->roughness, 0.005f, 0.0f, 1.0f);
                ImGui::DragFloat("Emissive", &m_SelectedMaterial->emissive, 0.005f, 0.0f, 1000.0f);
            }
            ImGui::End();

            // Render GUI
            SettingsUI();
        }

        ImGui::End(); // end dockspace
    }

    void EditorLayer::NewScene()
    {
        m_CurrentSceneFilePath.clear();

        // create editor scene
        m_EditorScene = CreateRef<Scene>("New Scene");

        // pass to active scene
        m_ActiveScene = m_EditorScene;
        m_ScenePanel->SetActiveScene(m_ActiveScene.get(), true); // reset selected entity
    }

    void EditorLayer::SaveScene()
    {
        if (m_CurrentSceneFilePath.empty())
        {
            SaveSceneAs();
        }
        else
        {
            SaveScene(m_CurrentSceneFilePath);
        }
    }

    bool EditorLayer::SaveScene(const std::filesystem::path &filepath)
    {
        SceneSerializer serializer(m_ActiveScene);
        return serializer.Serialize(filepath);
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Ignite Scene (*igs)\0*.igs\0");
        if (!filepath.empty())
        {
            m_CurrentSceneFilePath = filepath;
            SaveScene(filepath);
        }
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Ignite Scene (*igs)\0*.igs\0");
        if (!filepath.empty())
        {
            m_CurrentSceneFilePath = filepath;
            OpenScene(filepath);
        }
    }

    bool EditorLayer::OpenScene(const std::filesystem::path &filepath)
    {
        if (m_Data.sceneState == State::ScenePlay)
            OnSceneStop();

        Ref<Scene> openScene = SceneSerializer::Deserialize(filepath);
        if (openScene)
        {
            m_EditorScene = SceneManager::Copy(openScene);
            m_EditorScene->SetDirtyFlag(false);

            m_ActiveScene = m_EditorScene;
            m_ScenePanel->SetActiveScene(m_ActiveScene.get(), true);
        }

        return true;
    }

    void EditorLayer::NewProject()
    {

    }

    void EditorLayer::SaveProject()
    {

    }

    void EditorLayer::SaveProject(const std::filesystem::path &filepath)
    {

    }

    void EditorLayer::SaveProjectAs()
    {

    }

    void EditorLayer::OpenProject()
    {

    }

    void EditorLayer::OpenProject(const std::filesystem::path &filepath)
    {

    }

    void EditorLayer::OnScenePlay()
    {
        m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::NONE);

        if (m_Data.sceneState != State::SceneEdit)
            OnSceneStop();

        m_Data.sceneState = State::ScenePlay;

        // copy initial components to new scene
        m_ActiveScene = SceneManager::Copy(m_EditorScene);
        m_ActiveScene->OnStart();

        m_ScenePanel->SetActiveScene(m_ActiveScene.get());
    }

    void EditorLayer::OnSceneStop()
    {
        m_Data.sceneState = State::SceneEdit;
        
        m_ActiveScene->OnStop();
        m_ActiveScene = m_EditorScene;

        m_ScenePanel->SetActiveScene(m_EditorScene.get());
    }

    void  EditorLayer::OnSceneSimulate()
    {
        if (m_Data.sceneState != State::SceneEdit)
            OnSceneStop();

        m_Data.sceneState = State::SceneSimulate;
    }

    void EditorLayer::SettingsUI()
    {
        ImGui::Begin("Settings", &m_Data.settingsWindow);

         ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        // Camera
        if (ImGui::TreeNodeEx("Camera", treeFlags))
        {
            m_ScenePanel->CameraSettingsUI();

            static const char *debugRenderView[] = { "None", "Normals", "Roughness", "Metallic", "Reflection" };
            const char *currentDebugRenderView = debugRenderView[m_DebugRenderData.renderIndex];

            if (ImGui::BeginCombo("Debug View", currentDebugRenderView))
            {
                for (size_t i = 0; i < std::size(debugRenderView); ++i)
                {
                    bool isSelected = strcmp(currentDebugRenderView, debugRenderView[i]) == 0;
                    if (ImGui::Selectable(debugRenderView[i], isSelected))
                    {
                        currentDebugRenderView = debugRenderView[i];
                        m_DebugRenderData.renderIndex = i;

                        m_CommandList->open();
                        m_CommandList->writeBuffer(m_DebugRenderBuffer, &m_DebugRenderData, sizeof(DebugRenderData));
                        m_CommandList->close();
                        m_Device->executeCommandList(m_CommandList);
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            static const char *renderFillMode[] = { "Solid", "Wireframe" };
            const char *currentRenderFillMode = renderFillMode[(int)m_Data.rasterFillMode];

            if (ImGui::BeginCombo("Fill Mode", currentRenderFillMode))
            {
                for (size_t i = 0; i < std::size(renderFillMode); ++i)
                {
                    bool isSelected = strcmp(currentRenderFillMode, renderFillMode[i]) == 0;
                    
                    if (ImGui::Selectable(renderFillMode[i], isSelected))
                    {
                        m_Data.rasterFillMode = (nvrhi::RasterFillMode)i;
                        m_MeshPipeline->GetParams().fillMode = m_Data.rasterFillMode;
                        m_MeshPipeline->ResetHandle();
                    }
                    
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            static const char *rasterCullMode[] = { "Back", "Front", "None" };
            const char *currentRasterCullMode = rasterCullMode[(int)m_Data.rasterCullMode];

            if (ImGui::BeginCombo("Cull Mode", currentRasterCullMode))
            {
                for (size_t i = 0; i < std::size(rasterCullMode); ++i)
                {
                    bool isSelected = strcmp(currentRasterCullMode, rasterCullMode[i]) == 0;

                    if (ImGui::Selectable(rasterCullMode[i], isSelected))
                    {
                        m_Data.rasterCullMode = (nvrhi::RasterCullMode)i;
                        m_MeshPipeline->GetParams().cullMode = m_Data.rasterCullMode;

                        m_MeshPipeline->ResetHandle();
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::TreePop();
        }

        ImGui::Separator();

        // Environment
        if (ImGui::TreeNodeEx("Environment", treeFlags))
        {
            static glm::vec2 sunAngles = { 50, -27.0f }; // pitch (elevation), yaw (azimuth)

            if (ImGui::Button("Load HDR Texture"))
            {
                std::string filepath = FileDialogs::OpenFile("HDR Files (*.hdr)\0*.hdr\0");
                if (!filepath.empty())
                {
                    m_Environment->LoadTexture(filepath, m_EnvPipeline->GetBindingLayout());
                    m_Environment->WriteTexture();

                    for (auto &model : m_Models)
                    {
                        if (!model)
                            continue;

                        model->SetEnvironmentTexture(m_Environment->GetHDRTexture());
                        model->CreateBindingSet(m_MeshPipeline->GetBindingLayout());
                    }
                }
            }

            ImGui::Separator();
            
            ImGui::Text("Sun Angles");

            if (ImGui::SliderFloat("Elevation", &sunAngles.x, 0.0f, 180.0f))
                m_Environment->SetSunDirection(sunAngles.x, sunAngles.y);
            if (ImGui::SliderFloat("Azimuth", &sunAngles.y, -180.0f, 180.0f))
                m_Environment->SetSunDirection(sunAngles.x, sunAngles.y);

            ImGui::Separator();
            ImGui::ColorEdit4("Color", &m_Environment->dirLight.color.x);
            ImGui::SliderFloat("Intensity", &m_Environment->dirLight.intensity, 0.01f, 1.0f);

            float angularSize = glm::degrees(m_Environment->dirLight.angularSize);
            if (ImGui::SliderFloat("Angular Size", &angularSize, 0.1f, 90.0f))
            {
                m_Environment->dirLight.angularSize = glm::radians(angularSize);
            }

            ImGui::DragFloat("Ambient", &m_Environment->dirLight.ambientIntensity, 0.005f, 0.01f, 100.0f);
            ImGui::DragFloat("Exposure", &m_Environment->params.exposure, 0.005f, 0.1f, 10.0f);
            ImGui::DragFloat("Gamma", &m_Environment->params.gamma, 0.005f, 0.1f, 10.0f);
        
            ImGui::TreePop();
        }

        ImGui::End();
    }

    void EditorLayer::TraverseNodes(Model *model, const NodeInfo &node, int traverseIndex)
    {
        if (node.parentID != -1 && traverseIndex == 0)
            return;

        ImGuiTreeNodeFlags flags = (node.childrenIDs.empty()) ? ImGuiTreeNodeFlags_Leaf : 0 | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        bool opened = ImGui::TreeNodeEx(node.name.c_str(), flags, node.name.c_str());

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MESH_ITEM"))
            {
                LOG_ASSERT(payload->DataSize == sizeof(Ref<Mesh>), "Wrong mesh item");

                Ref<Mesh> mesh = *static_cast<Ref<Mesh> *>(payload->Data);
                // mesh->nodeID = node.id;
            }
            ImGui::EndDragDropTarget();
        }

        if (opened)
        {
            for (i32 child : node.childrenIDs)
            {
                TraverseNodes(model, model->nodes[child], ++traverseIndex);
            }

            for (i32 meshIndex : node.meshIndices)
            {
                Ref<Mesh> mesh = model->meshes[meshIndex];

                bool opened = ImGui::TreeNodeEx(mesh->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow, "%s", mesh->name.c_str());

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("MESH_ITEM", &mesh, sizeof(Ref<Mesh>));
                    ImGui::EndDragDropSource();
                }

                if (opened)
                {
                    if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_Leaf, "Material"))
                    {
                        if (ImGui::IsItemClicked())
                        {
                            m_SelectedMaterial = &mesh->material.data;
                        }

                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
    }

}
