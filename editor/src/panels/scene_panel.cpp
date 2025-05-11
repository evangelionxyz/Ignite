#include "scene_panel.hpp"

#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_event.hpp"
#include "ignite/core/input/mouse_event.hpp"
#include "ignite/graphics/texture.hpp"
#include "ignite/scene/icomponent.hpp"
#include "ignite/core/platform_utils.hpp"
#include "editor_layer.hpp"
#include "entt/entt.hpp"

#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>

namespace ignite
{
    static std::unordered_map<std::string, CompType> componentsName = 
    {
        { "Rigidbody2D", CompType_Rigidbody2D },
        { "Sprite2D", CompType_Sprite2D},
        { "BoxCollider2D", CompType_BoxCollider2D }
    };

    static std::string ToLower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    UUID ScenePanel::m_TrackingSelectedEntity = UUID(0);

    ScenePanel::ScenePanel(const char *windowTitle, EditorLayer *editor)
        : IPanel(windowTitle), m_Editor(editor)
    {
        Application *app = Application::GetInstance();

        m_ViewportCamera = CreateScope<Camera>("ScenePanel-Editor Camera");
        // m_ViewportCamera->CreateOrthographic(app->GetCreateInfo().width, app->GetCreateInfo().height, 8.0f, 0.1f, 350.0f);
        m_ViewportCamera->CreatePerspective(45.0f, app->GetCreateInfo().width, app->GetCreateInfo().height, 0.1f, 350.0f);
        m_ViewportCamera->position = {3.0f, 2.0f, 3.0f};
        m_ViewportCamera->yaw = -0.729f;
        m_ViewportCamera->pitch = 0.410f;
    }

    void ScenePanel::SetActiveScene(Scene *scene, bool reset)
    {
        if (reset)
        {
            m_SelectedEntityIDs.clear();
        }

        m_Scene = scene;
        m_SelectedEntity = SceneManager::GetEntity(m_Scene, m_TrackingSelectedEntity);
    }

