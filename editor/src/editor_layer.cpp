#include "editor_layer.hpp"

#include "panels/scene_panel.hpp"
#include "panels/content_browser_panel.hpp"

#include "ignite/core/platform_utils.hpp"
#include "ignite/core/command.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/graphics/texture.hpp"
#include "ignite/animation/animation_system.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/mesh_factory.hpp"
#include "ignite/imgui/gui_function.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/model.hpp"
#include "ignite/asset/asset.hpp"
#include "ignite/asset/asset_importer.hpp"

#include <glm/glm.hpp>
#include <nvrhi/utils.h>

#ifdef _WIN32
#   include <dwmapi.h>
#   include <ShellScalingApi.h>
#endif
#include <ranges>

namespace ignite
{
    static void TestLoader(nvrhi::IDevice *device, Ref<Scene> &scene, const std::filesystem::path &filepath)
    {
        LOG_ASSERT(std::filesystem::exists(filepath), "[Mesh Loader] File does not exists!");
        
        Assimp::Importer importer;
        const aiScene *assimpScene = importer.ReadFile(filepath.generic_string(), ASSIMP_IMPORTER_FLAGS);

        LOG_ASSERT(assimpScene == nullptr || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || assimpScene->mRootNode,
            "[Model] Failed to load {}: {}", filepath, importer.GetErrorString());

        if (!assimpScene)
        {
            return;
        }

        Entity rootNode = SceneManager::CreateEntity(scene.get(), "Model", EntityType_Node);
        SkinnedMesh &skinnedMesh = rootNode.AddComponent<SkinnedMesh>();

        if (assimpScene->HasAnimations())
        {
            MeshLoader::LoadAnimation(assimpScene, skinnedMesh.animations);

            // Process Skeleton
            MeshLoader::ExtractSkeleton(assimpScene, skinnedMesh.skeleton);
            MeshLoader::SortJointsHierchically(skinnedMesh.skeleton);
        }

        std::vector<Ref<Mesh>> meshes;
        meshes.resize(assimpScene->mNumMeshes);
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            meshes[i] = CreateRef<Mesh>();
        }

        std::vector<NodeInfo> nodes;

        MeshLoader::ProcessNode(assimpScene, assimpScene->mRootNode, filepath.generic_string(), meshes, nodes, skinnedMesh.skeleton, -1);
        MeshLoader::CalculateWorldTransforms(nodes, meshes);

        // First pass: create all node entities
        for (auto &node : nodes)
        {
            if (node.uuid == UUID(0)) // not yet created
            {
                Entity entity = SceneManager::CreateEntity(scene.get(), node.name, EntityType_Node);
                node.uuid = entity.GetUUID();

                Transform &tr = entity.GetComponent<Transform>();
                
                glm::vec3 skew;
                glm::vec4 perspective;

                glm::decompose(node.localTransform, tr.localScale, tr.localRotation, tr.localTranslation, skew, perspective);

                tr.dirty = true;
            }
        }

        // Second pass: establish hierarchy and add meshes


        for (auto &node : nodes)
        {
            Entity nodeEntity = SceneManager::GetEntity(scene.get(), node.uuid);

            if (node.parentID == -1)
            {
                // Attach the node to root node
                SceneManager::AddChild(scene.get(), rootNode, nodeEntity);
            }
            else
            {
                // Attach to parent if not root
                const auto &parentNode = nodes[node.parentID];
                Entity parentEntity = SceneManager::GetEntity(scene.get(), parentNode.uuid);
                SceneManager::AddChild(scene.get(), parentEntity, nodeEntity);
            }

            // Attach mesh entities to this node
            for (i32 meshIdx : node.meshIndices)
            {
                const auto &mesh = meshes[meshIdx];
                Entity meshEntity = SceneManager::CreateEntity(scene.get(), mesh->name, EntityType_Mesh);
                SceneManager::AddChild(scene.get(), nodeEntity, meshEntity);

                MeshRenderer &meshRenderer = meshEntity.AddComponent<MeshRenderer>();
                meshRenderer.root = rootNode.GetUUID();

                meshRenderer.mesh = CreateRef<EntityMesh>();

                const auto &parentNode = nodes[node.parentID];
                meshRenderer.parentNode = parentNode.uuid;

                meshRenderer.mesh->vertices = mesh->vertices;
                meshRenderer.mesh->indices = mesh->indices;
                meshRenderer.mesh->indexBuffer = mesh->indexBuffer;
                meshRenderer.mesh->vertexBuffer = mesh->vertexBuffer;

                meshRenderer.material = mesh->material;
            }
        }

