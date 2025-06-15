#include "scene_panel.hpp"

#include "ignite/audio/fmod_sound.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_event.hpp"
#include "ignite/core/input/mouse_event.hpp"
#include "ignite/core/input/joystick_event.hpp"
#include "ignite/graphics/texture.hpp"
#include "ignite/scene/icomponent.hpp"
#include "ignite/core/platform_utils.hpp"
#include "editor_layer.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/animation/animation_system.hpp"

#include "ignite/scripting/script_engine.hpp"
#include "ignite/scripting/script_field.hpp"
#include "ignite/scripting/script_instance.hpp"

#include "entt/entt.hpp"
#include "../states.hpp"

#ifdef _WIN32
#   include <dwmapi.h>
#   include <ShellScalingApi.h>
#endif

#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>

#include "ignite/asset/asset_importer.hpp"

namespace ignite
{
    UUID ScenePanel::m_TrackingSelectedEntity = UUID(0);

    ScenePanel::ScenePanel(const char *windowTitle, EditorLayer *editor)
        : IPanel(windowTitle), m_Editor(editor)
    {
        Application *app = Application::GetInstance();

        m_Camera = EditorCamera("ScenePanel-Editor Camera");
        // m_Camera.CreateOrthographic(app->GetCreateInfo().width, app->GetCreateInfo().height, 8.0f, 0.1f, 350.0f);
        m_Camera.CreatePerspective(45.0f, 
            static_cast<float>(app->GetCreateInfo().width), 
            static_cast<float>(app->GetCreateInfo().height),
            0.1f,
            450.0f);

        m_Camera.position = {3.0f, 2.0f, 3.0f};
        m_Camera.yaw = -0.729f;
        m_Camera.pitch = 0.410f;

        m_Camera.UpdateViewMatrix();
        m_Camera.UpdateProjectionMatrix();


        // Load icons
        TextureCreateInfo createInfo;
        createInfo.format = nvrhi::Format::RGBA8_UNORM;
        m_Icons["simulate"] = Texture::Create("resources/ui/ic_simulate.png", createInfo);
        m_Icons["play"] = Texture::Create("resources/ui/ic_play.png", createInfo);
        m_Icons["stop"] = Texture::Create("resources/ui/ic_stop.png", createInfo);
        m_Icons["checker128"] = Texture::Create("resources/ui/checker-128px.jpg", createInfo);
    }

    void ScenePanel::SetActiveScene(Scene *scene, bool reset)
    {
        m_Scene = scene;
        m_SelectedEntities.clear();
    }

    void ScenePanel::CreateRenderTarget(nvrhi::IDevice *device)
    {
        RenderTargetCreateInfo createInfo = {};
        createInfo.device = device;
        createInfo.attachments = 
        {
            FramebufferAttachments{ nvrhi::Format::D32S8, nvrhi::ResourceStates::DepthWrite }, // Depth
            FramebufferAttachments{ nvrhi::Format::SRGBA8_UNORM, nvrhi::ResourceStates::RenderTarget }, // Main Color
            FramebufferAttachments{ nvrhi::Format::R32_UINT, nvrhi::ResourceStates::RenderTarget }, // Mouse picking
        };

        m_RenderTarget = CreateRef<RenderTarget>(createInfo);
    }

