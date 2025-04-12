#include "scene_panel.hpp"

#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_event.hpp"
#include "ignite/core/input/mouse_event.hpp"
#include "ignite/graphics/texture.hpp"
#include "editor_layer.hpp"
#include "entt/entt.hpp"

namespace ignite
{
    ScenePanel::ScenePanel(const char *windowTitle, EditorLayer *editor)
        : IPanel(windowTitle), m_Editor(editor)
    {
        Application *app = Application::GetInstance();

        m_ViewportCamera = CreateScope<Camera>("ScenePanel-Editor Camera");
        m_ViewportCamera->CreateOrthographic(app->GetCreateInfo().width, app->GetCreateInfo().height, 8.0f, 0.1f, 350.0f);
        //m_ViewportCamera->CreatePerspective(45.0f, app->GetCreateInfo().width, app->GetCreateInfo().height, 0.1f, 350.0f);
        //m_ViewportCamera->position.z = 8.0f;
    }

    void ScenePanel::CreateRenderTarget(nvrhi::IDevice *device, f32 width, f32 height)
    {
        m_RenderTarget = CreateRef<RenderTarget>(device, width, height);
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
        RenderSettings();
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
        ImGui::Text("Entity count: %zu", m_Editor->m_ActiveScene->entities.size());

        for (auto &[uuid, e] : m_Editor->m_ActiveScene->entities)
        {
            RenderEntityNode(e, uuid, 0);
        }

        ImGui::End();
    }

