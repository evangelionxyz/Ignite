#include "icamera.hpp"
#include "ignite/core/logger.hpp"

namespace ignite
{
    ICamera::ICamera()
    : position({0.0f, 0.0f, 0.0f})
    , m_AspectRatio(16.0f / 9.0f)
    , zoom(1.0f)
    , yaw(0.0f)
    , pitch(0.0f)
    , projectionMatrix(glm::mat4(1.0f))
    , viewMatrix(glm::mat4(1.0f))
    , nearClip(0.1f)
    , farClip(300.0f)
    , width(1280.0f)
    , height(720.0f)
    , fov(45.0f)
    , projectionType(Type::Orthographic)
    {
    }

    void ICamera::CreateOrthographic(f32 width, f32 height, f32 zoom, f32 nearClip, f32 farClip)
    {
        projectionType = Type::Orthographic;

        this->nearClip = nearClip;
        this->farClip = farClip;
        this->zoom = zoom;

        SetSize(width, height);
        UpdateProjectionMatrix();
        UpdateViewMatrix();
    }

    void ICamera::CreatePerspective(f32 fov, f32 width, f32 height, f32 nearClip, f32 farClip)
    {
        projectionType = Type::Perspective;
        this->fov = fov;
        this->nearClip = nearClip;
        this->farClip = farClip;

        SetSize(width, height);
        UpdateProjectionMatrix();
        UpdateViewMatrix();
    }
    void ICamera::SetSize(const f32 w, const f32 h)
    {
        width = w;
        height = h;
        m_AspectRatio = width / height;
    }

    void ICamera::UpdateProjectionMatrix()
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
                projectionMatrix = glm::perspectiveZO(glm::radians(fov), m_AspectRatio, nearClip, farClip);
                break;
            }
        }
    }

    void ICamera::UpdateViewMatrix()
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

    glm::vec2 ICamera::GetSize()
    {
        return { width, height} ;
    }

    glm::vec3 ICamera::GetUpDirection() const
    {
        return glm::rotate(glm::quat({ -pitch, -yaw, 0.0f }), { 0.0f, 1.0f, 0.0f });
    }

    glm::vec3 ICamera::GetRightDirection() const
    {
        return glm::rotate(glm::quat({ -pitch, -yaw, 0.0f }), { 1.0f, 0.0f, 0.0f });
    }

    glm::vec3 ICamera::GetForwardDirection() const
    {
        return glm::rotate(glm::quat({ -pitch, -yaw, 0.0f }), { 0.0f, 0.0f, -1.0f });
    }
}
