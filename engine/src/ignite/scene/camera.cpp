#include "camera.hpp"
#include "ignite/core/logger.hpp"

Camera::Camera(const std::string &name)
    : m_Name(name)
    , position({0.0f, 0.0f, 0.0f})
    , m_AspectRatio(16.0f / 9.0f)
    , zoom(1.0f)
    , projectionMatrix(glm::mat4(1.0f))
    , viewMatrix(glm::mat4(1.0f))
    , nearClip(0.1f)
    , farClip(300.0f)
    , width(1280.0f)
    , height(720.0f)
    , fov(45.0f)
    , projectionType(Type::Unknown)
{
}
    
void Camera::CreateOrthographic(f32 width, f32 height, f32 zoom, f32 nearClip, f32 farClip)
{
    projectionType = Type::Orthographic;

    this->nearClip = nearClip;
    this->farClip = farClip;
    this->zoom = zoom;

    SetSize(width, height);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera::CreatePerspective(f32 fov, f32 width, f32 height, f32 nearClip, f32 farClip)
{
    projectionType = Type::Perspective;
    this->fov = fov;
    this->nearClip = nearClip;
    this->farClip = farClip;

    SetSize(width, height);
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}
void Camera::SetSize(const f32 w, const f32 h)
{
    width = w;
    height = h;
    m_AspectRatio = width / height;
}

void Camera::UpdateProjectionMatrix()
{
    switch (projectionType)
    {
    default:
    case Type::Orthographic:
    {
        f32 orthoWidth = zoom * m_AspectRatio / 2.0f;
        f32 orthoHeight = zoom / 2.0f;
        projectionMatrix = glm::ortho(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, nearClip, farClip);
        break;
    }
    case Type::Perspective:
    {
        projectionMatrix = glm::perspective(glm::radians(fov), m_AspectRatio, nearClip, farClip);
        break;
    }
    }
}

void Camera::UpdateViewMatrix()
{
    switch (projectionType)
    {
    case Type::Orthographic:
    default:
        viewMatrix = glm::translate(glm::mat4(1.0f), position);
        break;
    case Type::Perspective:
        viewMatrix = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat({ -pitch, -yaw, 0.0f }));
        break;
    }
    viewMatrix = glm::inverse(viewMatrix);
}

glm::vec2 Camera::GetSize()
{
    return { width, height} ;
}

glm::vec3 Camera::GetUpDirection() const
{
    return glm::rotate(glm::quat({ -pitch, -yaw, 0.0f }), { 0.0f, 1.0f, 0.0f });
}

glm::vec3 Camera::GetRightDirection() const
{
    return glm::rotate(glm::quat({ -pitch, -yaw, 0.0f }), { 1.0f, 0.0f, 0.0f });
}

glm::vec3 Camera::GetForwardDirection() const
{
    return glm::rotate(glm::quat({ -pitch, -yaw, 0.0f }), { 0.0f, 0.0f, -1.0f });
}

