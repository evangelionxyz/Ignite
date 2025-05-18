#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "icomponent.hpp"
#include "ignite/graphics/material.hpp"
#include "ignite/core/uuid.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <nvrhi/nvrhi.h>
#include <string>

namespace ignite
{
    class Texture;
    class EntityMesh;
    class Model;
    struct Material;

    static std::unordered_map<std::string, CompType> s_ComponentsName =
    {
        { "Rigid Body 2D", CompType_Rigidbody2D },
        { "Sprite 2D", CompType_Sprite2D},
        { "Box Collider 2D", CompType_BoxCollider2D },
        { "Skinned Mesh Renderer", CompType_SkinnedMeshRenderer },
        { "Static Mesh hRenderer", CompType_StaticMeshRenderer },
    };

    enum EntityType : u8
    {
        EntityType_Node,
        EntityType_Camera,
        EntityType_Mesh,
        EntityType_Prefab,
        EntityType_Invalid
    };

    static const char *EntityTypeToString(EntityType type)
    {
        switch (type)
        {
        case EntityType_Node: return "Node";
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
        if (typeStr == "Node") return EntityType_Node;
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

        // cached matrices
        glm::mat4 localMatrix = glm::mat4(1.0f);
        glm::mat4 worldMatrix = glm::mat4(1.0f);

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

        // local transformation
        void SetLocalTranslation(const glm::vec3 &newTranslation)
        {
            localTranslation = newTranslation;
            dirty = true;
        }

        void SetLocalRotation(const glm::quat &newRotation)
        {
            localRotation = newRotation;
            dirty = true;
        }

        void SetLocalScale(const glm::vec3 &newScale)
        {
            localScale = newScale;
            dirty = true;
        }

        void UpdateLocalMatrix()
        {
            localMatrix = glm::translate(glm::mat4(1.0f), localTranslation) 
                * glm::mat4(localRotation) * glm::scale(glm::mat4(1.0f), localScale);
        }

        // World transformation
        void SetWorldTranslation(const glm::vec3 &newTranslation)
        {
            translation = newTranslation;
            dirty = true;
        }

        void SetWorldRotation(const glm::quat &newRotation)
        {
            rotation = newRotation;
            dirty = true;
        }

        void SetWorldScale(const glm::vec3 &newScale)
        {
            scale = newScale;
            dirty = true;
        }

        void UpdateWorldMatrix()
        {
            worldMatrix = glm::translate(glm::mat4(1.0f), translation)
                * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
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

    class MeshRenderer
    {
    public:
        Material material;

        nvrhi::RasterCullMode cullMode = nvrhi::RasterCullMode::Front;
        nvrhi::RasterFillMode fillMode = nvrhi::RasterFillMode::Solid;
    };

    class SkinnedMeshRenderer : public IComponent, public MeshRenderer
    {
    public:
        // Root joint

        Ref<EntityMesh> mesh;
        UUID parentNode = UUID(0);

        static CompType StaticType() { return CompType_SkinnedMeshRenderer; }
        virtual CompType GetType() override { return StaticType(); }
    };

    class StaticMeshRenderer : public IComponent, public MeshRenderer
    {
    public:

        static CompType StaticType() { return CompType_StaticMeshRenderer; }
        virtual CompType GetType() override { return StaticType(); }
    };
}
