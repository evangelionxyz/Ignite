#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/key_codes.hpp"
#include "ignite/core/input/event.hpp"
#include "ignite/core/input/mouse_event.hpp"

#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>


class Camera
{
public:
    enum class Type
    {
        Unknown, Perspective, Orthographic
    };

    Camera() = default;
    Camera(const std::string &name);

    void CreateOrthographic(f32 width, f32 height, f32 zoom, f32 nearClip, f32 farClip);
    void CreatePerspective(f32 fovy, f32 width, f32 height, f32 nearClip, f32 farClip);

    void SetPosition(const glm::vec3 &position);
    void SetSize(f32 width, f32 height);
    void SetZoom(f32 zoom);
    void SetClipValue(f32 nearClip, f32 farClip);
    void SetFov(f32 fov);
    void SetYaw(f32 yaw);
    void SetPitch(f32 pitch);

    void UpdateProjectionMatrix();
    void UpdateViewMatrix();

    const std::string &GetName() const { return m_Name; }
    glm::vec2 GetSize();
    const glm::mat4 &GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4 &GetProjectionMatrix() const { return m_ProjectionMatrix; }
    glm::mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
    const glm::vec3 &GetPosition() const { return m_Position; }

    glm::vec3 GetUpDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetForwardDirection() const;

    const f32 GetZoom() const;
    const f32 GetNearClip() const;
    const f32 GetFarClip() const;
    const f32 GetFov() const;
    const f32 GetYaw() const;
    const f32 GetPitch() const;

    Type GetProjectionType() const { return m_ProjectionType; }

protected:
    

    f32 m_AspectRatio, m_Zoom, m_Width, m_Height;
    f32 m_Yaw, m_Pitch, m_Fov;
    f32 m_NearClip, m_FarClip;

    glm::vec3 m_Position;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjectionMatrix;
    Type m_ProjectionType;

    std::string m_Name;
};