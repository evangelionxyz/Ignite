#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

#include "ignite/core/uuid.hpp"

namespace ignite
{
class IComponent
{
public:
};

class ID : public IComponent
{
public:
    std::string name;
    UUID uuid;

    ID(const std::string &_name, const UUID &_uuid = UUID())
        : name(_name), uuid(_uuid)
    {
    }
};

class Transform : public IComponent
{
public:

    // world transforms
    glm::vec3 translation, scale;
    glm::quat rotation;

    // local transforms
    glm::vec3 local_translation, local_scale;
    glm::quat local_rotation;

    Transform() = default;

    Transform(const glm::vec3 &_translation)
        : translation(_translation), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(glm::vec3(1.0f))
    {
    }

    Transform(const glm::vec3 &_translation, const glm::quat &_rot, const glm::vec3 _size)
        : translation(_translation), rotation(_rot), scale(_size)
    {
    }

    glm::mat4 WorldTransform()
    {
        return glm::translate(glm::mat4(1.0f), translation)
            * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }

    glm::mat4 LocalTransform()
    {
        return glm::translate(glm::mat4(1.0f), local_translation)
            * glm::toMat4(local_rotation) * glm::scale(glm::mat4(1.0f), local_scale);
    }
};

class Sprite2D : public IComponent
{
public:
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
};

class Box2DCollider : public IComponent
{
public:
    glm::vec2 size;
};

}

