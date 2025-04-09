#include "camera.hpp"
#include "core/logger.hpp"

Camera::Camera(const std::string &name)
    : m_Name(name)
    , m_Position({0.0f, 0.0f, 0.0f})
    , m_AspectRatio(16.0f / 9.0f)
    , m_Zoom(1.0f)
    , m_ProjectionMatrix(glm::mat4(1.0f))
    , m_ViewMatrix(glm::mat4(1.0f))
    , m_NearClip(0.1f)
    , m_FarClip(300.0f)
    , m_Width(1280.0f)
    , m_Height(720.0f)
    , m_Fov(45.0f)
    , m_ProjectionType(Type::Unknown)
{
}
    
void Camera::CreateOrthographic(f32 width, f32 height, f32 zoom, f32 nearClip, f32 farClip)
{
    m_ProjectionType = Type::Orthographic;

    m_NearClip = nearClip;
    m_FarClip = farClip;

    SetSize(width, height);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera::CreatePerspective(f32 fovy, f32 width, f32 height, f32 nearClip, f32 farClip)
{
    m_ProjectionType = Type::Perspective;
    m_Fov = fovy;
    m_NearClip = nearClip;
    m_FarClip = farClip;

    SetSize(width, height);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>(BIND_CLASS_EVENT_FN(Camera::OnScrollEvent));
}

bool Camera::OnScrollEvent(MouseScrolledEvent &e)
{
    MouseZoom(e.GetYOffset());
    return false;
}

void Camera::MouseZoom(const f32 delta)
{
    LOG_INFO("Zoom: {} {} ", m_Zoom, delta);

    switch (m_ProjectionType)
    {
    case Type::Perspective:
        break;
    case Type::Orthographic:
        m_Zoom -= delta;
        m_Zoom = glm::clamp(m_Zoom, 1.0f, 100.0f);
        break;
    }
}

void Camera::SetPosition(const glm::vec3 &position)
{
    m_Position = position;
}

void Camera::SetSize(f32 width, f32 height)
{
    m_Width = width;
    m_Height = height;
    m_AspectRatio = m_Width / m_Height;
}

void Camera::SetZoom(f32 zoom)
{
    m_Zoom = zoom;
}

void Camera::UpdateProjectionMatrix()
{
    switch (m_ProjectionType)
    {
    default:
    case Type::Orthographic:
    {
        f32 orthoWidth = m_Zoom * m_AspectRatio / 2.0f;
        f32 orthoHeight = m_Zoom / 2.0f;
        m_ProjectionMatrix = glm::ortho(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, m_NearClip, m_FarClip);
        break;
    }
    case Type::Perspective:
    {
        m_ProjectionMatrix = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearClip, m_FarClip);
        break;
    }
    }
}

void Camera::UpdateViewMatrix()
{
    switch (m_ProjectionType)
    {
    case Type::Orthographic:
        m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position);
        break;
    case Type::Perspective:
        m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(glm::quat({ -m_Pitch, -m_Yaw, 0.0f }));
        break;
    }
    m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

glm::vec2 Camera::GetSize()
{
    return { m_Width, m_Height} ;
}

const f32 Camera::GetZoom() const
{
    return m_Zoom;
}

glm::vec3 Camera::GetUpDirection() const
{
    return glm::rotate(glm::quat({ -m_Pitch, -m_Yaw, 0.0f }), { 0.0f, 1.0f, 0.0f });
}

glm::vec3 Camera::GetRightDirection() const
{
    return glm::rotate(glm::quat({ -m_Pitch, -m_Yaw, 0.0f }), { 1.0f, 0.0f, 0.0f });
}

glm::vec3 Camera::GetForwardDirection() const
{
    return glm::rotate(glm::quat({ -m_Pitch, -m_Yaw, 0.0f }), { 0.0f, 0.0f, -1.0f });
}

