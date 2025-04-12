#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include "ignite/core/uuid.hpp"

namespace ignite
{
    class Texture;

    enum CompType : u8
    {
        CompType_Invalid = 0,
        CompType_ID = 1,
        CompType_Transform,
        CompType_Sprite2D,
        CompType_BoxCollider2D,
        CompType_Rigidbody2D
    };

    class IComponent
    {
    public:
        template<typename T>
        T *As()
        {
            return static_cast<T *>(this);
        }

        virtual CompType GetType() { return CompType_Invalid; };
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

        static CompType StaticType() { return CompType_ID; }
        virtual CompType GetType() override { return StaticType(); }
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

        static CompType StaticType() { return CompType_Transform; }
        virtual CompType GetType() override { return StaticType(); }
    };

    class Sprite2D : public IComponent
    {
    public:
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec2 tilingFactor = { 1.0f, 1.0f };
        Ref<Texture> texture = nullptr;

        static CompType StaticType() { return CompType_Sprite2D; }
        virtual CompType GetType() override { return StaticType(); }
    };

}