    void ScenePanel::CreateRenderTarget(nvrhi::IDevice *device)
    {
        RenderTargetCreateInfo createInfo = {};
        createInfo.device = device;
        createInfo.depthWrite = true;
        createInfo.attachments = {
            FramebufferAttachments{ nvrhi::Format::D32S8},
            FramebufferAttachments{ nvrhi::Format::SRGBA8_UNORM },
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

        RenderHierarchy();
        RenderInspector();
        RenderViewport();
    }

    void ScenePanel::OnUpdate(f32 deltaTime)
    {
        UpdateCameraInput(deltaTime);
        m_ViewportCamera->UpdateProjectionMatrix();
        m_ViewportCamera->UpdateViewMatrix();
    }

    void ScenePanel::RenderHierarchy()
    {
        ImGui::Begin("Hierarchy");

        static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
        ImGui::BeginChild("scene_hierarchy", { ImGui::GetContentRegionAvail().x, 20.0f }, 0, windowFlags);
        ImGui::Button(m_Scene->name.c_str(), ImGui::GetContentRegionAvail());
        // target drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
            {
                LOG_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ITEM, that should be an entity");
                Entity src { *static_cast<entt::entity *>(payload->Data), m_Scene };
                ID &idComp = src.GetComponent<ID>();

                // check if src entity has parent
                if (idComp.parent != 0)
                {
                    // current parent should be remove it
                    Entity parent = SceneManager::GetEntity(m_Scene, idComp.parent);
                    parent.GetComponent<ID>().RemoveChild(idComp.uuid);

                    // reset the parent to 0
                    idComp.parent = UUID(0);
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::EndChild();

        ImGui::BeginChild("entity_hierachy", ImGui::GetContentRegionAvail(), 0, windowFlags);
        ImGui::Text("Entity count: %zu", m_Scene->entities.size());

        static ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
        if (ImGui::BeginTable("entity_hierarchy_table", 3, tableFlags))
        {
            // setup table 3 columns
            // Name, Type, Active (check box)
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableHeadersRow();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2.0f, 0.0f });
            m_Scene->registry->view<ID>().each([&](entt::entity e, ID &id)
            {
                RenderEntityNode(Entity{ e, m_Scene }, id.uuid, 0);
            });
            ImGui::PopStyleVar();

            // show right click entity create context
            if (ImGui::BeginPopupContextWindow("create_entity_context", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                ShowEntityContextMenu();
                ImGui::EndPopup();
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();

        ImGui::End();
    }

    Entity ScenePanel::ShowEntityContextMenu()
    {
        Entity entity = {};
        if (ImGui::MenuItem("Empty"))
        {
            entity = SetSelectedEntity(SceneManager::CreateEntity(m_Scene, "Empty", EntityType_Common));
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

        }

        return entity;
    }

    void ScenePanel::RenderEntityNode(Entity entity, UUID uuid, i32 index)
    {
        ID &idComp = entity.GetComponent<ID>();

        if (!entity.IsValid() || (idComp.parent && index == 0))
            return;

        ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | (!idComp.HasChild() ? ImGuiTreeNodeFlags_Leaf : 0)
            | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        void *imguiPushId = (void*)(uint64_t)(uint32_t)entity;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        bool opened = ImGui::TreeNodeEx(imguiPushId, flags, idComp.name.c_str());

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
                    RenderEntityNode(childEntity, uuid, index + 1);
                }
            }

            ImGui::TreePop();
        }
    }

    void ScenePanel::RenderInspector()
    {
        ImGui::Begin("Inspector");


        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (m_SelectedEntity.IsValid())
        {
            // Main Component
            
            // ID Component
            ID &idComp = m_SelectedEntity.GetComponent<ID>();
            char buffer[255] = {};
            strncpy(buffer, idComp.name.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("##label", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                SceneManager::RenameEntity(m_Scene, m_SelectedEntity, std::string(buffer));
            }

            ImGui::SameLine();
            if (ImGui::Button("Add"))
            {
                ImGui::OpenPopupOnItemClick("add_component_context");
            }

            // transform component
            RenderComponent<Transform>("Transform", m_SelectedEntity, [this]()
            {
                Transform &comp = m_SelectedEntity.GetComponent<Transform>();
                ImGui::DragFloat3("Translation", &comp.localTranslation.x, 0.025f);

                glm::vec3 euler = eulerAngles(comp.localRotation);
                if (ImGui::DragFloat3("Rotation", &euler.x, 0.025f))
                {
                    comp.localRotation = glm::quat(euler);
                }
                ImGui::DragFloat3("Scale", &comp.localScale.x, 0.025f);
            }, false); // false: not allowed to remove the component

            auto &comps = m_Scene->registeredComps[m_SelectedEntity];
            for (IComponent *comp : comps)
            {
                switch (comp->GetType())
                {
                case CompType_Sprite2D:
                {
                    RenderComponent<Sprite2D>("Sprite 2D", m_SelectedEntity, [entity = m_SelectedEntity, comp]()
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
                                texCI.device = Application::GetInstance()->GetDeviceManager()->GetDevice();
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

                        ImGui::ColorEdit4("Color", &c->color.x);
                        ImGui::DragFloat2("Tiling", &c->tilingFactor.x, 0.025f);
                    });

                    break;
                }
                case CompType_Rigidbody2D:
                {
                    RenderComponent<Rigidbody2D>("Rigid Body 2D", m_SelectedEntity, [entity = m_SelectedEntity, comp, scene = m_Scene]()
                    {
                        Rigidbody2D *c = comp->As<Rigidbody2D>();
                        
                        static const char *bodyTypeStr[3] = { "Static", "Dynamic", "Kinematic" };
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
    
                        if (scene->IsPlaying())
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
                case CompType_BoxCollider2D:
                {
                    RenderComponent<BoxCollider2D>("Box Collider 2D", m_SelectedEntity, [entity = m_SelectedEntity, comp, scene = m_Scene]()
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
                }
            }

            if (ImGui::Button("Add Component", { ImGui::GetContentRegionAvail().x, 20.0f }))
            {
                ImGui::OpenPopupOnItemClick("add_component_context");
            }

            // add component context
            if (ImGui::BeginPopupContextItem("add_component_context", ImGuiPopupFlags_NoOpenOverExistingPopup))
            {
                static char buffer[256] = { 0 };
                static std::string compNameResult;
                static std::set<std::pair<std::string, CompType>> filteredCompName;

                ImGui::InputTextWithHint("##component_name", "Component", buffer, sizeof(buffer) + 1, ImGuiInputTextFlags_EscapeClearsAll);
                
                compNameResult = std::string(buffer);
                filteredCompName.clear();

                if (!compNameResult.empty())
                {
                    std::string search = ToLower(compNameResult);
                    for (const auto& [strName, type] : componentsName)
                    {
                        bool isExists = false;
                        for (IComponent *comp : comps)
                        {
                            if (comp->GetType() == type)
                            {
                                isExists = true;
                                break;
                            }
                        }
                        if (isExists)
                            continue;
                        std::string nameLower = ToLower(strName);
                        if (nameLower.find(search) != std::string::npos)
                        {
                            filteredCompName.insert({strName, type});
                        }
                    }
                }

                static std::function<void(Entity, CompType)> addCompFunc = [=](Entity entity, CompType type)
                {
                    switch (type)
                    {
                        case CompType_Sprite2D:
                        {
                            entity.AddComponent<Sprite2D>();
                            break;
                        }
                        case CompType_Rigidbody2D:
                        {
                            entity.AddComponent<Rigidbody2D>();
                            break;
                        }
                        case CompType_BoxCollider2D:
                        {
                            entity.AddComponent<BoxCollider2D>();
                            break;
                        }
                    }
                };
                

                if (compNameResult.empty())
                {
                    for (const auto& [strName, type] : componentsName)
                    {
                        bool isExists = false;
                        for (IComponent *comp : comps)
                        {
                            if (comp->GetType() == type)
                            {
                                isExists = true;
                                break;
                            }
                        }
                        if (isExists)
                            continue;

                        if (ImGui::Selectable(strName.c_str()))
                        {
                            addCompFunc(Entity {m_SelectedEntity, m_Scene}, type);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                for (const auto &[strName, type] : filteredCompName)
                {
                    if (ImGui::Selectable(strName.c_str()))
                    {
                        addCompFunc(Entity {m_SelectedEntity, m_Scene}, type);
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
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("Viewport", nullptr, windowFlags);

        const ImGuiWindow *window = ImGui::GetCurrentWindow();
        m_ViewportData.isFocused = ImGui::IsWindowFocused();
        m_ViewportData.isHovered = ImGui::IsWindowHovered();
        m_ViewportData.width = window->Size.x;
        m_ViewportData.height = window->Size.y;

        uint32_t vpWidth = static_cast<uint32_t>(m_ViewportData.width);
        uint32_t vpHeght = static_cast<uint32_t>(m_ViewportData.height);

        // trigger resize
        if (vpWidth > 0 && vpHeght > 0 && (vpWidth != m_RenderTarget->GetWidth() || vpHeght != m_RenderTarget->GetHeight()))
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                m_RenderTarget->Resize(vpWidth, vpHeght);
        }

        m_ViewportCamera->SetSize(window->Size.x, window->Size.y);

        const ImTextureID imguiTex = reinterpret_cast<ImTextureID>(m_RenderTarget->GetColorAttachment(0).Get());
        ImGui::Image(imguiTex, window->Size);

        static glm::mat4 gridMatrix(1.0f);

        GizmoInfo gizmoInfo;
        gizmoInfo.cameraView = m_ViewportCamera->viewMatrix;
        gizmoInfo.cameraProjection = m_ViewportCamera->projectionMatrix;
        gizmoInfo.cameraType = m_ViewportCamera->projectionType;
        gizmoInfo.viewRect = { window->Pos.x, window->Pos.y, window->Size.x, window->Size.y };

        m_Gizmo.SetInfo(gizmoInfo);

        for (auto &uuid : m_SelectedEntityIDs)
        {
            Entity entity = SceneManager::GetEntity(m_Scene, uuid);
            ImGuizmo::PushID((u64)uuid);
            
            Transform &tr = entity.GetComponent<Transform>();

            glm::mat4 transformMatrix = tr.WorldTransform();

            m_Gizmo.Manipulate(transformMatrix);

            if (m_Gizmo.IsManipulating())
            {
                glm::vec3 scale, translation, skew;
                glm::vec4 perspective;
                glm::quat orientation;
                glm::decompose(transformMatrix, scale, orientation, translation, skew, perspective);

                tr.localTranslation = translation;
                tr.localScale = scale;
                tr.localRotation = orientation;
            }

            ImGuizmo::PopID();

        }

        // m_Gizmo.DrawGrid(1000.0f);

        ImGui::End();
    }

    void ScenePanel::CameraSettingsUI()
    {
        // =================================
        // Camera settings

        static const char *cameraModeStr[2] = { "Orthographic", "Perspective" };
        const char *currentCameraModeStr = cameraModeStr[static_cast<i32>(m_ViewportCamera->projectionType)];
        if (ImGui::BeginCombo("Mode", currentCameraModeStr))
        {
            for (size_t i = 0; i < std::size(cameraModeStr); ++i)
            {
                bool isSelected = strcmp(currentCameraModeStr, cameraModeStr[i]) == 0;
                if (ImGui::Selectable(cameraModeStr[i], isSelected))
                {
                    currentCameraModeStr = cameraModeStr[i];
                    m_ViewportCamera->projectionType = static_cast<Camera::Type>(i);

                    if (m_ViewportCamera->projectionType == Camera::Type::Orthographic)
                    {
                        m_CameraData.lastPosition = m_ViewportCamera->position;

                        m_ViewportCamera->position = { 0.0f, 0.0f, 1.0f };
                        m_ViewportCamera->zoom = 20.0f;
                    }
                    else
                    {
                        m_ViewportCamera->position = m_CameraData.lastPosition;
                    }
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::DragFloat3("Position", &m_ViewportCamera->position[0], 0.025f);

        if (m_ViewportCamera->projectionType == Camera::Type::Perspective)
        {
            glm::vec2 yawPitch = { m_ViewportCamera->yaw, m_ViewportCamera->pitch };
            if (ImGui::DragFloat2("Yaw/Pitch", &yawPitch.x, 0.025f))
            {
                m_ViewportCamera->yaw = yawPitch.x;
                m_ViewportCamera->pitch = yawPitch.y;
            }
        }
        else if (m_ViewportCamera->projectionType == Camera::Type::Orthographic)
        {
            ImGui::DragFloat("Zoom", &m_ViewportCamera->zoom, 0.025f);
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

            ImGui::PushID(compID);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2 { 0.5f, 2.0f });
            ImGui::Separator();

            const bool open = ImGui::TreeNodeEx((const char *)(uint32_t *)(uint64_t *)&compID, treeNdeFlags, name.c_str());
            ImGui::PopStyleVar();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 24.0f);
            if (ImGui::Button("+", { 14.0f, 14.0f }))
                ImGui::OpenPopup("comp_settings");

            bool componentRemoved = false;
            if (allowedToRemove)
            {

            }

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
                    if (Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                    {
                        m_CameraData.moveSpeed += dy;
                        m_CameraData.moveSpeed = glm::clamp(m_CameraData.moveSpeed, 0.5f, m_CameraData.maxMoveSpeed);
                    }
                    else
                    {
                        m_ViewportCamera->position += m_ViewportCamera->GetForwardDirection() * dy * m_CameraData.moveSpeed * 0.5f;
                    }
                    break;
                }
                case Camera::Type::Orthographic:
                default:
                {
                    const f32 scaleFactor = std::max(m_ViewportData.width, m_ViewportData.height);
                    const f32 zoomSqrt = glm::sqrt(m_ViewportCamera->zoom * m_ViewportCamera->zoom);
                    const f32 moveSpeed = 50.0f / scaleFactor;
                    const f32 mulFactor = moveSpeed * m_ViewportCamera->GetAspectRatio() * zoomSqrt;
                    if (Input::IsKeyPressed(KEY_LEFT_SHIFT))
                    {
                        if (Input::IsKeyPressed(KEY_LEFT_CONTROL))
                        {
                            m_ViewportCamera->position -= m_ViewportCamera->GetRightDirection() * dy * mulFactor;
                            m_ViewportCamera->position += m_ViewportCamera->GetUpDirection() * dx * mulFactor;
                        }
                        else
                        {
                            m_ViewportCamera->position -= m_ViewportCamera->GetRightDirection() * dx * mulFactor;
                            m_ViewportCamera->position += m_ViewportCamera->GetUpDirection() * dy * mulFactor;
                        }
                    }
                    else
                    {
                        m_ViewportCamera->zoom -= dy * zoomSqrt * 0.1f;
                        m_ViewportCamera->zoom = glm::clamp(m_ViewportCamera->zoom, 1.0f, 100.0f);
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
        if (!m_ViewportData.isHovered)
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

        const f32 x = std::min(m_ViewportData.width * 0.01f, 1.8f);
        const f32 y = std::min(m_ViewportData.height * 0.01f, 1.8f);
        const f32 xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
        const f32 yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
        const f32 yawSign = m_ViewportCamera->GetUpDirection().y < 0 ? -1.0f : 1.0f;

        const bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        const bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

        // skip keyboard control
        if (control || shift || mbPressed == false)
            return;

        if (m_ViewportCamera->projectionType == Camera::Type::Orthographic)
        {
            if (Input::IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            {
                m_ViewportCamera->position.x += -mouseDelta.x * xFactor * m_ViewportCamera->zoom * 0.005f;
                m_ViewportCamera->position.y += mouseDelta.y * yFactor * m_ViewportCamera->zoom * 0.005f;
            }
        }

        if (m_ViewportCamera->projectionType == Camera::Type::Perspective)
        {
            // mouse input
            if (Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            {
                m_ViewportCamera->yaw += yawSign * m_CameraData.rotationSpeed * mouseDelta.x * xFactor * 0.03f;
                m_ViewportCamera->pitch += m_CameraData.rotationSpeed * mouseDelta.y * yFactor * 0.03f;
                m_ViewportCamera->pitch = glm::clamp(m_ViewportCamera->pitch, -89.0f, 89.0f);
            }
            else if (Input::IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
            {
                m_ViewportCamera->position += m_ViewportCamera->GetRightDirection() * mouseDelta.x * xFactor * m_CameraData.moveSpeed * 0.03f;
                m_ViewportCamera->position += m_ViewportCamera->GetUpDirection() * -mouseDelta.y * yFactor * m_CameraData.moveSpeed * 0.03f;
            }

            // key input
            if (Input::IsKeyPressed(KEY_W))
            {
                m_ViewportCamera->position += m_ViewportCamera->GetForwardDirection() * deltaTime * m_CameraData.moveSpeed;
            }
            else if (Input::IsKeyPressed(KEY_S))
            {
                m_ViewportCamera->position -= m_ViewportCamera->GetForwardDirection() * deltaTime * m_CameraData.moveSpeed;
            }
            
            if (Input::IsKeyPressed(KEY_A))
            {
                m_ViewportCamera->position -= m_ViewportCamera->GetRightDirection() * deltaTime * m_CameraData.moveSpeed;
            }
            else if (Input::IsKeyPressed(KEY_D))
            {
                m_ViewportCamera->position += m_ViewportCamera->GetRightDirection() * deltaTime * m_CameraData.moveSpeed;
            }
        }
    }

    void ScenePanel::DestroyEntity(Entity entity)
    {
        SceneManager::DestroyEntity(m_Scene, entity);

        m_SelectedEntity = {entt::null, nullptr};
    }

    void ScenePanel::ClearMultiSelectEntity()
    {
        m_SelectedEntityIDs.clear();
    }

    Entity ScenePanel::SetSelectedEntity(Entity entity)
    {
        if (!entity.IsValid())
        {
            m_SelectedEntityIDs.clear();
            m_TrackingSelectedEntity = UUID(0);
            m_SelectedEntity = {};
            return {};
        }

        m_TrackingSelectedEntity = entity.GetUUID();

        if (m_Editor->GetState().multiSelect)
        {
            m_SelectedEntityIDs.push_back(m_TrackingSelectedEntity);
        }
        else
        {
            if (m_SelectedEntityIDs.size() > 1)
                m_SelectedEntityIDs.clear();

            if (m_SelectedEntityIDs.empty())
                m_SelectedEntityIDs.push_back(m_TrackingSelectedEntity);

            m_SelectedEntityIDs[0] = m_TrackingSelectedEntity;
        }

        return m_SelectedEntity = entity;
    }

    Entity ScenePanel::GetSelectedEntity()
    {
        return m_SelectedEntity;
    }

    void ScenePanel::DuplicateSelectedEntity()
    {
        if (m_SelectedEntity.IsValid())
        {
            SceneManager::DuplicateEntity(m_Scene, m_SelectedEntity);
        }
    }
}