#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "icomponent.hpp"
#include "ignite/graphics/material.hpp"
#include "ignite/core/uuid.hpp"
#include "ignite/math/aabb.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/vertex_data.hpp"
#include "ignite/animation/skeletal_animation.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <nvrhi/nvrhi.h>
#include <string>

namespace ignite
{
    class Texture;

    static std::unordered_map<std::string, CompType> s_ComponentsName =
    {
        { "Rigid Body 2D", CompType_Rigidbody2D },
        { "Sprite 2D", CompType_Sprite2D},
        { "Box Collider 2D", CompType_BoxCollider2D },
        { "Mesh Renderer", CompType_MeshRenderer },
        { "Skinned Mesh", CompType_SkinnedMesh},
    };

    enum EntityType : u8
    {
        EntityType_Node,
        EntityType_Camera,
        EntityType_Mesh,
        EntityType_Prefab,
        EntityType_Joint,
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
        case EntityType_Joint: return "Joint";
        case EntityType_Invalid:
        default: return "Invalid";
        }
    }

    static EntityType EntityTypeFromString(const std::string &typeStr)
    {
        if (typeStr == "Node") return EntityType_Node;
        if (typeStr == "Camera") return EntityType_Camera;
        if (typeStr == "Mesh") return EntityType_Mesh;
        if (typeStr == "Prefab") return EntityType_Prefab;
        if (typeStr == "Joint") return EntityType_Prefab;
        return EntityType_Invalid;
    }

    class ID final : public IComponent
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

        bool isAnimated = false;
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

        Transform(const glm::vec3 &_translation, const glm::quat &_rotation, const glm::vec3 _scale)
            : translation(_translation)
            , rotation(_rotation)
            , scale(_scale)
            , localTranslation(_translation)
            , localRotation(_rotation)
            , localScale(_scale)
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

        glm::mat4 GetLocalMatrix() const
        {
            return glm::translate(glm::mat4(1.0f), localTranslation) * glm::mat4(localRotation) * glm::scale(glm::mat4(1.0f), localScale);
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

        glm::mat4 GetWorldMatrix() const
        {
            return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
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

     class SkinnedMesh : public IComponent
     {
     public:
         Ref<Skeleton> skeleton;
         std::vector<glm::mat4> boneTransforms;
         std::vector<Ref<SkeletalAnimation>> animations;
         i32 activeAnimIndex = 0;

         static CompType StaticType() { return CompType_SkinnedMesh; }
         virtual CompType GetType() override { return StaticType(); };
     };

    class MeshRenderer : public IComponent
    {
    public:
        Ref<Mesh> mesh;
        AssetHandle meshSource = AssetHandle(0); // actual .glb, .gltf, .fbx file
        int meshIndex = -1; // submesh index

        UUID root = UUID(0);

        ObjectBuffer meshBuffer;

        nvrhi::RasterCullMode cullMode = nvrhi::RasterCullMode::Front;
        nvrhi::RasterFillMode fillMode = nvrhi::RasterFillMode::Solid;

        MeshRenderer() = default;
        MeshRenderer(const MeshRenderer &other)
        {
            if (!other.mesh)
                return;

            mesh = CreateRef<Mesh>(*other.mesh.get());
            cullMode = other.cullMode;
            fillMode = other.fillMode;

            meshBuffer = other.meshBuffer;
            meshSource = other.meshSource;
            meshIndex = other.meshIndex;
        }

        static CompType StaticType() { return CompType_MeshRenderer; }
        virtual CompType GetType() override { return StaticType(); }
    };
}
