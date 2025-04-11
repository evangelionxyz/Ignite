#pragma once

#include "ignite/core/types.hpp"

#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace ignite
{
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
    void CreatePerspective(f32 fov, f32 width, f32 height, f32 nearClip, f32 farClip);

    void UpdateProjectionMatrix();
    void UpdateViewMatrix();

    void SetSize(f32 w, f32 h);

    const std::string &GetName() const { return m_Name; }
    glm::vec2 GetSize();
    glm::mat4 GetViewProjectionMatrix() const { return projectionMatrix * viewMatrix; }

    glm::vec3 GetUpDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetForwardDirection() const;
    f32 GetAspectRatio() const { return m_AspectRatio; }

    f32 zoom, width, height;
    f32 yaw, pitch, fov;
    f32 nearClip, farClip;

    glm::vec3 position;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    Type projectionType;

  protected:
    f32 m_AspectRatio = 1.0f;
    std::string m_Name;
  };
}