    void ScenePanel::OnGuiRender()
    {
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;
        ImGui::Begin(m_WindowTitle.c_str(), &m_IsOpen, windowFlags);
        ImGui::DockSpace(ImGui::GetID(m_WindowTitle.c_str()), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        ImGui::End();

        ImGui::ShowDemoWindow();

        if (m_Scene)
        {
            RenderHierarchy();
            RenderInspector();
        }
        
        RenderViewport();
        DebugRender();
    }

    void ScenePanel::OnUpdate(f32 deltaTime)
    {
        UpdateCameraInput(deltaTime);
    }

    void ScenePanel::RenderHierarchy()
    {
        ImGui::Begin("Hierarchy");
      
        ImGui::Button(m_Scene->name.c_str(), { ImGui::GetContentRegionAvail().x, 0.0f });

        // target drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
            {
                LOG_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ITEM, that should be an entity");
                Entity src{ *static_cast<entt::entity *>(payload->Data), m_Scene };
                ID &idComp = src.GetComponent<ID>();

                // check if src entity has parent
                if (idComp.parent != 0)
                {
                    // current parent should be removed it
                    Entity parent = SceneManager::GetEntity(m_Scene, idComp.parent);
                    parent.GetComponent<ID>().RemoveChild(idComp.uuid);

                    // reset the parent to 0
                    idComp.parent = UUID(0);
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::Text("Entity count: %zu", m_Scene->entities.size());

        ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;

        if (ImGui::BeginTable("entity_hierarchy_table", 3, tableFlags))
        {
            // setup table 3 columns
            // Name, Type, Active (check box)
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableHeadersRow();

            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, { 0.000f, 0.245f, 0.409f, 1.000f });
            ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.000f, 0.000f, 0.000f, 0.620f });
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 0.000f, 0.243f, 0.408f, 1.000f });
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2.0f, 0.0f });

            // Render root entity
            m_Scene->registry->view<ID>().each([&](entt::entity e, ID &id)
            {
                if (id.parent == UUID(0))
                    RenderEntityNode(Entity{ e, m_Scene }, id.uuid);
            });

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            // Context menu for creating entities
            if (ImGui::BeginPopupContextWindow("create_entity_context", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                ShowEntityContextMenu();
                ImGui::EndPopup();
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

    Entity ScenePanel::ShowEntityContextMenu()
    {
        Entity entity = {};
        if (ImGui::MenuItem("Empty"))
        {
            entity = SetSelectedEntity(SceneManager::CreateEntity(m_Scene, "Empty", EntityType_Node));
        }

        if (ImGui::BeginMenu("2D"))
        {
            if (ImGui::MenuItem("Sprite"))
            {
                entity = SetSelectedEntity(SceneManager::CreateSprite(m_Scene, "Sprite"));
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Camera"))
        {
            entity = SetSelectedEntity(SceneManager::CreateCamera(m_Scene, "Camera"));
        }

        return entity;
    }

    void ScenePanel::RenderEntityNode(Entity entity, UUID uuid)
    {
        if (!entity.IsValid())
            return;

        ID &idComp = entity.GetComponent<ID>();
        ImGuiTreeNodeFlags flags = (GetSelectedEntity() == entity ? ImGuiTreeNodeFlags_Selected : 0) | (!idComp.HasChild() ? ImGuiTreeNodeFlags_Leaf : 0)
            | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        void *imguiPushId = (void*)(uint64_t)(uint32_t)entity;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Header, { 0.000f, 0.305f, 0.453f, 1.000f });
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.435f, 0.287f, 0.000f, 1.000f });
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, { 0.780f, 0.520f, 0.000f, 1.000f });
        bool opened = ImGui::TreeNodeEx(imguiPushId, flags, idComp.name.c_str());
        ImGui::PopStyleColor(3);

        bool isDeleting = false;

        if (!m_Scene->IsPlaying() || true)
        {
            ImGui::PushID(imguiPushId);
            if (ImGui::BeginPopupContextItem(idComp.name.c_str()))
            {
                if (ImGui::BeginMenu("Create"))
                {
                    if (const Entity e = ShowEntityContextMenu())
                    {
                        SceneManager::AddChild(m_Scene, entity, e);
                        SetSelectedEntity(e);
                    }

                    ImGui::EndMenu();
                }
                
                if (ImGui::MenuItem("Delete"))
                {
                    DestroyEntity(entity);
                    isDeleting = true;

                    m_SelectedEntities.clear();
                }

                ImGui::EndPopup();
            }
            ImGui::PopID();
        }

        if (!isDeleting)
        {
            // drag and drop
            // drop source
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("ENTITY_SOURCE_ITEM", &entity, sizeof(Entity));

                ImGui::BeginTooltip();
                ImGui::Text("%s %llu", idComp.name.c_str(), u64(idComp.uuid));
                ImGui::EndTooltip();

                ImGui::EndDragDropSource();
            }

            // target drop
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
                {
                    LOG_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ITEM, that should be an entity");
                    Entity src { *static_cast<entt::entity *>(payload->Data), m_Scene };
                    
                    // the current 'entity' is the target (new parent for src)
                    SceneManager::AddChild(m_Scene, entity, src);
                }

                ImGui::EndDragDropTarget();
            }

            if (ImGui::IsItemHovered(ImGuiMouseButton_Left) && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                SetSelectedEntity(entity);
            }

            ImGui::PushID(imguiPushId);

            // second column
            ImGui::TableNextColumn();
            ImGui::TextWrapped(EntityTypeToString(idComp.type));

            // third column (last)
            ImGui::TableNextColumn();
            auto &tc = entity.GetComponent<Transform>();
            ImGui::Checkbox("##Active", &tc.visible);
            ImGui::PopID();
        }

        if (opened)
        {
            if (!isDeleting)
            {
                for (UUID uuid : entity.GetComponent<ID>().children)
                {
                    Entity childEntity = SceneManager::GetEntity(m_Scene, uuid);
                    RenderEntityNode(childEntity, uuid);
                }
            }

            ImGui::TreePop();
        }
    }

    void ScenePanel::RenderInspector()
    {
        ImGui::Begin("Inspector");

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        Entity selectedEntity = GetSelectedEntity();
        if (selectedEntity.IsValid())
        {
            // Main Component

            auto &comps = m_Scene->registeredComps[selectedEntity];

            // ID Component
            ID &idComp = selectedEntity.GetComponent<ID>();
            char buffer[255] = {};
            strncpy(buffer, idComp.name.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("##label", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                SceneManager::RenameEntity(m_Scene, selectedEntity, std::string(buffer));
            }

            ImGui::SameLine();

            if (ImGui::Button("Add Component", { ImGui::GetContentRegionAvail().x, 25.0f }))
            {
                ImGui::OpenPopup("##add_component_context");
            }

            // transform component
            RenderComponent<Transform>("Transform", selectedEntity, [&]()
            {
                Transform &comp = selectedEntity.GetComponent<Transform>();
                if (ImGui::DragFloat3("Translation", &comp.localTranslation.x, 0.025f))
                {
                    comp.dirty = true;
                }

                glm::vec3 euler = eulerAngles(comp.localRotation);
                if (ImGui::DragFloat3("Rotation", &euler.x, 0.025f))
                {
                    comp.localRotation = glm::quat(euler);
                    comp.dirty = true;
                }
                if (ImGui::DragFloat3("Scale", &comp.localScale.x, 0.025f))
                {
                    comp.dirty = true;
                }

            }, false); // false: not allowed to remove the component

            for (IComponent *comp : comps)
            {
                ImGui::PushID(&*comp);

                auto compType = comp->GetType();

                switch (compType)
                {
                case CompType_Sprite2D:
                {
                    RenderComponent<Sprite2D>("Sprite 2D", selectedEntity, [&]()
                    {
                        Sprite2D *c = comp->As<Sprite2D>();

                        std::string imageButtonLabel = "Load Texture";
                        if (c->texture)
                        {
                            imageButtonLabel = "Loaded";
                        }

                        if (ImGui::Button(imageButtonLabel.c_str()))
                        {
                            std::filesystem::path filepath = FileDialogs::OpenFile("JPG/JPEG (*.jpg;*jpeg)\0*.jpg;*jpeg\0PNG (*.png)\0*.png\0All Files (*.)\0*.*");
                            if (!filepath.empty())
                            {
                                TextureCreateInfo texCI;
                                texCI.flip = false;
                                texCI.format = nvrhi::Format::RGBA8_UNORM;
                                texCI.dimension = nvrhi::TextureDimension::Texture2D;
                                texCI.samplerMode = nvrhi::SamplerAddressMode::ClampToEdge;

                                c->texture = Texture::Create(filepath.generic_string(), texCI);
                            }
                        }

                        bool textureRemoved = false;

                        if (c->texture)
                        {
                            ImGui::SameLine();

                            if (ImGui::Button("X"))
                            {
                                c->texture.reset();
                                textureRemoved = true;
                            }

                            if (!textureRemoved)
                            {
                                ImGui::SameLine();
                                const std::filesystem::path &displayFilepath = c->texture->GetFilepath().filename();
                                ImGui::Text(displayFilepath.generic_string().c_str());
                            }
                        }

                        ImGui::DragFloat2("Tiling", &c->tilingFactor.x, 0.025f);
                        ImGui::ColorEdit4("Color", &c->color.x);
                    });

                    break;
                }
                case CompType_Rigidbody2D:
                {
                    RenderComponent<Rigidbody2D>("Rigid Body 2D", selectedEntity, [&]()
                    {
                        Rigidbody2D *c = comp->As<Rigidbody2D>();
                        
                        const char *bodyTypeStr[3] = { "Static", "Dynamic", "Kinematic" };
                        const char *currentBodyType = bodyTypeStr[static_cast<i32>(c->type)];
    
                        if (ImGui::BeginCombo("Body Type", currentBodyType))
                        {
                            for (i32 i = 0; i < std::size(bodyTypeStr); ++i)
                            {
                                if (ImGui::Selectable(bodyTypeStr[i]))
                                {
                                    c->type = static_cast<Body2DType>(i);
                                    break;
                                }
    
                                bool isSelected = (strcmp(bodyTypeStr[i], currentBodyType) == 0);
                                if (isSelected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
    
                        if (m_Scene->IsPlaying())
                        {
                            if (ImGui::DragFloat2("Linear Vel", &c->linearVelocity.x, 0.025f))
                                b2Body_SetLinearVelocity(c->bodyId, {c->linearVelocity.x, c->linearVelocity.y});
                                
                            if (ImGui::DragFloat("Angular Vel", &c->angularVelocity, 0.025f))
                                b2Body_SetAngularVelocity(c->bodyId, c->angularVelocity);
        
                            if (ImGui::DragFloat("Gravity", &c->gravityScale, 0.025f))
                                b2Body_SetGravityScale(c->bodyId, c->gravityScale);
        
        
                            if (ImGui::DragFloat("Linear Damping", &c->linearDamping, 0.025f))
                                b2Body_SetLinearDamping(c->bodyId, c->linearDamping);
        
                            if (ImGui::DragFloat("Angular Damping", &c->angularDamping, 0.025f))
                                b2Body_SetAngularDamping(c->bodyId, c->angularDamping);
        
        
                            if (ImGui::Checkbox("Fixed Rotation", &c->fixedRotation))
                                b2Body_SetFixedRotation(c->bodyId, c->fixedRotation);
        
                            if (ImGui::Checkbox("Awake", &c->isAwake))
                                b2Body_SetAwake(c->bodyId, c->isAwake);
        
                            if (ImGui::Checkbox("Enabled", &c->isEnabled))
                            {
                                c->isEnabled ? b2Body_Enable(c->bodyId) : b2Body_Disable(c->bodyId);
                            }
        
                            if (ImGui::Checkbox("Sleep", &c->isEnableSleep))
                                b2Body_EnableSleep(c->bodyId, c->isEnableSleep);
                        }
                        else
                        {
                            ImGui::DragFloat2("Linear Vel", &c->linearVelocity.x, 0.025f, FLT_MIN, FLT_MAX);
                            ImGui::DragFloat("Angular Vel", &c->angularVelocity, 0.025f, FLT_MIN, FLT_MAX);
                            ImGui::DragFloat("Gravity", &c->gravityScale, 0.025f, FLT_MIN, FLT_MAX);
                            ImGui::DragFloat("Linear Damping", &c->linearDamping, 0.0f, FLT_MAX);
                            ImGui::DragFloat("Angular Damping", &c->angularDamping, 0.025f, 0.0f, FLT_MAX);
                            ImGui::Checkbox("Fixed Rotation", &c->fixedRotation);
                            ImGui::Checkbox("Awake", &c->isAwake);
                            ImGui::Checkbox("Enabled", &c->isEnabled);
                            ImGui::Checkbox("Sleep", &c->isEnableSleep);
                        }
                    });

                    break;
                }
                case CompType_Camera:
                    {
                        RenderComponent<Camera>("Camera", selectedEntity, [&]()
                        {
                            Camera *c = comp->As<Camera>();

                            static const char *projectionTypeStr[] = {"Ortho", "Perspective"};
                            const char *currentProjectionTypeStr = projectionTypeStr[static_cast<int>(c->projectionType)];

                            if (ImGui::BeginCombo("Projection", currentProjectionTypeStr))
                            {
                                for (size_t i = 0; i < std::size(projectionTypeStr); ++i)
                                {
                                    bool selected = false;
                                    if (ImGui::Selectable(projectionTypeStr[i]))
                                    {
                                        c->projectionType = static_cast<ICamera::Type>(i);
                                        c->camera.projectionType = c->projectionType;
                                        c->camera.UpdateProjectionMatrix();

                                        selected = true;
                                    }
                                    
                                    if (selected)
                                    {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            bool modified = false;

                            if (c->projectionType == ICamera::Type::Perspective)
                            {
                                modified |= ImGui::DragFloat("Fov", &c->fov, 0.025f, 0.0f, FLT_MAX);
                            }
                            else
                            {
                                modified |= ImGui::DragFloat("Zoom", &c->zoom, 0.025f, 0.0f, FLT_MAX);
                            }
                            
                            modified |= ImGui::DragFloat("Near", &c->nearClip, 0.025f, 0.0f, FLT_MAX);
                            modified |= ImGui::DragFloat("Far", &c->farClip, 0.025f, 0.0f, FLT_MAX);
                            modified |= ImGui::Checkbox("Primary", &c->primary);

                            if (modified)
                            {
                                c->camera.fov = c->fov;
                                c->camera.nearClip = c->nearClip;
                                c->camera.farClip = c->farClip;
                                c->camera.zoom = c->zoom;
                                c->camera.UpdateProjectionMatrix();
                            }
                        });

                        break;
                    }
                case CompType_BoxCollider2D:
                {
                    RenderComponent<BoxCollider2D>("Box Collider 2D", selectedEntity, [&]()
                    {
                        BoxCollider2D *c = comp->As<BoxCollider2D>();
                        ImGui::DragFloat2("Size", &c->size.x, 0.025f, 0.0f, FLT_MAX);
                        ImGui::DragFloat2("Offset", &c->offset.x, 0.025f, 0.0f, FLT_MAX);
                        ImGui::DragFloat("Restitution", &c->restitution, 0.025f, 0.0f, FLT_MAX);
                        ImGui::DragFloat("Friction", &c->friction, 0.025f, 0.0f, FLT_MAX);
                        ImGui::DragFloat("Density", &c->density, 0.025f);
                        ImGui::Checkbox("Is Sensor", &c->isSensor);
                    });

                    break;
                }
                case CompType_Rigidbody:
                {
                    RenderComponent<Rigibody>("Rigid Body", selectedEntity, [&]()
                    {
                        Rigibody *c = comp->As<Rigibody>();
                        ImGui::Checkbox("Static", &c->isStatic);

                    });
                    break;
                }
                case CompType_BoxCollider:
                {
                    RenderComponent<BoxCollider>("Box Collider", selectedEntity, [&]()
                    {
                        BoxCollider *c = comp->As<BoxCollider>();
                        ImGui::DragFloat3("Scale", &c->scale.x, 0.025f, 0.0f, 10000.0f);
                        ImGui::DragFloat("Friction", &c->friction, 0.025f);
                        ImGui::DragFloat("Static Friction", &c->staticFriction, 0.025f);
                        ImGui::DragFloat("Restitution", &c->restitution, 0.025f);
                        ImGui::DragFloat("Density", &c->density, 0.025f);
                    });
                    break;
                }
                case CompType_SphereCollider:
                {
                    RenderComponent<SphereCollider>("Sphere Collider", selectedEntity, [&]()
                        {
                            SphereCollider *c = comp->As<SphereCollider>();
                            ImGui::DragFloat("Radius", &c->radius, 0.025f, 0.01f, 10000.0f);
                            ImGui::DragFloat("Friction", &c->friction, 0.025f);
                            ImGui::DragFloat("Static Friction", &c->staticFriction, 0.025f);
                            ImGui::DragFloat("Restitution", &c->restitution, 0.025f);
                            ImGui::DragFloat("Density", &c->density, 0.025f);
                        });
                    break;
                }
                case CompType_AudioSource:
                {
                    RenderComponent<AudioSource>("Audio Source", selectedEntity, [&]()
                        {
                            AudioSource *c = comp->As<AudioSource>();

                            ImGui::Button("Drag Here", { 65, 30.0f });

                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("content_browser_item"))
                                {
                                    if (payload->DataSize == sizeof(AssetHandle))
                                    {
                                        AssetHandle *handle = static_cast<AssetHandle *>(payload->Data);
                                        if (handle && *handle != AssetHandle(0))
                                        {
                                            AssetMetaData metadata = Project::GetActive()->GetAssetManager().GetMetaData(*handle);
                                            if (metadata.type == AssetType::Audio)
                                            {
                                                c->handle = *handle;
                                                Ref<FmodSound> sound = Project::GetAsset<FmodSound>(*handle);
                                            }
                                        }
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }

                            ImGui::SameLine();
                            ImGui::Text("%llu", c->handle);

                            if (c->handle != AssetHandle(0))
                            {
                                if (Ref<FmodSound> sound = Project::GetAsset<FmodSound>(c->handle))
                                {
                                    if (ImGui::Button("Play", { 55.0f, 30.0f }))
                                    {
                                        sound->Stop();

                                        sound->Play();
                                        sound->SetVolume(c->volume);
                                        sound->SetPitch(c->pitch);
                                        sound->SetPan(c->pan);
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::Button("Stop", { 55.0f, 30.0f }))
                                    {
                                        sound->Stop();
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::Button("Pause", { 55.0f, 30.0f }))
                                    {
                                        sound->Pause();
                                    }
                                    if (sound->IsPaused())
                                    {
                                        ImGui::SameLine();
                                        if (ImGui::Button("Resume", { 55.0f, 30.0f }))
                                        {
                                            sound->Resume();
                                        }
                                    }

                                    ImGui::DragFloat("Volume", &c->volume, 0.001f, 0.0f, 1.0f);
                                    ImGui::DragFloat("Pitch", &c->pitch, 0.001f);
                                    ImGui::DragFloat("Pan", &c->pan, 0.001f);
                                    ImGui::Checkbox("Play On Start", &c->playOnStart);
                                }
                            }
                        }
                    );

                    break;
                }
                case CompType_Script:
                {
                    RenderComponent<Script>("C# Script", selectedEntity, [&]()
                    {
                        Script *c = comp->As<Script>();

                        bool scriptClassExist = ScriptEngine::EntityClassExists(c->className);
                        bool is_selected = false;

                        if (!scriptClassExist)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                        }

                        auto scriptStorage = ScriptEngine::GetScriptClassStorage();
                        std::string currentScriptClasses = c->className;

                        // drop-down
                        if (ImGui::BeginCombo("Script Class", currentScriptClasses.c_str()))
                        {
                            for (int i = 0; i < scriptStorage.size(); i++)
                            {
                                is_selected = currentScriptClasses == scriptStorage[i];
                                if (ImGui::Selectable(scriptStorage[i].c_str(), is_selected))
                                {
                                    currentScriptClasses = scriptStorage[i];
                                    c->className = scriptStorage[i];
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        if (ImGui::Button("Detach"))
                        {
                            c->className = "Detached";
                            is_selected = false;
                        }

                        bool detached = c->className == "Detached";

                        // classFields
                        bool isRunning = m_Scene->IsPlaying();

                        // Editable classFields (edit mode)
                        if (isRunning && !detached)
                        {
                            Ref<ScriptInstance> scriptInstance = ScriptEngine::GetEntityScriptInstance(selectedEntity.GetUUID());
                            if (scriptInstance)
                            {
                                auto classFields = scriptInstance->GetScriptClass()->GetFields();

                                for (const auto &[fieldName, field] : classFields)
                                {
                                    switch (field.Type)
                                    {
                                    case ScriptFieldType::Float:
                                    {
                                        float data = scriptInstance->GetFieldValue<float>(fieldName);
                                        if (ImGui::DragFloat(fieldName.c_str(), &data, 0.1f))
                                        {
                                            scriptInstance->SetFieldValue<float>(fieldName, data);
                                        }
                                        break;
                                    }
                                    case ScriptFieldType::Int:
                                    {
                                        int data = scriptInstance->GetFieldValue<int>(fieldName);
                                        if (ImGui::DragInt(fieldName.c_str(), &data, 1))
                                        {
                                            scriptInstance->SetFieldValue<int>(fieldName, data);
                                        }
                                        break;
                                    }
                                    case ScriptFieldType::Vector2:
                                    {
                                        glm::vec2 data = scriptInstance->GetFieldValue<glm::vec2>(fieldName);
                                        if (ImGui::DragFloat2(fieldName.c_str(), &data.x, 0.1f))
                                        {
                                            scriptInstance->SetFieldValue<glm::vec2>(fieldName, data);
                                        }
                                        break;
                                    }
                                    case ScriptFieldType::Vector3:
                                    {
                                        glm::vec3 data = scriptInstance->GetFieldValue<glm::vec3>(fieldName);
                                        if (ImGui::DragFloat3(fieldName.c_str(), &data.x, 0.1f))
                                        {
                                            scriptInstance->SetFieldValue<glm::vec3>(fieldName, data);
                                        }
                                        break;
                                    }
                                    case ScriptFieldType::Vector4:
                                    {
                                        glm::vec4 data = scriptInstance->GetFieldValue<glm::vec4>(fieldName);
                                        if (ImGui::DragFloat4(fieldName.c_str(), &data.x, 0.1f))
                                        {
                                            scriptInstance->SetFieldValue<glm::vec4>(fieldName, data);
                                        }
                                        break;
                                    }
                                    case ScriptFieldType::Entity:
                                    {
                                        uint64_t uuid = scriptInstance->GetFieldValue<uint64_t>(fieldName);
                                        if (Entity entity = SceneManager::GetEntity(m_Scene, UUID(uuid)))
                                        {
                                            ImGui::Button(fieldName.c_str());
                                            if (ImGui::IsItemHovered())
                                            {
                                                ImGui::BeginTooltip();
                                                ImGui::Text("%llu", uuid);
                                                ImGui::EndTooltip();
                                            }
                                        }
                                        break;
                                    }
                                    }
                                }
                            }
                        }

                        // Scene is running, we don't have to set anything
                        else if (!isRunning && scriptClassExist && !detached)
                        {
                            // !IsRunning
                            Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClassesByName(c->className);
                            if (entityClass)
                            {
                                const auto &classFields = entityClass->GetFields();
                                auto &entityFields = ScriptEngine::GetScriptFieldMap(selectedEntity);

                                for (const auto &[name, field] : classFields)
                                {
                                    if (entityFields.find(name) != entityFields.end())
                                    {
                                        ScriptFieldInstance &scriptField = entityFields.at(name);

                                        switch (field.Type)
                                        {
                                        case ScriptFieldType::Float:
                                        {
                                            float data = scriptField.GetValue<float>();
                                            if (ImGui::DragFloat(name.c_str(), &data, 0.1f))
                                            {
                                                scriptField.SetValue<float>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Int:
                                        {
                                            int data = scriptField.GetValue<int>();
                                            if (ImGui::DragInt(name.c_str(), &data))
                                            {
                                                scriptField.SetValue<int>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Vector2:
                                        {
                                            glm::vec2 data = scriptField.GetValue<glm::vec3>();
                                            if (ImGui::DragFloat2(name.c_str(), &data.x, 0.1f))
                                            {
                                                scriptField.SetValue<glm::vec2>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Vector3:
                                        {
                                            glm::vec3 data = scriptField.GetValue<glm::vec3>();
                                            if (ImGui::DragFloat3(name.c_str(), &data.x, 0.1f))
                                            {
                                                scriptField.SetValue<glm::vec3>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Vector4:
                                        {
                                            glm::vec4 data = scriptField.GetValue<glm::vec4>();
                                            if (ImGui::DragFloat4(name.c_str(), &data.x, 0.1f))
                                            {
                                                scriptField.SetValue<glm::vec4>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Entity:
                                        {
                                            uint64_t uuid = scriptField.GetValue<uint64_t>();
                                            std::string label = "Drag Here";
                                            if (uuid)
                                            {
                                                Entity e = SceneManager::GetEntity(m_Scene, UUID(uuid));
                                                if (e)
                                                {
                                                    label = e.GetName();
                                                }
                                            }

                                            ImGui::Button(label.c_str());

                                            if (ImGui::BeginDragDropTarget())
                                            {
                                                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
                                                {
                                                    LOG_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ENTITY ITEM");
                                                    if (payload->DataSize == sizeof(Entity))
                                                    {
                                                        Entity src{ *static_cast<entt::entity *>(payload->Data), m_Scene };
                                                        uint64_t id = (uint64_t)src.GetUUID();
                                                        scriptField.Field.Type = ScriptFieldType::Entity;
                                                        scriptField.SetValue<uint64_t>(id);
                                                    }
                                                }
                                                ImGui::EndDragDropTarget();
                                            }

                                            if (ImGui::IsItemHovered())
                                            {
                                                ImGui::BeginTooltip();

                                                if (uuid)
                                                    ImGui::Text("%llu", uuid);
                                                else
                                                    ImGui::Text("Null Entity!");

                                                ImGui::EndTooltip();
                                            }

                                            ImGui::SameLine();
                                            if (ImGui::Button("X"))
                                            {
                                                scriptField.SetValue<uint64_t>(0);
                                            }
                                            break;
                                        }
                                        }
                                    }
                                    else
                                    {
                                        ScriptFieldInstance &fieldInstance = entityFields[name];
                                        switch (field.Type)
                                        {
                                        case ScriptFieldType::Float:
                                        {
                                            float data = 0.0f;
                                            if (ImGui::DragFloat(name.c_str(), &data, 0.1f))
                                            {
                                                fieldInstance.Field = field;
                                                fieldInstance.SetValue<float>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Int:
                                        {
                                            int data = 0;
                                            if (ImGui::DragInt(name.c_str(), &data))
                                            {
                                                fieldInstance.Field = field;
                                                fieldInstance.SetValue<int>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Vector2:
                                        {
                                            glm::vec2 data(0.0f);
                                            if (ImGui::DragFloat2(name.c_str(), &data.x, 0.1f))
                                            {
                                                fieldInstance.Field = field;
                                                fieldInstance.SetValue<glm::vec2>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Vector3:
                                        {
                                            glm::vec3 data(0.0f);
                                            if (ImGui::DragFloat3(name.c_str(), &data.x, 0.1f))
                                            {
                                                fieldInstance.Field = field;
                                                fieldInstance.SetValue<glm::vec3>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Vector4:
                                        {
                                            glm::vec4 data(0.0f);
                                            if (ImGui::DragFloat4(name.c_str(), &data.x, 0.1f))
                                            {
                                                fieldInstance.Field = field;
                                                fieldInstance.SetValue<glm::vec4>(data);
                                            }
                                            break;
                                        }
                                        case ScriptFieldType::Entity:
                                        {
                                            ImGui::Button("Drag Here");

                                            if (ImGui::BeginDragDropTarget())
                                            {
                                                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
                                                {
                                                    LOG_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ENTITY ITEM");
                                                    if (payload->DataSize == sizeof(Entity))
                                                    {
                                                        Entity src{ *static_cast<entt::entity *>(payload->Data), m_Scene };
                                                        fieldInstance.Field = field;
                                                        fieldInstance.SetValue<uint64_t>(src.GetUUID());
                                                    }
                                                }
                                                ImGui::EndDragDropTarget();
                                            }

                                            if (ImGui::IsItemHovered())
                                            {
                                                ImGui::BeginTooltip();
                                                ImGui::Text("Null Entity!");
                                                ImGui::EndTooltip();
                                            }

                                            ImGui::SameLine();
                                            if (ImGui::Button("X"))
                                            {
                                                fieldInstance.Field = field;
                                                fieldInstance.SetValue<uint64_t>(0);
                                            }

                                            break;
                                        }
                                        default: break;
                                        }
                                    }
                                }
                            }
                        }

                        if (!scriptClassExist)
                        {
                            ImGui::PopStyleColor();
                        }
                    });
                    break;
                }
                case CompType_MeshRenderer:
                {
                    RenderComponent<MeshRenderer>("Mesh Renderer", selectedEntity, [&]()
                    {
                        MeshRenderer *c = comp->As<MeshRenderer>();
                        
                        ImGui::Text("Mesh [%d]: %s", c->meshIndex, c->mesh ? c->mesh->name.c_str() : "<null>");
                        if (c->meshIndex != -1)
                        {
                            const ImVec2 imageSize = { 72.0f, 72.0f };

                            ImGui::ColorEdit4("Base Color", &c->mesh->material.data.baseColor.x);
                            
                            auto checkerTex = m_Icons["checker128"]->GetHandle().Get();
                            auto baseColorTex = c->mesh->material.textures[aiTextureType_BASE_COLOR].handle ? c->mesh->material.textures[aiTextureType_BASE_COLOR].handle.Get() : checkerTex;
                            ImTextureID texId = reinterpret_cast<ImTextureID>(baseColorTex);
                            if (ImGui::ImageButton("Base Color Texture", texId, imageSize))
                            {
                                const std::filesystem::path filepath = FileDialogs::OpenFile("Image Files (*.jpg,*.jpeg,*.png)\0*.jpg;*.jpeg;*.png");
                                if (!filepath.empty())
                                {
                                    TextureCreateInfo createInfo;
                                    createInfo.format = nvrhi::Format::RGBA8_UNORM;
                                    Ref<Texture> texture = Texture::Create(filepath, createInfo);
                                    c->mesh->UpdateTexture(texture, aiTextureType_BASE_COLOR);
                                }
                            }

                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("content_browser_item"))
                                {
                                    if (payload->DataSize == sizeof(AssetHandle))
                                    {
                                        AssetHandle *handle = static_cast<AssetHandle *>(payload->Data);
                                        if (handle && *handle != AssetHandle(0))
                                        {
                                            AssetMetaData metadata = Project::GetActive()->GetAssetManager().GetMetaData(*handle);
                                            if (metadata.type == AssetType::Texture)
                                            {
                                                Ref<Texture> texture = Project::GetAsset<Texture>(*handle);
                                                c->mesh->UpdateTexture(texture, aiTextureType_BASE_COLOR);
                                            }
                                        }
                                    }
                                }

                                ImGui::EndDragDropTarget();
                            }

                            ImGui::DragFloat("Metallic", &c->mesh->material.data.metallic, 0.025f, 0.0f, 1.0f);
                            auto metalTex = c->mesh->material.textures[aiTextureType_SPECULAR].handle ? c->mesh->material.textures[aiTextureType_SPECULAR].handle.Get() : checkerTex;
                            texId = reinterpret_cast<ImTextureID>(metalTex);
                            if (ImGui::ImageButton("Specular Texture", texId, imageSize))
                            {
                                const std::filesystem::path filepath = FileDialogs::OpenFile("Image Files (*.jpg,*.jpeg,*.png)\0*.jpg;*.jpeg;*.png");
                                if (!filepath.empty())
                                {
                                    TextureCreateInfo createInfo;
                                    createInfo.format = nvrhi::Format::RGBA8_UNORM;
                                    Ref<Texture> texture = Texture::Create(filepath, createInfo);
                                    c->mesh->UpdateTexture(texture, aiTextureType_SPECULAR);
                                }
                            }

                            ImGui::DragFloat("Roughness", &c->mesh->material.data.roughness, 0.025f, 0.0f, 1.0f);
                            auto roughnessTex = c->mesh->material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle ? c->mesh->material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle.Get() : checkerTex;
                            texId = reinterpret_cast<ImTextureID>(roughnessTex);
                            if (ImGui::ImageButton("Roughness Texture", texId, imageSize))
                            {
                                const std::filesystem::path filepath = FileDialogs::OpenFile("Image Files (*.jpg,*.jpeg,*.png)\0*.jpg;*.jpeg;*.png");
                                if (!filepath.empty())
                                {
                                    TextureCreateInfo createInfo;
                                    createInfo.format = nvrhi::Format::RGBA8_UNORM;
                                    Ref<Texture> texture = Texture::Create(filepath, createInfo);
                                    c->mesh->UpdateTexture(texture, aiTextureType_DIFFUSE_ROUGHNESS);
                                }
                            }

                            ImGui::DragFloat("Emissive", &c->mesh->material.data.emissive, 0.025f, 0.0f, 1.0f);
                            auto emmisiveTex = c->mesh->material.textures[aiTextureType_EMISSIVE].handle ? c->mesh->material.textures[aiTextureType_EMISSIVE].handle.Get() : checkerTex;
                            texId = reinterpret_cast<ImTextureID>(emmisiveTex);
                            if (ImGui::ImageButton("Emissive Texture", texId, imageSize))
                            {
                                const std::filesystem::path filepath = FileDialogs::OpenFile("Image Files (*.jpg,*.jpeg,*.png)\0*.jpg;*.jpeg;*.png");
                                if (!filepath.empty())
                                {
                                    TextureCreateInfo createInfo;
                                    createInfo.format = nvrhi::Format::RGBA8_UNORM;
                                    Ref<Texture> texture = Texture::Create(filepath, createInfo);
                                    c->mesh->UpdateTexture(texture, aiTextureType_EMISSIVE);
                                }
                            }
                        }
                    });
                    break;
                }
                case CompType_SkinnedMesh:
                {
                    RenderComponent<SkinnedMesh>("Skinned Mesh", selectedEntity, [&]()
                    {
                        SkinnedMesh *c = comp->As<SkinnedMesh>();

                        std::string btnLabel = c->filepath.empty() ? "Load" : "Loaded";
                        if (ImGui::Button(btnLabel.c_str(), {55.0f, 30.0f}))
                        {
                            std::filesystem::path filepath = FileDialogs::OpenFile("GLB/GLTF (*.glb;*.gltf)\0*.glb;*.gltf\0FBX (*.fbx)\0*.fbx");
                            if (!filepath.empty())
                            {
                                AssetImporter::LoadSkinnedMesh(m_Scene, selectedEntity, filepath);
                            }
                        }

                        ImGui::SameLine();
                        ImGui::Text("%s", c->filepath.generic_string().c_str());

                        if (!c->animations.empty())
                        {
                            ImGui::SeparatorText("Animations");
                            
                            if (ImGui::Button("Play"))
                            {
                                SkeletalAnimation &anim = c->animations[c->activeAnimIndex];
                                anim.isPlaying = true;
                            }

                            ImGui::SameLine();

                            if (ImGui::Button("Stop"))
                            {
                                SkeletalAnimation &anim = c->animations[c->activeAnimIndex];
                                anim.isPlaying = false;
                            }

#if 0
                            ImGui::SameLine();

                            if (ImGui::Button("Save Animations"))
                            {
                                std::filesystem::path directory = FileDialogs::SelectFolder();
                                if (!directory.empty())
                                {
                                    for (auto &anim : c->animations)
                                    {
                                        std::filesystem::path filepath = directory / std::format("anim_{0}.anim", anim.name);
                                        AnimationSerializer sr(anim);
                                        sr.Serialize(filepath);
                                    }
                                }
                            }
#endif

                            if (ImGui::TreeNodeEx("Animations", 0))
                            {
                                for (size_t animIdx = 0; animIdx < c->animations.size(); ++animIdx)
                                {
                                    const SkeletalAnimation &anim = c->animations[animIdx];
                                    if (ImGui::TreeNodeEx(anim.name.c_str(), ImGuiTreeNodeFlags_Leaf, "%s", anim.name.c_str()))
                                    {
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                        {
                                            bool play = false;
                                            if (c->animations[c->activeAnimIndex].isPlaying)
                                                play = true;
                                            
                                            c->activeAnimIndex = static_cast<int>(animIdx);
                                            c->animations[c->activeAnimIndex].isPlaying = play;
                                        }
                                        ImGui::TreePop();
                                    }
                                }
                                ImGui::TreePop();
                            }
                        }
                    });
                    break;
                }
                }

                ImGui::PopID();
            }

            if (ImGui::BeginPopup("##add_component_context", ImGuiWindowFlags_NoDecoration))
            {
                static char buffer[256] = { 0 };
                static std::string compNameFilterResultStr;
                static std::set<std::pair<std::string, CompType>> filteredCompName;

                ImGui::InputTextWithHint("##component_name", "Component", buffer, sizeof(buffer) + 1, ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_NoHorizontalScroll);

                compNameFilterResultStr = std::string(buffer);

                filteredCompName.clear();

                if (!compNameFilterResultStr.empty())
                {
                    std::string search = stringutils::ToLower(compNameFilterResultStr);
                    for (const auto &[strName, type] : s_ComponentsName)
                    {
                        bool found = false;
                        for (IComponent *comp : comps)
                        {
                            if (comp->GetType() == type)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            continue;

                        std::string nameLower = stringutils::ToLower(strName);
                        if (nameLower.find(search) != std::string::npos)
                        {
                            filteredCompName.insert({ strName, type });
                        }
                    }
                }

                static std::function addCompFunc = [=](Entity entity, CompType type)
                {
                    switch (type)
                    {
                    case CompType_Camera:
                        entity.AddComponent<Camera>();
                        break;
                    case CompType_Sprite2D:
                        entity.AddComponent<Sprite2D>();
                        break;
                    case CompType_Rigidbody2D:
                        entity.AddComponent<Rigidbody2D>();
                        break;
                    case CompType_BoxCollider2D:
                        entity.AddComponent<BoxCollider2D>();
                        break;
                    case CompType_MeshRenderer:
                        entity.AddComponent<MeshRenderer>();
                        break;
                    case CompType_SkinnedMesh:
                        entity.AddComponent<SkinnedMesh>();
                        break;
                    case CompType_Rigidbody:
                        entity.AddComponent<Rigibody>();
                        break;
                    case CompType_BoxCollider:
                        entity.AddComponent<BoxCollider>();
                        break;
                    case CompType_SphereCollider:
                        entity.AddComponent<SphereCollider>();
                        break;
                    case CompType_AudioSource:
                        entity.AddComponent<AudioSource>();
                        break;
                    case CompType_Script:
                        entity.AddComponent<Script>();
                        break;
                    default: break;
                    }
                };

                if (compNameFilterResultStr.empty())
                {
                    for (const auto &[strName, type] : s_ComponentsName)
                    {
                        bool found = false;
                        for (IComponent *comp : comps)
                        {
                            if (comp->GetType() == type)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found)
                        {
                            continue;
                        }

                        if (ImGui::Selectable(strName.c_str()))
                        {
                            addCompFunc(Entity{ selectedEntity, m_Scene }, type);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                for (const auto &[strName, type] : filteredCompName)
                {
                    if (ImGui::Selectable(strName.c_str()))
                    {
                        addCompFunc(Entity{ selectedEntity, m_Scene }, type);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

    void ScenePanel::RenderViewport()
    {
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (m_Scene && m_Scene->IsDirty())
        {
            windowFlags |= ImGuiWindowFlags_UnsavedDocument;
        }

        ImGui::Begin("Viewport", nullptr, windowFlags);

        const ImGuiWindow *window = ImGui::GetCurrentWindow();

        m_IsFocused = ImGui::IsWindowFocused();
        m_IsHovered = ImGui::IsWindowHovered();

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionMax();

        m_ViewportData.rect.min = { canvasPos.x, canvasPos.y };
        m_ViewportData.rect.max = { canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y };

        uint32_t vpWidth = static_cast<uint32_t>(m_ViewportData.rect.GetSize().x);
        uint32_t vpHeight = static_cast<uint32_t>(m_ViewportData.rect.GetSize().y);

        // Mouse position in screen space
        ImVec2 mousePos = ImGui::GetMousePos();

        // Mouse position relative to viewport
        glm::vec2 localMouse = { mousePos.x - canvasPos.x, mousePos.y - canvasPos.y };

        // Clamp to valid range [0, size - 1]
        localMouse = glm::clamp(localMouse, glm::vec2(0.0f), glm::vec2(vpWidth - 1, vpHeight - 1));

        m_ViewportData.mousePos = localMouse;

        // trigger resize
        if (vpWidth > 0 && vpHeight > 0 && (vpWidth != m_RenderTarget->GetWidth() || vpHeight != m_RenderTarget->GetHeight()))
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_RenderTarget->Resize(vpWidth, vpHeight);
                m_RenderTarget->CreateSingleFramebuffer();

                m_Camera.SetSize(static_cast<float>(vpWidth), static_cast<float>(vpHeight));
                m_Camera.UpdateProjectionMatrix();

                if (m_Scene)
                {
                    m_Scene->OnResize(vpWidth, vpHeight);
                }
            }
        }

        const ImTextureID imguiTex = reinterpret_cast<ImTextureID>(m_RenderTarget->GetColorAttachment(0).Get());
        ImGui::Image(imguiTex, window->Size);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("content_browser_item"))
            {
                if (payload->DataSize == sizeof(AssetHandle))
                {
                    AssetHandle *handle = static_cast<AssetHandle *>(payload->Data);
                    if (handle && *handle != AssetHandle(0))
                    {
                        AssetMetaData metadata = Project::GetActive()->GetAssetManager().GetMetaData(*handle);
                        if (metadata.type == AssetType::Scene)
                        {
                            std::filesystem::path filepath = Project::GetActive()->GetAssetFilepath(metadata.filepath);
                            m_Editor->OpenScene(filepath);
                        }
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        GizmoInfo gizmoInfo;
        gizmoInfo.cameraView = m_Camera.viewMatrix;
        gizmoInfo.cameraProjection = m_Camera.projectionMatrix;
        gizmoInfo.cameraType = m_Camera.projectionType;

        gizmoInfo.viewRect = m_ViewportData.rect;

        m_Gizmo.SetInfo(gizmoInfo);

        // Start manipulation: Fired only on the first frame of interaction
        bool isManipulatingNow = m_Gizmo.IsManipulating();

        static std::unordered_map<UUID, Transform> initialTransforms;

        if (isManipulatingNow && !m_Data.isGizmoManipulating)
        {
            initialTransforms.clear();
            for (auto [uuid, entity]: m_SelectedEntities)
            {
                // Store the original transform of each selected entity
                initialTransforms[uuid] = entity.GetTransform();
            }
        }
        // Set the master flag for the current frame
        m_Data.isGizmoManipulating = isManipulatingNow;
        m_Data.isGizmoBeingUse = isManipulatingNow || m_Gizmo.IsHovered();

        if (m_SelectedEntities.size() > 1)
        {
            // Step 1: Compute shared pivot (center of all selected entities)
            glm::vec3 pivot(0.0f);
            for (Entity entity : m_SelectedEntities | std::views::values)
            {
                pivot += entity.GetTransform().translation;
            }
            pivot /= static_cast<float>(m_SelectedEntities.size());
     
            // Step 2: create a transform matrix for the gizmo at the pivot point
            glm::mat4 gizmoTransform = glm::translate(glm::mat4(1.0f), pivot);
            glm::mat4 manipulatedTransform = gizmoTransform; // This will be modified by the gizmo
     
            // Step 3: Manipulate the matrix
            m_Gizmo.Manipulate(manipulatedTransform);

            if (m_Data.isGizmoManipulating)
            {
                // THis delta is now the TOTAL change from the moment of manipulation began
                glm::mat4 gizmoDelta = glm::inverse(gizmoTransform) * manipulatedTransform;

                // Decompose the total delta
                glm::vec3 deltaTranslation, deltaScale, deltaRotation;
                Math::DecomposeTransformEuler(gizmoDelta , deltaTranslation, deltaRotation, deltaScale);
                
                for (auto [uuid, entity] : m_SelectedEntities)
                {
                    // Get the live transform component to apply changes to it
                    Transform &tr = entity.GetTransform();
     
                    // Get the ORIGINAL transform we stored at the beginning of the manipulation
                    const Transform &initialTransform = initialTransforms.at(uuid);
                    glm::mat4 initialWorldMatrix = initialTransform.GetWorldMatrix();

                    // Apply Translation and Rotation around the shared pivot
                    glm::mat4 toPivot = glm::translate(glm::mat4(1.0f), -pivot);
                    glm::mat4 fromPivot = glm::translate(glm::mat4(1.0f), pivot);
                    glm::mat4 noScaleDelta = Math::RemoveScale(gizmoDelta);

                    // Apply the total delta to the ORIGINAL world matrix
                    glm::mat4 newWorldMatrix = fromPivot * noScaleDelta * toPivot * tr.GetWorldMatrix();
                    glm::vec3 newTranslation, newRotationEuler, newScale;
                    Math::DecomposeTransformEuler(newWorldMatrix, newTranslation, newRotationEuler, newScale);
                    
                    // ----- Apply Scale and Update Local Transform -----
                    if (entity.GetParentUUID() != UUID(0))
                    {
                        Entity parent = SceneManager::GetEntity(m_Scene, entity.GetParentUUID());
                        const Transform &parentTr = parent.GetTransform();
                        glm::mat4 parentWorld = parentTr.GetWorldMatrix();
                        glm::mat4 localMatrix = glm::inverse(parentWorld) * newWorldMatrix;

                        glm::vec3 localTranslation, localEuler, localScale;
                        Math::DecomposeTransformEuler(localMatrix, localTranslation, localEuler, localScale);
                        tr.localTranslation = localTranslation;
                        tr.localRotation = glm::quat(localEuler);

                        // Apply the total scale delta to the ORIGINAL local scale
                        tr.localScale = initialTransform.localScale * deltaScale;
                    }
                    else
                    {
                        tr.localTranslation = newTranslation;
                        tr.localRotation = glm::quat(newRotationEuler);

                        // Apply the total scale delta to the ORIGINAL local scale
                        tr.localScale = initialTransform.localScale * deltaScale;
                    }
                    tr.dirty = true;
                }
            }
        }
        else if (Entity entity = GetSelectedEntity())
        {
            Transform &tr = entity.GetTransform();
            glm::mat4 transformMatrix = tr.GetWorldMatrix();

            m_Gizmo.Manipulate(transformMatrix);

            if (m_Gizmo.IsManipulating())
            {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransformEuler(transformMatrix, translation, rotation, scale);

                if (entity.GetParentUUID() != UUID(0))
                {
                    Entity parent = SceneManager::GetEntity(m_Scene, entity.GetParentUUID());
                    const Transform &parentTr = parent.GetTransform();
                    glm::vec4 localTranslation = glm::inverse(parentTr.GetWorldMatrix()) * glm::vec4(translation, 1.0f);
                    tr.localTranslation = localTranslation;
                    tr.localRotation = glm::inverse(parentTr.rotation) * glm::quat(rotation);
                    tr.localScale = scale / parentTr.scale;
                }
                else
                {
                    tr.localTranslation = translation;
                    tr.localRotation = glm::quat(rotation);
                    tr.localScale = scale;
                }
                tr.dirty = true;
            }
        }

        ImGui::End();

        // TOOLBAR: 
        ImGui::SetNextWindowPos(canvasPos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 12.0f, 5.0f });

        const ImVec2 buttonSize = { 20.0f, 20.0f };
        auto mode = m_Gizmo.GetMode();
        std::string gizmoModeStr = mode == ImGuizmo::MODE::LOCAL ? "LOCAL" : "WORLD";
        if (ImGui::Button(gizmoModeStr.c_str(), buttonSize))
        {
            m_Gizmo.SetMode(mode == ImGuizmo::MODE::LOCAL ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL);
        }

        auto sceneState = m_Editor->GetState().sceneState;
        const bool isScenePlaying = sceneState == ignite::State::ScenePlay;
        Ref<Texture> scenePlayStopTex = isScenePlaying ? m_Icons["stop"] : m_Icons["play"];
        ImTextureID scenePlayStopID = reinterpret_cast<ImTextureID>(scenePlayStopTex->GetHandle().Get());

        ImGui::SameLine();
        ImGui::Image(scenePlayStopID, buttonSize);
        if (ImGui::IsItemClicked())
        {
            if (isScenePlaying)
            {
                m_Editor->OnSceneStop();
#if _WIN32
                HWND hwnd = glfwGetWin32Window(Application::GetInstance()->GetDeviceManager()->GetWindow());
                COLORREF rgbRed = 0x00E86071;
                DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif
            }
            else
            {
                m_Editor->OnScenePlay();
#if _WIN32
                HWND hwnd = glfwGetWin32Window(Application::GetInstance()->GetDeviceManager()->GetWindow());
                COLORREF rgbRed = 0x000000AB;
                DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif
            }
        }

        const bool isSceneSimulate = sceneState == ignite::State::SceneSimulate;
        Ref<Texture> sceneSimulateTex = isSceneSimulate ? m_Icons["stop"] : m_Icons["simulate"];
        ImTextureID sceneSimulateID = reinterpret_cast<ImTextureID>(sceneSimulateTex->GetHandle().Get());

        ImGui::SameLine();
        ImGui::Image(sceneSimulateID, buttonSize);
        if (ImGui::IsItemClicked())
        {
            if (isSceneSimulate)
            {
                m_Editor->OnSceneStop();
#if _WIN32
                HWND hwnd = glfwGetWin32Window(Application::GetInstance()->GetDeviceManager()->GetWindow());
                COLORREF rgbRed = 0x00E86071;
                DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif
            }
            else
            {
                m_Editor->OnSceneSimulate();
#if _WIN32
                HWND hwnd = glfwGetWin32Window(Application::GetInstance()->GetDeviceManager()->GetWindow());
                COLORREF rgbRed = 0x000000AB;
                DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif
            }
        }

        ImGui::PopStyleVar(1);
        
        ImGui::End();
    }

    void ScenePanel::DebugRender()
    {
        /*ImGui::Begin("Debug Render");

        ImGui::End();*/
    }

    void ScenePanel::CameraSettingsUI()
    {
        ImGui::Text("Mouse Pos: %.2f, %.2f Entity: (%d)", m_ViewportData.mousePos.x, m_ViewportData.mousePos.y, static_cast<entt::entity>(GetSelectedEntity()));

        // =================================
        // Camera settings
        static const char *cameraModeStr[2] = { "Orthographic", "Perspective" };
        const char *currentCameraModeStr = cameraModeStr[static_cast<i32>(m_Camera.projectionType)];
        if (ImGui::BeginCombo("Mode", currentCameraModeStr))
        {
            for (size_t i = 0; i < std::size(cameraModeStr); ++i)
            {
                bool isSelected = strcmp(currentCameraModeStr, cameraModeStr[i]) == 0;
                if (ImGui::Selectable(cameraModeStr[i], isSelected))
                {
                    m_Camera.projectionType = static_cast<ICamera::Type>(i);

                    if (m_Camera.projectionType == ICamera::Type::Orthographic)
                    {
                        m_CameraData.lastPosition = m_Camera.position;

                        m_Camera.position = { 0.0f, 0.0f, 1.0f };
                        m_Camera.zoom = 20.0f;
                    }
                    else
                    {
                        m_Camera.position = m_CameraData.lastPosition;
                    }

                    m_Camera.UpdateProjectionMatrix();
                    m_Camera.UpdateViewMatrix();
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::DragFloat3("Position", &m_Camera.position[0], 0.025f);

        if (m_Camera.projectionType == ICamera::Type::Perspective)
        {
            glm::vec2 yawPitch = { m_Camera.yaw, m_Camera.pitch };
            if (ImGui::DragFloat2("Yaw/Pitch", &yawPitch.x, 0.025f))
            {
                m_Camera.yaw = yawPitch.x;
                m_Camera.pitch = yawPitch.y;
            }
        }
        else if (m_Camera.projectionType == ICamera::Type::Orthographic)
        {
            ImGui::DragFloat("Zoom", &m_Camera.zoom, 0.025f);
        }
    }

    template<typename T, typename UIFunction>
    void ScenePanel::RenderComponent(const std::string &name, Entity entity, UIFunction uiFunction, bool allowedToRemove)
    {

        constexpr ImGuiTreeNodeFlags treeNdeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (entity.HasComponent<T>())
        {
            T &comp = entity.GetComponent<T>();
            UUID compID = comp.GetCompID();

            ImGui::PushID(static_cast<int>(compID));

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2 { 0.5f, 6.0f });
            ImGui::Separator();

            const bool open = ImGui::TreeNodeEx((const char *)(uint32_t *)(uint64_t *)&compID, treeNdeFlags, name.c_str());
            ImGui::PopStyleVar();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 24.0f);
            if (ImGui::Button("+", {24.0f, 24.0f}))
                ImGui::OpenPopup("comp_settings");

            bool componentRemoved = false;

            if (ImGui::BeginPopup("comp_settings"))
            {
                if (allowedToRemove && ImGui::MenuItem("Remove")) 
                    componentRemoved = true;
                ImGui::EndPopup();
            }

            if (open)
            {
                uiFunction();
                ImGui::TreePop();
            }

            if (componentRemoved)
                entity.RemoveComponent<T>();

            ImGui::PopID();
        }
    }

    void ScenePanel::OnEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_CLASS_EVENT_FN(ScenePanel::OnMouseScrolledEvent));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_CLASS_EVENT_FN(ScenePanel::OnMouseMovedEvent));
        dispatcher.Dispatch<JoystickConnectionEvent>(BIND_CLASS_EVENT_FN(ScenePanel::OnJoystickConnectionEvent));
    }

    bool ScenePanel::OnMouseScrolledEvent(MouseScrolledEvent &event)
    {
        if (m_IsHovered)
        {
            const f32 dx = event.GetXOffset(), dy = event.GetYOffset();

            switch (m_Camera.projectionType)
            {
                case ICamera::Type::Perspective:
                {
                    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                    {
                        m_CameraData.moveSpeed += dy;
                        m_CameraData.moveSpeed = glm::clamp(m_CameraData.moveSpeed, 0.5f, m_CameraData.maxMoveSpeed);
                    }
                    else
                    {
                        m_Camera.position += m_Camera.GetForwardDirection() * dy * m_CameraData.moveSpeed * 0.5f;
                    }
                    break;
                }
                case ICamera::Type::Orthographic:
                default:
                {
                    const f32 scaleFactor = std::max(m_ViewportData.rect.GetSize().x, m_ViewportData.rect.GetSize().y);
                    const f32 zoomSqrt = glm::sqrt(m_Camera.zoom * m_Camera.zoom);
                    const f32 moveSpeed = 50.0f / scaleFactor;
                    const f32 mulFactor = moveSpeed * m_Camera.GetAspectRatio() * zoomSqrt;
                    if (Input::IsKeyPressed(KEY_LEFT_SHIFT))
                    {
                        if (Input::IsKeyPressed(KEY_LEFT_CONTROL))
                        {
                            m_Camera.position -= m_Camera.GetRightDirection() * dy * mulFactor;
                            m_Camera.position += m_Camera.GetUpDirection() * dx * mulFactor;
                        }
                        else
                        {
                            m_Camera.position -= m_Camera.GetRightDirection() * dx * mulFactor;
                            m_Camera.position += m_Camera.GetUpDirection() * dy * mulFactor;
                        }
                    }
                    else
                    {
                        m_Camera.zoom -= dy * zoomSqrt * 0.1f;
                        m_Camera.zoom = glm::clamp(m_Camera.zoom, 1.0f, 100.0f);
                    }
                    break;
                }
            }

            if (m_IsFocused)
            {
            }

            m_Camera.UpdateViewMatrix();
        }

        return false;
    }

    bool ScenePanel::OnMouseMovedEvent(MouseMovedEvent &event)
    {
        return false;
    }

    bool ScenePanel::OnJoystickConnectionEvent(JoystickConnectionEvent &event)
    {
        LOG_INFO(event.ToString());

        return false;
    }

    void ScenePanel::SetGizmoOperation(ImGuizmo::OPERATION op)
    {
        m_Gizmo.SetOperation(op);
    }

    void ScenePanel::SetGizmoMode(ImGuizmo::MODE mode)
    {
        m_Gizmo.SetMode(mode);
    }

    void ScenePanel::UpdateCameraInput(f32 deltaTime)
    {
        for (const Ref<Joystick> &j : JoystickManager::GetConnectedJoystick())
        {
            const glm::vec2 &camViewAxis = j->GetRightAxis();
            const glm::vec2 &camMoveAxis = j->GetLeftAxis();
            const glm::vec2 &l2r2 = j->GetTriggerAxis();

            m_Camera.yaw += deltaTime * camViewAxis.x;
            m_Camera.pitch += deltaTime * camViewAxis.y;

            m_Camera.position += m_Camera.GetForwardDirection() * deltaTime * m_CameraData.moveSpeed * -camMoveAxis.y;
            m_Camera.position += m_Camera.GetRightDirection() * deltaTime * m_CameraData.moveSpeed * camMoveAxis.x;

            // m_CameraData.moveSpeed += l2r2.y * deltaTime * 0.1f + 1.0f;
            // m_CameraData.moveSpeed -= l2r2.x * deltaTime * 0.1f + 1.0f;

            LOG_INFO(j->ToString());
        }

        if (!m_IsHovered)
            return;

        
        // Static to preserve state between frames
        static glm::vec2 lastMousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        glm::vec2 currentMousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        glm::vec2 mouseDelta = { 0.0f, 0.0f };

        // Only update mouseDelta when a mouse button is pressed
        bool mbPressed = Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || Input::IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);

        if (mbPressed)
            mouseDelta = currentMousePos - lastMousePos;
        else
            mouseDelta = { 0.0f, 0.0f };

        lastMousePos = currentMousePos;

        const f32 x = std::min(m_ViewportData.rect.GetSize().x * 0.01f, 1.8f);
        const f32 y = std::min(m_ViewportData.rect.GetSize().y * 0.01f, 1.8f);
        const f32 xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
        const f32 yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
        const f32 yawSign = m_Camera.GetUpDirection().y < 0 ? -1.0f : 1.0f;

        const bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        const bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

        // skip keyboard control
        if (control || shift || mbPressed == false)
            return;

        if (m_Camera.projectionType == ICamera::Type::Orthographic)
        {
            if (Input::IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            {
                m_Camera.position.x += -mouseDelta.x * xFactor * m_Camera.zoom * 0.005f;
                m_Camera.position.y += mouseDelta.y * yFactor * m_Camera.zoom * 0.005f;
            }
        }

        if (m_Camera.projectionType == ICamera::Type::Perspective)
        {
            // mouse input
            if (Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            {
                m_Camera.yaw += yawSign * m_CameraData.rotationSpeed * mouseDelta.x * xFactor * 0.03f;
                m_Camera.pitch += m_CameraData.rotationSpeed * mouseDelta.y * yFactor * 0.03f;
                m_Camera.pitch = glm::clamp(m_Camera.pitch, -89.0f, 89.0f);
            }
            else if (Input::IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            {
                m_Camera.position += m_Camera.GetRightDirection() * mouseDelta.x * xFactor * m_CameraData.moveSpeed * 0.03f;
                m_Camera.position += m_Camera.GetUpDirection() * -mouseDelta.y * yFactor * m_CameraData.moveSpeed * 0.03f;
            }

            // key input
            if (Input::IsKeyPressed(KEY_W))
            {
                m_Camera.position += m_Camera.GetForwardDirection() * deltaTime * m_CameraData.moveSpeed;
            }
            else if (Input::IsKeyPressed(KEY_S))
            {
                m_Camera.position -= m_Camera.GetForwardDirection() * deltaTime * m_CameraData.moveSpeed;
            }
            
            if (Input::IsKeyPressed(KEY_A))
            {
                m_Camera.position -= m_Camera.GetRightDirection() * deltaTime * m_CameraData.moveSpeed;
            }
            else if (Input::IsKeyPressed(KEY_D))
            {
                m_Camera.position += m_Camera.GetRightDirection() * deltaTime * m_CameraData.moveSpeed;
            }
        }

        m_Camera.UpdateViewMatrix();
    }

    void ScenePanel::DestroyEntity(Entity entity)
    {
        SceneManager::DestroyEntity(m_Scene, entity);
    }

    void ScenePanel::ClearSelection()
    {
        m_SelectedEntities.clear();
    }

    Entity ScenePanel::SetSelectedEntity(Entity entity)
    {
        if (!entity.IsValid())
        {
            m_SelectedEntities.clear();
            m_TrackingSelectedEntity = UUID(0);
            return {};
        }

        // multi select
        if (m_Editor->GetState().multiSelect)
        {
            auto it = m_SelectedEntities.find(entity.GetUUID());
            if (it != m_SelectedEntities.end())
            {
                // deselect
                it = m_SelectedEntities.erase(it);
                
                if (!m_SelectedEntities.empty())
                {
                    m_TrackingSelectedEntity = m_SelectedEntities.begin()->first;
                    return m_SelectedEntities.begin()->second;
                }
            }
            else
            {
                m_SelectedEntities[entity.GetUUID()] = entity;
            }
        }
        else // single select
        {
            // clear first
            if (!m_SelectedEntities.empty())
                m_SelectedEntities.clear();

            m_SelectedEntities[entity.GetUUID()] = entity;
        }

        if (m_SelectedEntities.empty())
        {
            m_Gizmo.SetOperation(ImGuizmo::OPERATION::NONE);
        }

        m_TrackingSelectedEntity = entity.GetUUID();
        return entity;
    }

    Entity ScenePanel::GetSelectedEntity()
    {
        return m_SelectedEntities.empty() ? Entity{} : m_SelectedEntities.begin()->second;
    }

    void ScenePanel::DuplicateSelectedEntity()
    {
        for (Entity entity : m_SelectedEntities | std::views::values)
        {
            if (entity.IsValid())
            {
                SceneManager::DuplicateEntity(m_Scene, entity);
            }
        }
    }
}