        MeshLoader::ClearTextureCache();
    }

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
            params.recompileShader = false;
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
            m_Environment = Environment::Create(m_Device);
            m_Environment->LoadTexture(m_Device, "resources/hdr/klippad_sunrise_2_2k.hdr", m_EnvPipeline->GetBindingLayout());
            
            m_CommandList->open();
            m_Environment->WriteBuffer(m_CommandList);
            m_CommandList->close();
            m_Device->executeCommandList(m_CommandList);

            m_Environment->SetSunDirection(50.0f, -27.0f);
        }

        // write buffer with command list
        Renderer2D::InitQuadData(m_Device, m_CommandList);

        m_ScenePanel = CreateRef<ScenePanel>("Scene Panel", this);
        m_ScenePanel->CreateRenderTarget(m_Device);

        m_ContentBrowserPanel = CreateRef<ContentBrowserPanel>("Content Browser");

        // const auto &cmdArgs = Application::GetInstance()->GetCreateInfo().cmdLineArgs;
        // for (int i = 0; i < cmdArgs.count; ++i)
        // {
        //     std::string args = cmdArgs[i];
        //     if (args.find("-project=") != std::string::npos)
        //     {
        //         std::string projectFilepath = args.substr(9, args.size() - 9);
        //         OpenProject(projectFilepath);
        //     }
        // }

        NewScene();

        TestLoader(m_Device, m_ActiveScene, "C:/Users/Evangelion/Downloads/Compressed/KayKit_Adventurers_1.0_FREE/Characters/gltf/Rogue.glb");

        m_CommandList->open();

        for (auto &[uuid, ent] : m_ActiveScene->entities)
        {
            Entity entity { ent, m_ActiveScene.get() };
            ID &idcomp = entity.GetComponent<ID>();
            Transform &tr = entity.GetComponent<Transform>();

            if (entity.HasComponent<MeshRenderer>())
            {
                MeshRenderer &meshRenderer = entity.GetComponent<MeshRenderer>();

                meshRenderer.mesh->CreateConstantBuffers(m_Device);

                // m_CreateInfo.commandList->open();
                m_CommandList->writeBuffer(meshRenderer.mesh->vertexBuffer, meshRenderer.mesh->vertices.data(), sizeof(VertexMesh) * meshRenderer.mesh->vertices.size());
                m_CommandList->writeBuffer(meshRenderer.mesh->indexBuffer, meshRenderer.mesh->indices.data(), sizeof(uint32_t) * meshRenderer.mesh->indices.size());

                // write textures
                if (meshRenderer.material.ShouldWriteTexture())
                {
                    meshRenderer.material.WriteBuffer(m_CommandList);
                }

                auto desc = nvrhi::BindingSetDesc();

                desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_Environment->GetCameraBuffer()));
                desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, m_Environment->GetDirLightBuffer()));
                desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, m_Environment->GetParamsBuffer()));
                desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(3, meshRenderer.mesh->objectBuffer));
                desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(4, meshRenderer.mesh->materialBuffer));
                desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(5, m_DebugRenderBuffer));

                desc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, meshRenderer.material.textures[aiTextureType_DIFFUSE].handle));
                desc.addItem(nvrhi::BindingSetItem::Texture_SRV(1, meshRenderer.material.textures[aiTextureType_SPECULAR].handle));
                desc.addItem(nvrhi::BindingSetItem::Texture_SRV(2, meshRenderer.material.textures[aiTextureType_EMISSIVE].handle));
                desc.addItem(nvrhi::BindingSetItem::Texture_SRV(3, meshRenderer.material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle));
                desc.addItem(nvrhi::BindingSetItem::Texture_SRV(4, meshRenderer.material.textures[aiTextureType_NORMALS].handle));
                desc.addItem(nvrhi::BindingSetItem::Texture_SRV(5, m_Environment->GetHDRTexture()));
                desc.addItem(nvrhi::BindingSetItem::Sampler(0, meshRenderer.material.sampler));

                meshRenderer.mesh->bindingSet = m_Device->createBindingSet(desc, m_MeshPipeline->GetBindingLayout());
                LOG_ASSERT(meshRenderer.mesh->bindingSet, "Failed to create binding set");
            }
        }

        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void EditorLayer::OnDetach()
    {
        Layer::OnDetach();
        m_CommandList = nullptr;
    }

    void EditorLayer::OnUpdate(f32 deltaTime)
    {
        Layer::OnUpdate(deltaTime);

        AssetImporter::SyncMainThread(m_CommandList, m_Device);

        float timeInSeconds = static_cast<float>(glfwGetTime());

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

        // if (AnimationSystem::UpdateSkeleton(m_TestData.skeleton, m_TestData.animations[m_TestData.activeAnimIndex], timeInSeconds))
        // {
        //     m_TestData.animations[m_TestData.activeAnimIndex]->isPlaying = true;
        //     m_TestData.boneTransforms = AnimationSystem::GetFinalJointTransforms(m_TestData.skeleton);
        // }


        for (auto &model : m_Models)
        {
            if (!model)
                continue;

            AnimationSystem::UpdateAnimation(model, timeInSeconds);
            model->OnUpdate(deltaTime);
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
        m_Environment->Render(m_CommandList, viewportFramebuffer, m_EnvPipeline, m_ScenePanel->GetViewportCamera());

        for (entt::entity e : m_ActiveScene->entities | std::views::values)
        {
            Entity entity = { e, m_ActiveScene.get() };
            Transform &tr = entity.GetComponent<Transform>();
            ID &idcomp = entity.GetComponent<ID>();

            if (!tr.visible)
            {
                continue;
            }

            if (entity.HasComponent<MeshRenderer>())
            {
                MeshRenderer &meshRenderer = entity.GetComponent<MeshRenderer>();

                // write material constant buffer
                m_CommandList->writeBuffer(meshRenderer.mesh->materialBuffer, &meshRenderer.material.data, sizeof(meshRenderer.material.data));
                m_CommandList->writeBuffer(meshRenderer.mesh->objectBuffer, &meshRenderer.meshBuffer, sizeof(meshRenderer.meshBuffer));

                // render
                auto meshGraphicsState = nvrhi::GraphicsState();
                meshGraphicsState.pipeline = m_MeshPipeline->GetHandle();
                meshGraphicsState.framebuffer = viewportFramebuffer;
                meshGraphicsState.viewport = nvrhi::ViewportState().addViewportAndScissorRect(viewport);
                meshGraphicsState.addVertexBuffer({ meshRenderer.mesh->vertexBuffer, 0, 0 });
                meshGraphicsState.indexBuffer = { meshRenderer.mesh->indexBuffer, nvrhi::Format::R32_UINT };

                if (meshRenderer.mesh->bindingSet != nullptr)
                    meshGraphicsState.addBindingSet(meshRenderer.mesh->bindingSet);

                m_CommandList->setGraphicsState(meshGraphicsState);

                nvrhi::DrawArguments args;
                args.setVertexCount((uint32_t)meshRenderer.mesh->indices.size());
                args.instanceCount = 1;

                m_CommandList->drawIndexed(args);
            }
        }

        // render objects
        for (auto &model : m_Models)
        {
            if (!model)
            {
                continue;
            }
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
                m_Data.popupNewProjectModal = true;
            }

            else if (ImGui::MenuItem("Save Project"))
            {
                SaveProject();
            }

            else if (ImGui::MenuItem("Open Project"))
            {
                OpenProject();
            }

            ImGui::EndPopup();
        }

        if (m_Data.popupNewProjectModal)
        {
            ImGui::OpenPopup("New Project");
            m_Data.popupNewProjectModal = false;
        }

        if (ImGui::BeginPopupModal("New Project", nullptr, 0))
        {
            ImGui::Text("Create a brand new project here...");

            static char nameBuffer[128] = {};
            if (ImGui::InputText("Project Name", nameBuffer, 128))
            {
                m_Data.projectCreateInfo.name = std::string(nameBuffer);
            }

            static char filepathBuffer[256] = {};
            if (ImGui::InputText("Location", filepathBuffer, sizeof(filepathBuffer), ImGuiInputTextFlags_ReadOnly))
            {
                m_Data.projectCreateInfo.filepath = std::string(filepathBuffer);
            }

            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                m_Data.projectCreateInfo.filepath = FileDialogs::SelectFolder();

                if (!m_Data.projectCreateInfo.filepath.empty())
                {
                    std::string filepathCopy = m_Data.projectCreateInfo.filepath.generic_string();
                    memcpy(filepathBuffer, filepathCopy.c_str(), sizeof(filepathBuffer));
                }
            }

            if (ImGui::Button("Create"))
            {
                if (!m_Data.projectCreateInfo.name.empty() && !m_Data.projectCreateInfo.filepath.empty())
                {
                    while (m_Data.projectCreateInfo.name.find(" ") != std::string::npos)
                    {
                        size_t spacePos = m_Data.projectCreateInfo.name.find(" ");
                        m_Data.projectCreateInfo.name.replace(spacePos, 1, "");
                    }

                    m_Data.projectCreateInfo.filepath /= (m_Data.projectCreateInfo.name + ".ixproj");

                    Ref<Project> newProject = Project::Create(m_Data.projectCreateInfo);

                    if (newProject)
                    {
                        ProjectSerializer serializer(newProject);
                        serializer.Serialize(m_Data.projectCreateInfo.filepath);

                        m_ActiveProject = newProject;
                    }

                    m_Data.projectCreateInfo.filepath.clear();
                    m_Data.projectCreateInfo.name.clear();

                    memset(nameBuffer, 0, sizeof(nameBuffer));
                    memset(filepathBuffer, 0, sizeof(filepathBuffer));

                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    // TODO: Show error message text
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                m_Data.projectCreateInfo.filepath.clear();
                m_Data.projectCreateInfo.name.clear();

                memset(nameBuffer, 0, sizeof(nameBuffer));
                memset(filepathBuffer, 0, sizeof(filepathBuffer));

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }


        // dockspace
        ImGui::SetCursorScreenPos({viewport->Pos.x, totalTitlebarHeight});
        ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        {
            // scene dockspace
            m_ScenePanel->OnGuiRender();
            m_ContentBrowserPanel->OnGuiRender();

            ImGui::Begin("Project");

            if (Project *activeProject = Project::GetInstance())
            {
                const auto &info = activeProject->GetInfo();

                std::string projectName = info.name;
                if (activeProject->IsDirty())
                    projectName += "*";

                ImGui::Text("Name: %s", projectName.c_str());
                
                ImGui::Text("Filepath: %s", info.filepath.generic_string().c_str());

                auto &assetManager = activeProject->GetAssetManager();
                auto &assetRegistry = assetManager.GetAssetAssetRegistry();

                if (!assetRegistry.empty())
                {
                    ImGui::Text("Registered assets:");

                    for (auto &[handle, metadata] : assetRegistry)
                    {
                        ImGui::Text("Handle: %ull", (uint64_t)handle);
                        ImGui::Text("Type: %s", AssetTypeToString(metadata.type).c_str());
                        ImGui::Text("Filepath: %s", metadata.filepath.generic_string().c_str());
                    }
                }
            }

            ImGui::End();

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
                        m_SelectedNode = nullptr;
                    }

                    ImGui::EndPopup();
                }

                if (opened)
                {
                    /*glm::vec3 scale, translation, skew;
                    glm::vec4 perspective;
                    glm::quat orientation;
                    glm::decompose(model->transform, scale, orientation, translation, skew, perspective);

                    ImGui::DragFloat3("Translation", &translation.x, 0.025f);
                    glm::vec3 eulerAngle = glm::degrees(glm::eulerAngles(orientation));
                    if (ImGui::DragFloat3("Rotation", &eulerAngle.x, 0.025f))
                        orientation = glm::radians(eulerAngle);
                    ImGui::DragFloat3("Scale", &scale.x, 0.025f);
                    model->transform = glm::recompose(scale, orientation, translation, skew, perspective);*/

                    if (!requestToDelete)
                    {
                        if (m_SelectedNode)
                        {
                            {
                                glm::vec3 translation, scale;
                                glm::quat orientation;
                                Math::DecomposeTransform(m_SelectedNode->localTransform, translation, orientation, scale);
                                ImGui::DragFloat3("Translation", &translation.x, 0.025f);
                                glm::vec3 euler = eulerAngles(orientation);
                                ImGui::DragFloat3("Rotation", &euler.x, 0.025f);
                                ImGui::DragFloat3("Scale", &scale.x, 0.025f);
                            }

                            ImGui::Separator();


                            {
                                glm::vec3 translation, scale;
                                glm::quat orientation;
                                Math::DecomposeTransform(m_SelectedNode->worldTransform, translation, orientation, scale);
                                ImGui::DragFloat3("WTranslation", &translation.x, 0.025f);
                                glm::vec3 euler = eulerAngles(orientation);
                                ImGui::DragFloat3("WRotation", &euler.x, 0.025f);
                                ImGui::DragFloat3("WScale", &scale.x, 0.025f);
                            }
                        }

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
                for (auto &node : m_SelectedModel->nodes)
                {
                    if (node.parentID == -1)
                    {
                        TraverseNodes(m_SelectedModel, node);
                    }
                }
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
        std::string filepath = FileDialogs::SaveFile("Ignite Scene (*ixs)\0*.ixs\0");
        if (!filepath.empty())
        {
            m_CurrentSceneFilePath = filepath;
            SaveScene(filepath);
        }
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Ignite Scene (*igs)\0*.ixs\0");
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

    void EditorLayer::SaveProject()
    {
        if (m_ActiveProject)
        {
            const auto &info = m_ActiveProject->GetInfo();
            ProjectSerializer sr(m_ActiveProject);
            if (!info.filepath.empty())
            {
                sr.Serialize(info.filepath);
            }
        }
    }

    void EditorLayer::SaveProjectAs()
    {

    }

    void EditorLayer::OpenProject()
    {
        std::filesystem::path filepath = FileDialogs::OpenFile("Ignite Project (*.ixproj)\0*.ixproj\0");
        if (!filepath.empty())
        {
            OpenProject(filepath);
        }
    }

    bool EditorLayer::OpenProject(const std::filesystem::path &filepath)
    {
        Ref<Project> openedProject = ProjectSerializer::Deserialize(filepath);
        if (!openedProject)
        {
            return false;
        }

        m_ActiveProject = openedProject;

        m_ContentBrowserPanel->SetActiveProject(m_ActiveProject);


        if (m_ActiveProject->GetActiveScene())
        {
            Ref<Scene> scene = m_ActiveProject->GetActiveScene();

            m_EditorScene = SceneManager::Copy(scene);
            m_EditorScene->SetDirtyFlag(false);

            m_ActiveScene = m_EditorScene;
            m_ScenePanel->SetActiveScene(m_ActiveScene.get(), true);

            AssetMetaData metadata = Project::GetInstance()->GetAssetManager().GetMetaData(scene->handle);
            auto scenePath = Project::GetInstance()->GetAssetFilepath(metadata.filepath);
            m_CurrentSceneFilePath = scenePath;
        }


        return true;
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
                    EnvironmentImporter::UpdateTexture(&m_Environment, m_Device, filepath, m_EnvPipeline);
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

    void EditorLayer::TraverseNodes(Model *model, NodeInfo &node)
    {
        // Determine if the node is a leaf node (has no children and no meshes)
        bool isLeaf = node.childrenIDs.empty() && node.meshIndices.empty();

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        if (isLeaf)
            flags |= ImGuiTreeNodeFlags_Leaf;

        // Tree node UI
        bool opened = ImGui::TreeNodeEx((void *)(intptr_t)node.id, flags, node.name.c_str());

        // Handle drag drop target for meshes
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MESH_ITEM"))
            {
                LOG_ASSERT(payload->DataSize == sizeof(Ref<Mesh>), "Wrong mesh item");

                Ref<Mesh> mesh = *static_cast<Ref<Mesh> *>(payload->Data);
                mesh->nodeID = node.id;
                mesh->nodeParentID = node.parentID;
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_SelectedNode = &node;
        }

        if (opened)
        {
            // Display meshes under this node
            for (i32 meshIndex : node.meshIndices)
            {
                Ref<Mesh> mesh = model->meshes[meshIndex];
                bool meshOpened = ImGui::TreeNodeEx((void *)(intptr_t)meshIndex, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow, "%s", mesh->name.c_str());

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("MESH_ITEM", &mesh, sizeof(Ref<Mesh>));
                    ImGui::Text("%s", mesh->name.c_str());
                    ImGui::EndDragDropSource();
                }

                if (meshOpened)
                {
                    if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_Leaf, "Material"))
                    {
                        if (ImGui::IsItemClicked())
                        {
                            m_SelectedMaterial = &mesh->material.data;
                        }
                        ImGui::TreePop();
                    }

                    ImGui::TreePop(); // mesh node
                }
            }

            // Recurse into children
            for (i32 child : node.childrenIDs)
            {
                TraverseNodes(model, model->nodes[child]);
            }

            ImGui::TreePop(); // current node
        }
    }
}