    void ScenePanel::RenderEntityNode(entt::entity entity, UUID uuid, i32 index)
    {
        ID &idComp = m_Editor->m_ActiveScene->GetComponent<ID>(entity);

        ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0)
            | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, idComp.name.c_str());

        bool isDeleting = false;

        if (!isDeleting)
        {
            if (ImGui::IsItemHovered(ImGuiMouseButton_Left) && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                m_SelectedEntity = entity;
        }

        if (opened)
        {

            if (!isDeleting)
            {
                // TODO: traverse child
            }

            ImGui::TreePop();
        }
    }

    void ScenePanel::RenderInspector()
    {
        ImGui::Begin("Inspector");


        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (m_SelectedEntity != (entt::entity)-1)
        {
            std::vector<IComponent *> comps = m_Editor->m_ActiveScene->registeredComps[m_SelectedEntity];

            ImGui::PushID((i32)m_SelectedEntity);
            for (IComponent *comp : comps)
            {
                switch (comp->GetType())
                {
                case CompType_ID:
                {
                    if (ImGui::TreeNodeEx("ID", flags))
                    {
                        ID *c = comp->As<ID>();
                        ImGui::Text(c->name.c_str());

                        ImGui::TreePop();
                    }
                    break;
                }
                case CompType_Sprite2D:
                {
                    if (ImGui::TreeNodeEx("Sprite2D", flags))
                    {
                        Sprite2D *c = comp->As<Sprite2D>();
                        ImGui::ColorEdit4("Color", &c->color.x);
                        ImGui::DragFloat2("Tiling", &c->tilingFactor.x, 0.025f);

                        ImGui::TreePop();
                    }
                    break;
                }
                case CompType_Transform:
                {
                    if (ImGui::TreeNodeEx("Transfrorm", flags))
                    {
                        Transform *c = comp->As<Transform>();
                        ImGui::DragFloat3("Translation", &c->translation.x, 0.025f);

                        glm::vec3 euler = eulerAngles(c->rotation);
                        if (ImGui::DragFloat3("Rotation", &euler.x, 0.025f))
                        {
                            c->rotation = glm::quat(euler);
                        }
                        ImGui::DragFloat3("Scale", &c->scale.x, 0.025f);

                        ImGui::TreePop();
                    }
                    break;
                }
                case CompType_Rigidbody2D:
                {
                    if (ImGui::TreeNodeEx("Rigidbody 2D", flags))
                    {
                        Rigidbody2D *c = comp->As<Rigidbody2D>();
                        
                        static const char *bodyTypeStr[3] = { "Static", "Dynamic", "Kinematic" };
                        const char *currentBodyType = bodyTypeStr[static_cast<i32>(c->GetType())];
    
                        if (ImGui::BeginCombo("Type", currentBodyType))
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
    
                        if (m_Editor->m_ActiveScene->IsPlaying())
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
                            ImGui::DragFloat2("Linear Vel", &c->linearVelocity.x, 0.025f);
                            ImGui::DragFloat("Angular Vel", &c->angularVelocity, 0.025f);
                            ImGui::DragFloat("Gravity", &c->gravityScale, 0.025f);
                            ImGui::DragFloat("Linear Damping", &c->linearDamping, 0.025f);
                            ImGui::DragFloat("Angular Damping", &c->angularDamping, 0.025f);
                            ImGui::Checkbox("Fixed Rotation", &c->fixedRotation);
                            ImGui::Checkbox("Awake", &c->isAwake);
                            ImGui::Checkbox("Enabled", &c->isEnabled);
                            ImGui::Checkbox("Sleep", &c->isEnableSleep);
                        }

                        ImGui::TreePop();
                    }

                    break;
                }
                case CompType_BoxCollider2D:
                {
                    if (ImGui::TreeNodeEx("Box Collider 2D", flags))
                    {
                        BoxCollider2D *c = comp->As<BoxCollider2D>();
                        ImGui::DragFloat2("Size", &c->size.x, 0.025f);
                        ImGui::DragFloat2("Offset", &c->offset.x, 0.025f);
                        ImGui::DragFloat("Restitution", &c->restitution, 0.025f);
                        ImGui::DragFloat("Friction", &c->friction, 0.025f);
                        ImGui::DragFloat("Density", &c->density, 0.025f);
                        ImGui::Checkbox("Is Sensor", &c->isSensor);
                        ImGui::TreePop();
                    }
                    break;
                }
                }
            }
            ImGui::PopID();
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

        m_RenderTarget->width = window->Size.x;
        m_RenderTarget->height = window->Size.y;

        m_ViewportCamera->SetSize(m_RenderTarget->width, m_RenderTarget->height);

        const ImTextureID imguiTex = reinterpret_cast<ImTextureID>(m_RenderTarget->texture.Get());
        ImGui::Image(imguiTex, window->Size);

        ImGui::End();
    }

    void ScenePanel::RenderSettings()
    {
        ImGui::Begin("Settings", &m_State.settingsWindow);

        // =================================
        // Camera settings
        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static const char *cameraModeStr[2] = { "Orthographic", "Perspective" };
            const char *currentCameraModeStr = cameraModeStr[static_cast<i32>(m_ViewportCamera->projectionType)];
            if (ImGui::BeginCombo("Mode", currentCameraModeStr))
            {
                for (i32 i = 0; i < std::size(cameraModeStr); ++i)
                {
                    bool isSelected = (strcmp(currentCameraModeStr, cameraModeStr[i]) == 0);
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
            
            ImGui::Separator();
            ImGui::ColorEdit3("Clear color", &m_RenderTarget->clearColor[0]);

            ImGui::TreePop();
        }

        ImGui::End();
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
                        m_CameraData.moveSpeed += dy * 1.0f;
                        m_CameraData.moveSpeed = glm::clamp(m_CameraData.moveSpeed, 0.1f, m_CameraData.maxMoveSpeed);
                    }
                    else
                    {
                        m_ViewportCamera->position += m_ViewportCamera->GetForwardDirection() * dy * m_CameraData.moveSpeed;
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

    void ScenePanel::UpdateCameraInput(f32 deltaTime)
    {
        if (!m_ViewportData.isHovered)
            return;

        
        // Static to preserve state between frames
        static glm::vec2 lastMousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        glm::vec2 currentMousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        glm::vec2 mouseDelta = { 0.0f, 0.0f };

        // Only update mouseDelta when a mouse button is pressed
        bool mbPressed = Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) | Input::IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
        if (mbPressed)
        {
            mouseDelta = currentMousePos - lastMousePos;
        }
        else
        {
            mouseDelta = { 0.0f, 0.0f };
        }

        lastMousePos = currentMousePos;


        const f32 x = std::min(m_ViewportData.width * 0.01f, 1.8f);
        const f32 y = std::min(m_ViewportData.height * 0.01f, 1.8f);
        const f32 xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
        const f32 yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
        const f32 yawSign = m_ViewportCamera->GetUpDirection().y < 0 ? -1.0f : 1.0f;

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
                m_ViewportCamera->position.x += mouseDelta.x * xFactor * m_CameraData.moveSpeed * 0.03f;
                m_ViewportCamera->position.y += -mouseDelta.y * yFactor * m_CameraData.moveSpeed * 0.03f;
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
}