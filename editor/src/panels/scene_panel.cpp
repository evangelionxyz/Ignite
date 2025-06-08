#include "scene_panel.hpp"

#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"
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
#include "../states.hpp"
#include "entt/entt.hpp"

#ifdef _WIN32
#   include <dwmapi.h>
#   include <ShellScalingApi.h>
#endif

#include <set>
#include <unordered_map>
#include <string>
#include <algorithm>

namespace ignite
{

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

        m_ViewportCamera->UpdateViewMatrix();
        m_ViewportCamera->UpdateProjectionMatrix();


        // Load icons
        TextureCreateInfo createInfo;
        createInfo.format = nvrhi::Format::RGBA8_UNORM;
        m_Icons["simulate"] = Texture::Create("resources/ui/ic_simulate.png", createInfo);
        m_Icons["play"] = Texture::Create("resources/ui/ic_play.png", createInfo);
        m_Icons["stop"] = Texture::Create("resources/ui/ic_stop.png", createInfo);

        nvrhi::IDevice *device = app->GetRenderDevice();
        nvrhi::CommandListHandle commandList = device->createCommandList();
        commandList->open();
        for (auto &[name, icon] : m_Icons)
        {
            icon->Write(commandList);
        }
        commandList->close();
        device->executeCommandList(commandList);
    }

    void ScenePanel::SetActiveScene(Scene *scene, bool reset)
    {
        if (reset)
        {
            m_SelectedEntities.clear();
        }

        m_Scene = scene;
        m_SelectedEntity = SceneManager::GetEntity(m_Scene, m_TrackingSelectedEntity);
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
                    // current parent should be removed it
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

        }

        return entity;
    }

    void ScenePanel::RenderEntityNode(Entity entity, UUID uuid, i32 index)
    {
        if (!entity.IsValid())
            return;

        ID &idComp = entity.GetComponent<ID>();
        if (idComp.parent && index == 0)
            return;

        ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | (!idComp.HasChild() ? ImGuiTreeNodeFlags_Leaf : 0)
            | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow
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

            auto &comps = m_Scene->registeredComps[m_SelectedEntity];
            
            // ID Component
            ID &idComp = m_SelectedEntity.GetComponent<ID>();
            char buffer[255] = {};
            strncpy(buffer, idComp.name.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("##label", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                SceneManager::RenameEntity(m_Scene, m_SelectedEntity, std::string(buffer));
            }

            ImGui::SameLine();

            if (ImGui::Button("Add Component", { ImGui::GetContentRegionAvail().x, 25.0f }))
            {
                ImGui::OpenPopup("##add_component_context");
            }
    
            // transform component
            RenderComponent<Transform>("Transform", m_SelectedEntity, [this]()
            {
                Transform &comp = m_SelectedEntity.GetComponent<Transform>();
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
                    RenderComponent<Rigidbody2D>("Rigid Body 2D", m_SelectedEntity, [entity = m_SelectedEntity, comp, scene = m_Scene]()
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
                case CompType_MeshRenderer:
                {
                    RenderComponent<MeshRenderer>("Mesh Renderer", m_SelectedEntity, [entity = m_SelectedEntity, comp, scene = m_Scene]()
                    {
                        MeshRenderer *c = comp->As<MeshRenderer>();
                        ImGui::Text("Mesh: %s", c->name.c_str());
                        ImGui::ColorEdit4("Base Color", &c->material.data.baseColor.x);
                        ImGui::DragFloat("Metallic", &c->material.data.metallic, 0.025f);
                        ImGui::DragFloat("Roughness", &c->material.data.roughness, 0.025f);
                        ImGui::DragFloat("Emissive", &c->material.data.emissive, 0.025f);
                    });
                    break;
                }
                case CompType_SkinnedMesh:
                {
                    RenderComponent<SkinnedMesh>("Skinned Mesh", m_SelectedEntity, [entity = m_SelectedEntity, comp, scene = m_Scene, this]()
                    {
                        SkinnedMesh *c = comp->As<SkinnedMesh>();

                        if (ImGui::Button("Load", {50.0f, 30.0f}))
                        {
                            std::filesystem::path filepath = FileDialogs::OpenFile("GLB/GLTF (*.glb;*.gltf)\0*.glb;*.gltf\0FBX (*.fbx)\0*.fbx");
                            if (!filepath.empty())
                            {
                                LOG_ASSERT(std::filesystem::exists(filepath), "[Mesh Loader] File does not exists!");

                                Assimp::Importer importer;
                                const aiScene *assimpScene = importer.ReadFile(filepath.generic_string(), ASSIMP_IMPORTER_FLAGS);

                                LOG_ASSERT(assimpScene == nullptr || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || assimpScene->mRootNode,
                                    "[Model] Failed to load {}: {}",
                                    filepath,
                                    importer.GetErrorString());

                                if (!assimpScene)
                                {
                                    return Entity{};
                                }

                                c->skeleton = CreateRef<Skeleton>();

                                if (assimpScene->HasAnimations())
                                {
                                    MeshLoader::LoadAnimation(assimpScene, c->animations);

                                    // Process Skeleton
                                    MeshLoader::ExtractSkeleton(assimpScene, c->skeleton);
                                    MeshLoader::SortJointsHierarchically(c->skeleton);
                                }

                                std::vector<Ref<Mesh>> meshes;
                                meshes.resize(assimpScene->mNumMeshes);
                                for (auto &mesh : meshes)
                                {
                                    mesh = CreateRef<Mesh>();
                                }

                                std::vector<NodeInfo> nodes;

                                MeshLoader::ProcessNode(assimpScene, assimpScene->mRootNode, filepath.generic_string(), meshes, nodes, c->skeleton, -1);
                                MeshLoader::CalculateWorldTransforms(nodes, meshes);

                                // First pass: create all node entities
                                for (auto &node : nodes)
                                {
                                    if (node.uuid == UUID(0) || node.parentID != -1) // not yet created
                                    {
                                        Entity entity = SceneManager::CreateEntity(scene, node.name, EntityType_Node);
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
                                    Entity nodeEntity = SceneManager::GetEntity(scene, node.uuid);

                                    if (node.parentID == -1)
                                    {
                                        // Attach the node to root node
                                        ID &id = m_SelectedEntity.GetComponent<ID>();
                                        id.name = filepath.stem().string();

                                        SceneManager::AddChild(scene, m_SelectedEntity, nodeEntity);
                                    }
                                    else
                                    {
                                        // Attach to parent if not root
                                        const auto &parentNode = nodes[node.parentID];
                                        Entity parentEntity = SceneManager::GetEntity(scene, parentNode.uuid);
                                        SceneManager::AddChild(scene, parentEntity, nodeEntity);
                                    }

                                    // Attach mesh entities to this node
                                    for (i32 meshIdx : node.meshIndices)
                                    {
                                        const auto &mesh = meshes[meshIdx];

                                        MeshRenderer &meshRenderer = nodeEntity.AddComponent<MeshRenderer>();

                                        meshRenderer.name = mesh->name;
                                        meshRenderer.root = m_SelectedEntity.GetUUID();

                                        for (auto &vertex : mesh->vertices)
                                        {
                                            entt::entity e = static_cast<entt::entity>(nodeEntity);
                                            vertex.entityID = entt::to_integral(e);
                                        }

                                        meshRenderer.mesh = CreateRef<EntityMesh>();

                                        meshRenderer.mesh->vertices = mesh->vertices;
                                        meshRenderer.mesh->indices = mesh->indices;
                                        meshRenderer.mesh->indexBuffer = mesh->indexBuffer;
                                        meshRenderer.mesh->vertexBuffer = mesh->vertexBuffer;
                                        meshRenderer.mesh->aabb = mesh->aabb;

                                        // outline
                                        meshRenderer.mesh->outlineVertexBuffer = mesh->outlineVertexBuffer;
                                        meshRenderer.mesh->outlineVertices = mesh->outlineVertices;

                                        meshRenderer.material = mesh->material;

                                        SceneManager::WriteMeshBuffer(scene, meshRenderer);
                                    }

                                    // Extract skeleton joints into entity
                                    for (size_t i = 0; i < c->skeleton->joints.size(); ++i)
                                    {
                                        if (const std::string &name = c->skeleton->joints[i].name; scene->nameToUUID.contains(name))
                                        {
                                            UUID uuid = scene->nameToUUID[name];
                                            c->skeleton->jointEntityMap[static_cast<i32>(i)] = uuid;

                                            Entity entity = SceneManager::GetEntity(scene, uuid);
                                            entity.GetComponent<ID>().type = EntityType_Joint;
                                        }
                                    }
                                }

                                MeshLoader::ClearTextureCache();
                            }
                        }

                        if (!c->animations.empty())
                        {

                            ImGui::SeparatorText("Animations");
                            if (ImGui::Button("Save Animations"))
                            {
                                std::filesystem::path directory = FileDialogs::SelectFolder();
                                if (!directory.empty())
                                {
                                    for (auto &anim : c->animations)
                                    {
                                        std::filesystem::path filepath = directory / std::format("anim_{0}.anim", anim->name);
                                        AnimationSerializer sr(anim);
                                        sr.Serialize(filepath);
                                    }
                                }
                            }

                            if (ImGui::Button("Play"))
                            {
                                const Ref<SkeletalAnimation> &anim = c->animations[c->activeAnimIndex];
                                anim->isPlaying = true;
                            }

                            ImGui::SameLine();

                            if (ImGui::Button("Stop"))
                            {
                                const Ref<SkeletalAnimation> &anim = c->animations[c->activeAnimIndex];
                                anim->isPlaying = false;
                            }

                            if (ImGui::TreeNode("Animations"))
                            {
                                for (size_t animIdx = 0; animIdx < c->animations.size(); ++animIdx)
                                {
                                    const Ref<SkeletalAnimation> &anim = c->animations[animIdx];
                                    if (ImGui::TreeNodeEx(anim->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", anim->name.c_str()))
                                    {
                                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                        {
                                            c->activeAnimIndex = animIdx;
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
                static std::string compNameResult;
                static std::set<std::pair<std::string, CompType>> filteredCompName;

                ImGui::InputTextWithHint("##component_name", "Component", buffer, sizeof(buffer) + 1, ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_NoHorizontalScroll);

                compNameResult = std::string(buffer);

                filteredCompName.clear();

                if (!compNameResult.empty())
                {
                    std::string search = stringutils::ToLower(compNameResult);
                    for (const auto &[strName, type] : s_ComponentsName)
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

                        std::string nameLower = stringutils::ToLower(strName);
                        if (nameLower.find(search) != std::string::npos)
                        {
                            filteredCompName.insert({ strName, type });
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
                        case CompType_MeshRenderer:
                        {
                            entity.AddComponent<MeshRenderer>();
                            break;
                        }
                        case CompType_SkinnedMesh:
                        {
                            entity.AddComponent<SkinnedMesh>();
                            break;
                        }
                        }
                    };

                if (compNameResult.empty())
                {
                    for (const auto &[strName, type] : s_ComponentsName)
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
                        {
                            continue;
                        }

                        if (ImGui::Selectable(strName.c_str()))
                        {
                            addCompFunc(Entity{ m_SelectedEntity, m_Scene }, type);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                for (const auto &[strName, type] : filteredCompName)
                {
                    if (ImGui::Selectable(strName.c_str()))
                    {
                        addCompFunc(Entity{ m_SelectedEntity, m_Scene }, type);
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
                m_ViewportCamera->SetSize(vpWidth, vpHeight);
                m_ViewportCamera->UpdateProjectionMatrix();
            }
        }

        const ImTextureID imguiTex = reinterpret_cast<ImTextureID>(m_RenderTarget->GetColorAttachment(0).Get());
        ImGui::Image(imguiTex, window->Size);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::GetDragDropPayload())
            {
                if (payload->DataSize == sizeof(AssetHandle))
                {
                    AssetHandle *handle = static_cast<AssetHandle *>(payload->Data);
                    if (handle && *handle != AssetHandle(0))
                    {
                        AssetMetaData metadata = Project::GetInstance()->GetAssetManager().GetMetaData(*handle);
                        if (metadata.type == AssetType::Scene)
                        {
                            std::filesystem::path filepath = Project::GetInstance()->GetAssetFilepath(metadata.filepath);
                            m_Editor->OpenScene(filepath);
                        }
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        GizmoInfo gizmoInfo;
        gizmoInfo.cameraView = m_ViewportCamera->viewMatrix;
        gizmoInfo.cameraProjection = m_ViewportCamera->projectionMatrix;
        gizmoInfo.cameraType = m_ViewportCamera->projectionType;

        gizmoInfo.viewRect = m_ViewportData.rect;

        m_Gizmo.SetInfo(gizmoInfo);

        // Start manipulation: Fired only on the first frame of interaction
        bool isManipulatingNow = m_Gizmo.IsManipulating();

        if (isManipulatingNow && !m_Data.isGizmoManipulating)
        {
            m_InitialTransforms.clear();
            for (Entity entity : m_SelectedEntities)
            {
                // Store the original transform of each selected entity
                m_InitialTransforms[entity.GetUUID()] = entity.GetTransform();
            }
        }
        // Set the master flag for the current frame
        m_Data.isGizmoManipulating = isManipulatingNow;
        m_Data.isGizmoBeingUse = isManipulatingNow || m_Gizmo.IsHovered();

        if (m_SelectedEntities.size() > 1)
        {
            // Step 1: Compute shared pivot (center of all selected entities)
            glm::vec3 pivot(0.0f);
            for (Entity entity : m_SelectedEntities)
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
                
                for (Entity entity : m_SelectedEntities)
                {
                    // Get the live transform component to apply changes to it
                    Transform &tr = entity.GetTransform();
     
                    // Get the ORIGINAL transform we stored at the beginning of the manipulation
                    const Transform &initialTransform = m_InitialTransforms.at(entity.GetUUID());
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
        else if (Entity entity = m_SelectedEntity)
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

        // m_Data.isGizmoBeingUse = m_Gizmo.IsManipulating() || m_Gizmo.IsHovered();

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
        ImGui::Text("Mouse Pos: %.2f, %.2f Entity: (%d)", m_ViewportData.mousePos.x, m_ViewportData.mousePos.y, static_cast<entt::entity>(m_SelectedEntity));

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

                    m_ViewportCamera->UpdateProjectionMatrix();
                    m_ViewportCamera->UpdateViewMatrix();
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
                    const f32 scaleFactor = std::max(m_ViewportData.rect.GetSize().x, m_ViewportData.rect.GetSize().y);
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

            if (m_IsFocused)
            {
            }

            m_ViewportCamera->UpdateViewMatrix();
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

            m_ViewportCamera->yaw += deltaTime * camViewAxis.x;
            m_ViewportCamera->pitch += deltaTime * camViewAxis.y;

            m_ViewportCamera->position += m_ViewportCamera->GetForwardDirection() * deltaTime * m_CameraData.moveSpeed * -camMoveAxis.y;
            m_ViewportCamera->position += m_ViewportCamera->GetRightDirection() * deltaTime * m_CameraData.moveSpeed * camMoveAxis.x;

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

        m_ViewportCamera->UpdateViewMatrix();
    }

    void ScenePanel::DestroyEntity(Entity entity)
    {
        SceneManager::DestroyEntity(m_Scene, entity);

        m_SelectedEntity = {entt::null, nullptr};
    }

    void ScenePanel::ClearMultiSelectEntity()
    {
        m_SelectedEntities.clear();
    }

    Entity ScenePanel::SetSelectedEntity(Entity entity)
    {
        if (!entity.IsValid())
        {
            m_SelectedEntities.clear();
            m_TrackingSelectedEntity = UUID(0);
            m_SelectedEntity = {};
            return {};
        }

        m_TrackingSelectedEntity = entity.GetUUID();

        if (m_Editor->GetState().multiSelect)
        {
            m_SelectedEntities.push_back(entity);
        }
        else
        {
            if (m_SelectedEntities.size() > 1)
                m_SelectedEntities.clear();

            if (m_SelectedEntities.empty())
                m_SelectedEntities.push_back(entity);

            m_SelectedEntities[0] = entity;
        }

        return m_SelectedEntity = entity;
    }

    Entity ScenePanel::GetSelectedEntity()
    {
        return m_SelectedEntity;
    }

    void ScenePanel::DuplicateSelectedEntity()
    {
        for (Entity entity : m_SelectedEntities)
        {
            if (entity.IsValid())
            {
                SceneManager::DuplicateEntity(m_Scene, entity);
            }
        }
    }
}