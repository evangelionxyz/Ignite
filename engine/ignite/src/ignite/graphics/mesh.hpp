#pragma once

#include "vertex_data.hpp"
#include "material.hpp"

#include "renderer.hpp"

#include "ignite/core/uuid.hpp"
#include "ignite/math/aabb.hpp"
#include "ignite/animation/skeletal_animation.hpp"
#include "ignite/scene/entity.hpp"

#include <assimp/Importer.hpp>
#include <nvrhi/nvrhi.h>
#include <filesystem>

namespace ignite {

#define ASSIMP_IMPORTER_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

    class Shader;
    class Environment;
    class GraphicsPipeline;
    class Scene;

    struct Joint
    {
        std::string name;
        i32 id; // index in joints array
        i32 parentJointId; // parent in skeleton hierarchy (-1 for root)
        glm::mat4 inverseBindPose; // inverse bind pose matrix
        glm::mat4 localTransform; // current local transform
        glm::mat4 globalTransform; // current global transform
    };

    struct Skeleton
    {
        std::vector<Joint> joints;
        std::unordered_map<std::string, i32> nameToJointMap; // for fast lookup by name

        std::unordered_map<i32, UUID> jointEntityMap;
    };

    struct NodeInfo
    {
        i32 id = -1;
        i32 parentID = -1;
        bool isJoint = false;
        
        UUID uuid = UUID(0);

        std::string name;
        glm::mat4 localTransform;
        glm::mat4 worldTransform;
        std::vector<i32> childrenIDs;
        std::vector<i32> meshIndices;  // Meshes owned by this node
    };

    struct BoneInfo
    {
        float weights[MAX_BONES] = { 0.0f };
        glm::mat4 offsetMatrix = glm::mat4(1.0f);
    };

    struct MeshData
    {
        std::vector<VertexMesh> vertices;
        std::vector<uint32_t> indices;

        int materialIndex = -1;
    };

    class Mesh
    {
    public:
        std::string name;

        MeshData data;
        Material material;
        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;
        Ref<Environment> environment;

        // do not copy the buffer
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BufferHandle objectBufferHandle;
        nvrhi::BufferHandle materialBufferHandle;
        std::unordered_map<GPipeline, nvrhi::BindingSetHandle> bindingSets;

        i32 nodeParentID = -1;
        i32 nodeID = -1; // ID of the bone this mesh is attached to

        std::vector<BoneInfo> boneInfo; // Bone weights and indices
        std::unordered_map<std::string, uint32_t> boneMapping; // Maps bone name to indices

        AABB aabb;

        Mesh() = default;
        Mesh(const Mesh &other)
        {
            data = other.data;

            vertexShader = other.vertexShader;
            pixelShader = other.pixelShader;
            material = other.material;
            boneInfo = other.boneInfo;
            boneMapping = other.boneMapping;
            aabb = other.aabb;

            CreateBuffers();
        }

        void CreateBuffers();
        void CreateBindingSet();
        void WriteBuffers(uint32_t entityID = -1);
        void UpdateTexture(Ref<Texture> texture, aiTextureType type);
    };
    
}