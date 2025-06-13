#include "editor_layer.hpp"

#include "panels/scene_panel.hpp"
#include "panels/content_browser_panel.hpp"

#include "ignite/core/platform_utils.hpp"
#include "ignite/core/command.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/imgui/gui_function.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/asset/asset.hpp"
#include "ignite/asset/asset_importer.hpp"

#include "ignite/audio/fmod_audio.hpp"
#include "ignite/audio/fmod_sound.hpp"
#include "ignite/audio/fmod_dsp.hpp"

#include <glm/glm.hpp>
#include <nvrhi/utils.h>
#include <ranges>
#include <cinttypes>

namespace ignite
{
    struct TestAudio
    {
        Ref<FmodReverb> reverb;
        Ref<FmodDistortion> distortion;
        Ref<FmodCompressor> compressor;
        Ref<FmodSound> sound;

        FMOD::ChannelGroup* reverbGroup;
    };

    TestAudio audio;
    
    EditorLayer::EditorLayer(const std::string &name)
        : Layer(name)
    {
    }

    void EditorLayer::OnAttach()
    {
        Layer::OnAttach();

        m_Device = Application::GetDeviceManager()->GetDevice();
        m_CommandList = m_Device->createCommandList();

        m_SceneRenderer.Init();

        // write buffer with command list
        Renderer2D::InitQuadData(m_CommandList);
        Renderer2D::InitLineData(m_CommandList);

        m_ScenePanel = CreateRef<ScenePanel>("Scene Panel", this);
        m_ScenePanel->CreateRenderTarget(m_Device);

        m_ContentBrowserPanel = CreateRef<ContentBrowserPanel>("Content Browser");

        const auto &cmdArgs = Application::GetInstance()->GetCreateInfo().cmdLineArgs;
        for (int i = 0; i < cmdArgs.count; ++i)
        {
            std::string args = cmdArgs[i];

            char projectArgs[] = "-project=";
            if (args.find(projectArgs) != std::string::npos)
            {
                std::string projectFilepath = args.substr(std::size(projectArgs) - 1, args.size() - std::size(projectArgs) + 1);
                OpenProject(projectFilepath);
            }
        }

        // create render target framebuffer
        m_ScenePanel->GetRenderTarget()->CreateSingleFramebuffer();
        m_SceneRenderer.CreatePipelines(m_ScenePanel->GetRenderTarget()->GetCurrentFramebuffer());

#if 0
        // Test Audio

        // Create reverb DSB
        audio.reverb = FmodReverb::Create();
        audio.reverb->SetDiffusion(25.0f);
        audio.reverb->SetWetLevel(20.0f);
        audio.reverb->SetDecayTime(500.0f);

        // Create distortion DSP
        audio.distortion = FmodDistortion::Create();
        audio.distortion->SetDistortionLevel(0.1f);
        audio.distortion->SetActive(false);

        // Create compressor DSP
        audio.compressor = FmodCompressor::Create();
        audio.compressor->SetRatio(3.0f);
        audio.compressor->SetRelease(100.0f);

        // Create reverb group
        audio.reverbGroup = FmodAudio::CreateChannelGroup("ReverbGroup");
        audio.reverbGroup->setMode(FMOD_CHANNELCONTROL_DSP_TAIL);
        audio.reverbGroup->addDSP(0, audio.distortion->GetFmodDsp()); // add dsp
        audio.reverbGroup->addDSP(1, audio.reverb->GetFmodDsp()); // add dsp
        audio.reverbGroup->addDSP(2, audio.compressor->GetFmodDsp());

        // create sound
        audio.sound = FmodSound::Create("music", "C:/Users/Evangelion Manuhutu/Downloads/Music/06. Mick Gordon - Hellwalker.flac");
        audio.sound->AddToChannelGroup(audio.reverbGroup);
        audio.sound->Play();
#endif
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

        if (!m_ActiveScene)
            return;

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

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
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_CLASS_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent &event)
    {
        bool control = Input::IsKeyPressed(KEY_LEFT_CONTROL);
        bool shift = Input::IsKeyPressed(KEY_LEFT_SHIFT);

        if (ImGui::GetIO().WantTextInput)
            return false;

        switch (event.GetKeyCode())
        {
            case Key::Escape:
            {
                if (m_ScenePanel->IsFocused())
                {
                    m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::NONE);
                }
                break;
            }
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

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &event)
    {
        if (event.Is(Mouse::ButtonLeft) && !m_ScenePanel->IsGizmoBeingUse() && m_ScenePanel->IsHovered())
        {
            m_Data.isPickingEntity = true;
        }

        return false;
    }

