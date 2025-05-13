#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include "ignite/core/uuid.hpp"
#include "icomponent.hpp"

namespace ignite
{
    class Texture;
    class Mesh;
    class Model;

    enum EntityType : u8
    {
        EntityType_Common = 0,
        EntityType_Camera,
        EntityType_Mesh,
        EntityType_Prefab,
        EntityType_Invalid
    };

    static const char *EntityTypeToString(EntityType type)
    {
        switch (type)
        {
        case EntityType_Common: return "Common";
        case EntityType_Camera: return "Camera";
        case EntityType_Mesh: return "Mesh";
        case EntityType_Prefab: return "Prefab";
        case EntityType_Invalid:
        default:
            return "Invalid";
        }
    }

    static EntityType EntityTypeFromString(const std::string &typeStr)
    {
        if (typeStr == "Common") return EntityType_Common;
        else if (typeStr == "Camera") return EntityType_Camera;
        else if (typeStr == "Mesh") return EntityType_Mesh;
        else if (typeStr == "Prefab") return EntityType_Prefab;
        return EntityType_Invalid;
    }

    class ID : public IComponent
    {
    public:
        std::string name;
        UUID uuid;
        EntityType type;

        UUID parent = UUID(0);
        std::vector<UUID> children;

        void AddChild(UUID childId)
        {
            children.push_back(childId);
        }

        void RemoveChild(UUID childId)
        {
            children.erase(std::remove_if(children.begin(), children.end(), [childId](const UUID id) 
            { 
                return id == childId;
            }), children.end());
        }

        bool HasChild() const
        {
            return !children.empty();
        }

        ID(const std::string &_name,  EntityType _type, const UUID &_uuid = UUID())
            : name(_name)
            , type(_type)
            , uuid(_uuid)
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
        glm::vec3 localTranslation, localScale;
        glm::quat localRotation;

        bool visible = true;

        Transform() = default;

        Transform(const glm::vec3 &_translation)
            : translation(_translation)
            , rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
            , scale(glm::vec3(1.0f))
            , localTranslation(_translation)
            , localRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
            , localScale(glm::vec3(1.0f))
        {
        }

        Transform(const glm::vec3 &_translation, const glm::quat &_rot, const glm::vec3 _size)
            : translation(_translation)
            , rotation(_rot)
            , scale(_size)
            , localTranslation(_translation)
            , localRotation(_rot)
            , localScale(_size)
        {
        }

        glm::mat4 WorldTransform()
        {
            return glm::translate(glm::mat4(1.0f), translation)
                * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
        }

        glm::mat4 LocalTransform()
        {
            return glm::translate(glm::mat4(1.0f), localTranslation)
                * glm::toMat4(localRotation) * glm::scale(glm::mat4(1.0f), localScale);
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

    class MeshComponent : public IComponent
    {
        Ref<Mesh> mesh;

        static CompType StaticType() { return CompType_Mesh; }
        virtual CompType GetType() { return StaticType(); }
    };

    class ModelComponent : public IComponent
    {
    public:
        Ref<Model> model;

        static CompType StaticType() { return CompType_Mesh; }
        virtual CompType GetType() { return StaticType(); }
    };
}

