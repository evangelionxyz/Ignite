#include "editor_layer.hpp"
#include "panels/scene_panel.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/mesh_factory.hpp"
#include "ignite/core/platform_utils.hpp"
#include <glm/glm.hpp>
#include <nvrhi/utils.h>
#include "ignite/core/command.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/graphics/texture.hpp"

#include "ignite/imgui/gui_function.hpp"

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

        for (auto it = m_PendingLoadModels.begin(); it != m_PendingLoadModels.end(); )
        {
            std::future<Ref<Model>> &future = *it;
            if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                Ref<Model> model = future.get();
                m_Models.push_back(model);

                m_CommandList->open();
                model->WriteBuffer(m_CommandList);
                m_CommandList->close();
                m_Device->executeCommandList(m_CommandList);

                it = m_PendingLoadModels.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

        for (auto &model : m_Models)
            model->OnUpdate(deltaTime);

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
        m_EnvPipeline->Create(m_Device, viewportFramebuffer);

        m_MeshPipeline->Create(m_Device, viewportFramebuffer);


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
        const ImVec2 menuBarButtonSize = { 45.0f, 20.0f };
        ImVec2 menubarBTCursorPos = { viewport->Pos.x, viewport->Pos.y };
        ImVec2 menubarBTPos = menubarBTCursorPos + menuBarButtonSize;

        //                                                       Text,         OpenPopup
        static std::function<void(const ImVec2 &buttonSize, std::vector<UIButton> buttons, const float &gap, const ImVec2 &padding, const ImVec2 &margin)>
            menubarAction = [](const ImVec2 &buttonSize, std::vector<UIButton> buttons, const float &gap, const ImVec2 &padding, const ImVec2 &margin)
        {
            const ImGuiWindow *currentWindow = ImGui::GetCurrentWindow();
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            // add margin
            ImVec2 startPosition = currentWindow->DC.CursorPos; // use cursor pos (not position)
            startPosition += margin;

            // update screen pos
            ImGui::SetCursorScreenPos(startPosition);

            for (auto &[text, decorate, onClick, isHovered, isClicked] : buttons)
            {
                // Create an invisible button (captures input and sets hovered / clicked state)
                ImGui::InvisibleButton(text.c_str(), buttonSize + padding);

                if (ImGui::IsItemHovered())
                    isHovered = true;

                // Handle click
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    isClicked = true;
                    onClick();
                }

                const ImVec2 paddedSize = buttonSize + padding;

                glm::vec4 glmFillColor = decorate.fillColor.GetColor(isHovered);
                glm::vec4 glmOutlineColor = decorate.outlineColor.GetColor(isHovered);

                ImColor fillColor = { glmFillColor.r, glmFillColor.g, glmFillColor.b, glmFillColor.w };
                ImColor outlineColor = { glmOutlineColor.r, glmOutlineColor.g, glmOutlineColor.b, glmOutlineColor.w };

                // Draw background rectangle
                drawList->AddRectFilled(startPosition, startPosition + paddedSize, fillColor); // fill
                drawList->AddRect(startPosition, startPosition + paddedSize, outlineColor); // outline

                // Draw the text centered
                ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
                ImVec2 textStartPos = (paddedSize - textSize) * 0.5f;

                drawList->AddText({ startPosition + textStartPos }, IM_COL32(255, 255, 255, 255), text.c_str());

                // increment Position
                startPosition.x += buttonSize.x + gap;
                startPosition.x += padding.x; // only add x (Horizontally growing)
                startPosition.x += margin.x; // only add x (Horizontally growing)

                // update screen pos
                ImGui::SetCursorScreenPos(startPosition);
            }
        };

        const auto decorateBt = UIButtonDecorate()
            .Fill( { 0.32f, 0.0f, 0.0f, 1.0f }, { 0.62f, 0.0f, 0.0f, 1.0f })
            .Outline( { 0.62f, 0.0f, 0.0f, 1.0f }, { 0.92f, 0.0f, 0.0f, 1.0f });

        menubarAction(
            menuBarButtonSize,
            { 
                UIButton("File", decorateBt, []() { ImGui::OpenPopup("MenuBar_File"); }),
                 UIButton("Edit", decorateBt, []() { ImGui::OpenPopup("MenuBar_Edit"); }),
                UIButton("View", decorateBt, []() {  ImGui::OpenPopup("MenuBar_View"); })
            },
            2.0f, // GAP
            ImVec2{ 12.0f, 2.0f }, // Padding
            ImVec2{ 3.0f, 3.0f } // Margin
        );
        

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

        menubarBTCursorPos.x += menuBarButtonSize.x;
        /*ImGui::SetCursorScreenPos(menubarBTCursorPos);
        if (ImGui::Button("View"))
        {
            ImGui::OpenPopup("MenuBar_View");
        }*/

        // =========== Toolbar ===========
        // f32 buttonMiddle = totalTitlebarHeight / 2.0f - toolbarButtonSize.y / 2.0f;
        // ImVec2 toolbarBTCursorPos = { menubarBTCursorPos.x, buttonMiddle + (totalTitlebarHeight / 4.0f)};

        //menubarBTCursorPos.x += menuBarButtonSize.x + 5.0f; // 5.0f spacing
        //ImVec2 toolbarBTCursorPos = { menubarBTCursorPos.x, viewport->Pos.y };
        //ImGui::SetCursorScreenPos(toolbarBTCursorPos);
        bool sceneStatePlaying = m_Data.sceneState == State::ScenePlay;
        if (ImGui::Button(sceneStatePlaying ? "Stop" : "Play", menuBarButtonSize))
        {
            if (sceneStatePlaying)
                OnSceneStop();
            else 
                OnScenePlay();
        }

        if (ImGui::Button("Simulate"))
        {

        }

        // dockspace
        ImGui::SetCursorScreenPos({viewport->Pos.x, totalTitlebarHeight});
        ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        {
            // scene dockspace
            m_ScenePanel->OnGuiRender();
            ImGui::Begin("Models");

            if (ImGui::Button("Load GLTF/GLB"))
            {
                std::string filepath = FileDialogs::OpenFile("GLTF/GLB Files (*.gltf;*.glb)\0*.gltf;*.glb\0All Files (*.*)\0*.*\0");
                if (!filepath.empty())
                {
                    LoadModel(filepath);
                }
            }

            for (size_t i = 0; i < m_Models.size(); ++i)
            {
                auto &model = m_Models[i];

                bool opened = ImGui::TreeNodeEx((void *)(uint64_t *) &model, 0, "Model %zu", i);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    m_SelectedModel = model.get();

                if (opened)
                {
                    for (const auto &node : model->nodes)
                        TraverseNodes(model.get(), node, 0);
                    
                    ImGui::TreePop();
                }
            }

            if(m_SelectedModel && !m_SelectedModel->animations.empty() && ImGui::TreeNode("Animation"))
            {
                for (auto &anim : m_SelectedModel->animations)
                {
                    bool opened = ImGui::TreeNodeEx(anim->GetName().c_str(), ImGuiTreeNodeFlags_Leaf, "%s", anim->GetName().c_str());
                    if (opened)
                    {
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            if (m_SelectedMaterial)
            {
                ImGui::Separator();

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

        if (opened)
        {
            for (i32 child : node.childrenIDs)
            {
                TraverseNodes(model, model->nodes[child], ++traverseIndex);
            }

            ImGui::TreePop();
        }
    }

    void EditorLayer::LoadModel(const std::string &filepath)
    {
        std::future<Ref<Model>> modelFuture = std::async(std::launch::async, [this, filepath]()
        {
            ModelCreateInfo modelCI;
            modelCI.device = m_Device;
            modelCI.cameraBuffer = m_Environment->GetCameraBuffer();
            modelCI.lightBuffer = m_Environment->GetDirLightBuffer();
            modelCI.envBuffer = m_Environment->GetParamsBuffer();
            modelCI.debugBuffer = m_DebugRenderBuffer;

            Ref<Model> model = Model::Create(filepath, modelCI);
            model->SetEnvironmentTexture(m_Environment->GetHDRTexture());
            model->CreateBindingSet(m_MeshPipeline->GetBindingLayout());
            return model;
        });
        
        m_PendingLoadModels.push_back(std::move(modelFuture));
    }

}