    void EditorLayer::OnRender(nvrhi::IFramebuffer *mainFramebuffer)
    {
        Layer::OnRender(mainFramebuffer);

        nvrhi::IFramebuffer *viewportFramebuffer = m_ScenePanel->GetRenderTarget()->GetCurrentFramebuffer();

        // main scene rendering
        m_CommandList->open();

        // clear main framebuffer color attachment
        nvrhi::utils::ClearColorAttachment(m_CommandList, mainFramebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

        m_ScenePanel->GetRenderTarget()->ClearColorAttachmentFloat(m_CommandList, 0);
        m_ScenePanel->GetRenderTarget()->ClearColorAttachmentUint(m_CommandList, 1, static_cast<uint32_t>(-1));

        f32 farDepth = 1.0f; // LessOrEqual
        m_CommandList->clearDepthStencilTexture(m_ScenePanel->GetRenderTarget()->GetDepthAttachment(), 
            nvrhi::AllSubresources, 
            true, // clear depth ?
            farDepth, // depth
            true, // clear stencil?
            0 // stencil
        );
        
        if (m_ActiveScene)
        {
            CameraBuffer cameraBuffer = { m_ScenePanel->GetViewportCamera().GetViewProjectionMatrix(), glm::vec4(m_ScenePanel->GetViewportCamera().position, 1.0f) };
            m_CommandList->writeBuffer(Renderer::GetCameraBufferHandle(), &cameraBuffer, sizeof(cameraBuffer));

            m_SceneRenderer.Render(m_ActiveScene.get(), &m_ScenePanel->GetViewportCamera(), m_CommandList, viewportFramebuffer);
            // m_SceneRenderer.RenderOutline(&m_ScenePanel->GetViewportCamera(), m_CommandList, viewportFramebuffer, m_ScenePanel->GetSelectedEntities());
        }

        // Create staging texture for read-back
        if (m_Data.isPickingEntity && m_ActiveScene)
        {
            nvrhi::TextureDesc stagingDesc = m_ScenePanel->GetRenderTarget()->GetColorAttachment(1)->getDesc();
            stagingDesc.initialState = nvrhi::ResourceStates::CopyDest;
            m_EntityIDStagingTexture = m_Device->createStagingTexture(stagingDesc, nvrhi::CpuAccessMode::Read);
            LOG_ASSERT(m_EntityIDStagingTexture, "Failed to create staging texture");
            m_CommandList->copyTexture(m_EntityIDStagingTexture, nvrhi::TextureSlice(), m_ScenePanel->GetRenderTarget()->GetColorAttachment(1), nvrhi::TextureSlice());
        }

        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);

        if (m_Data.isPickingEntity && m_ActiveScene)
        {
            // Map and read the pixel data
            size_t rowPitch = 0;
            void *mappedData = m_Device->mapStagingTexture(m_EntityIDStagingTexture, nvrhi::TextureSlice(), nvrhi::CpuAccessMode::Read, &rowPitch);
            if (mappedData) {
                uint32_t *pixelData = static_cast<uint32_t *>(mappedData);

                glm::vec2 mousePos = m_ScenePanel->GetViewportMousePos();
                int pixelX = static_cast<i32>(mousePos.x);
                int pixelY = static_cast<i32>(mousePos.y);

                // Get row pitch from texture mapping
                m_Data.hoveredEntity = pixelData[pixelY * (rowPitch / sizeof(uint32_t)) + pixelX];

                bool found = false;
                auto view = m_ActiveScene->registry->view<Transform>();
                for (entt::entity e : view)
                {
                    uint32_t eId = static_cast<uint32_t>(e);
                    if (eId == m_Data.hoveredEntity)
                    {
                        m_ScenePanel->SetSelectedEntity(Entity{ e, m_ActiveScene.get() });
                        found = true;
                        break;
                    }
                }

                if (!found && !m_Data.multiSelect)
                {
                    m_ScenePanel->SetSelectedEntity(Entity{});
                    m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::NONE);
                }

                m_Device->unmapStagingTexture(m_EntityIDStagingTexture);
            }

            m_Data.isPickingEntity = false;
        }
    }

    void EditorLayer::OnGuiRender()
    {
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::Begin("##main_dockspace", nullptr, windowFlags);
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        window->DC.LayoutType = ImGuiLayoutType_Horizontal;
        window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        if (ImGui::BeginMenuBar())
        {

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", nullptr, false, m_ActiveProject != nullptr))
                {
                    NewScene();
                }
                else if (ImGui::MenuItem("Open Scene", nullptr, false, m_ActiveProject != nullptr))
                {
                    OpenScene();
                }
                if (ImGui::MenuItem("Save Scene", nullptr, false, m_ActiveProject != nullptr))
                {
                    SaveScene();
                }
                else if (ImGui::MenuItem("Save Scene As", nullptr, false, m_ActiveProject != nullptr))
                {
                    SaveSceneAs();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("New Project"))
                {
                    m_Data.popupNewProjectModal = true;
                }

                else if (ImGui::MenuItem("Save Project", nullptr, false, m_ActiveProject != nullptr))
                {
                    SaveProject();
                }

                else if (ImGui::MenuItem("Open Project"))
                {
                    OpenProject();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Asset Registry", nullptr, false, m_ActiveProject != nullptr))
                {
                    m_Data.assetRegistryWindow = true;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
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

                    for (const auto [handle, metadata] : assetRegistry)
                    {
                        ImGui::Text("Handle: %" PRIu64, static_cast<uint64_t>(handle));
                        ImGui::Text("Type: %s", AssetTypeToString(metadata.type).c_str());
                        ImGui::Text("Filepath: %s", metadata.filepath.generic_string().c_str());
                    }
                }
            }

            ImGui::End();
            
            // Render GUI
            SettingsUI();
        }

        ImGui::End(); // end dockspace
    }

    void EditorLayer::NewScene()
    {
        if (m_EditorScene)
        {
            m_EditorScene->OnStop();
        }

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
        std::string filepath = FileDialogs::SaveFile("Ignite Scene (*.ixasset)\0*.ixasset\0");
        if (!filepath.empty())
        {
            m_CurrentSceneFilePath = filepath;
            SaveScene(filepath);
        }
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Ignite Scene (*.ixasset)\0*.ixasset\0");
        if (!filepath.empty())
        {
            OpenScene(filepath);
        }
    }

    bool EditorLayer::OpenScene(const std::filesystem::path &filepath)
    {
        if (m_EditorScene)
        {
            m_EditorScene->OnStop();
        }

        if (m_Data.sceneState == State::ScenePlay)
            OnSceneStop();

        if (Ref<Scene> openScene = SceneSerializer::Deserialize(filepath))
        {
            m_EditorScene = SceneManager::Copy(openScene);
            m_EditorScene->SetDirtyFlag(false);

            m_ActiveScene = m_EditorScene;
            m_ScenePanel->SetActiveScene(m_ActiveScene.get(), true);

            m_CurrentSceneFilePath = filepath;
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
        else
        {
            // Create default scene
            NewScene();
        }
        return true;
    }

    void EditorLayer::OnScenePlay()
    {
        if (m_EditorScene)
        {
            m_EditorScene->OnStop();
        }

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

        // copy initial components to new scene
        m_ActiveScene = SceneManager::Copy(m_EditorScene);
        m_ActiveScene->OnStart();

        m_ScenePanel->SetActiveScene(m_ActiveScene.get());
    }

    void EditorLayer::SettingsUI()
    {
        ImGui::Begin("Settings", &m_Data.settingsWindow);

        if (ImGui::TreeNodeEx("Pipeline"))
        {
            m_ScenePanel->CameraSettingsUI();

            ImGui::SeparatorText("Pipeline");

            // Raster settings
            static const char *rasterFillStr[2] = { "Solid", "Wireframe" };
            const char *currentFillMode = rasterFillStr[static_cast<i32>(m_Data.rasterFillMode)];
            if (ImGui::BeginCombo("Fill", currentFillMode))
            {
                for (size_t i = 0; i < std::size(rasterFillStr); ++i)
                {
                    bool isSelected = strcmp(currentFillMode, rasterFillStr[i]) == 0;
                    if (ImGui::Selectable(rasterFillStr[i], isSelected))
                    {
                        m_Data.rasterFillMode = static_cast<nvrhi::RasterFillMode>(i);
                        m_SceneRenderer.SetFillMode(m_Data.rasterFillMode);
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

        if (m_ActiveScene)
        {
            // Environment
            if (ImGui::TreeNodeEx("Environment"))
            {
                static glm::vec2 sunAngles = { 50, -27.0f }; // pitch (elevation), yaw (azimuth)

                if (ImGui::Button("Load HDR Texture"))
                {
                    std::string filepath = FileDialogs::OpenFile("HDR Files (*.hdr)\0*.hdr\0");
                    if (!filepath.empty())
                    {
                        EnvironmentImporter::UpdateTexture(&m_SceneRenderer.GetEnvironment(), filepath);
                    }
                }

                ImGui::Separator();
            
                ImGui::Text("Sun Angles");

                if (ImGui::SliderFloat("Elevation", &sunAngles.x, 0.0f, 180.0f))
                    m_SceneRenderer.GetEnvironment()->SetSunDirection(sunAngles.x, sunAngles.y);
                if (ImGui::SliderFloat("Azimuth", &sunAngles.y, -180.0f, 180.0f))
                    m_SceneRenderer.GetEnvironment()->SetSunDirection(sunAngles.x, sunAngles.y);

                ImGui::Separator();
                ImGui::ColorEdit4("Color", &m_SceneRenderer.GetEnvironment()->dirLight.color.x);
                ImGui::SliderFloat("Intensity", &m_SceneRenderer.GetEnvironment()->dirLight.intensity, 0.01f, 1.0f);

                float angularSize = glm::degrees(m_SceneRenderer.GetEnvironment()->dirLight.angularSize);
                if (ImGui::SliderFloat("Angular Size", &angularSize, 0.1f, 90.0f))
                {
                    m_SceneRenderer.GetEnvironment()->dirLight.angularSize = glm::radians(angularSize);
                }

                ImGui::DragFloat("Ambient", &m_SceneRenderer.GetEnvironment()->dirLight.ambientIntensity, 0.005f, 0.01f, 100.0f);
                ImGui::DragFloat("Exposure", &m_SceneRenderer.GetEnvironment()->params.exposure, 0.005f, 0.1f, 10.0f);
                ImGui::DragFloat("Gamma", &m_SceneRenderer.GetEnvironment()->params.gamma, 0.005f, 0.1f, 10.0f);
        
                ImGui::TreePop();
            }
        }

        ImGui::End();


        if (m_Data.assetRegistryWindow)
        {
            static std::unordered_set<std::string> registryFilter;
            static bool showFullPath = false;
            ImGui::Begin("Asset Registry", &m_Data.assetRegistryWindow);

            static char buffer[256];
            ImGui::Text("Filter");
            ImGui::SameLine();
            ImGui::InputText("##filter", buffer, sizeof(buffer));
            ImGui::SameLine();

            ImGui::Text("Full path");
            ImGui::SameLine();
            ImGui::Checkbox("##fullpath", &showFullPath);

            auto assetRegistry = m_ActiveProject->GetAssetManager().GetAssetAssetRegistry();
            ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
            if (ImGui::BeginTable("asset_registry_table", 3, tableFlags))
            {

                // setup table 3 columns
                // AssetHandle, Type, Filepath
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("UUID", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.5f);
                ImGui::TableSetupColumn("Filepath", ImGuiTableColumnFlags_WidthStretch, 1.5f);
                ImGui::TableHeadersRow();

                for (auto &[handle, metadata] : assetRegistry)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%llu", static_cast<uint64_t>(handle));

                    ImGui::TableNextColumn();

                    std::string assetTypeStr = AssetTypeToString(metadata.type);
                    ImGui::TextWrapped("%s", assetTypeStr.c_str());

                    ImGui::TableNextColumn();
                    if (showFullPath)
                    {
                        std::string assetTypeStr = std::filesystem::absolute(m_ActiveProject->GetAssetFilepath(metadata.filepath)).generic_string();
                        ImGui::TextWrapped("%s", assetTypeStr.c_str());
                    }
                    else
                    {
                        std::string assetTypeStr = metadata.filepath.generic_string();
                        ImGui::TextWrapped("%s", assetTypeStr.c_str());
                    }
                }


                ImGui::EndTable();
            }
            

            ImGui::End();
        }
    }
}
