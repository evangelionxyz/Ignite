#pragma once

#include "vertex_data.hpp"
#include "material.hpp"

#include "renderer.hpp"

#include "ignite/asset/asset.hpp"
#include "ignite/core/uuid.hpp"
#include "ignite/math/aabb.hpp"
#include "ignite/core/buffer.hpp"
#include "ignite/animation/skeletal_animation.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <nvrhi/nvrhi.h>
#include <filesystem>

namespace ignite {

#define ASSIMP_IMPORTER_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

    class Shader;
    class Camera;
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

        std::vector<VertexMeshOutline> outlineVertices;

        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;

        // do not copy the buffer
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle outlineVertexBuffer;
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

            // outline
            outlineVertices = other.outlineVertices;

            CreateBuffers();
        }

        void CreateBuffers();
    };
    
    class MeshLoader
    {
    public:

        static void ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &mesh, std::vector<NodeInfo> &nodes, const Ref<Skeleton> &skeleton, i32 parentNodeID);
        static void LoadSingleMesh(const std::string &filepath, const aiScene *scene, aiMesh *mesh, const uint32_t meshIndex, MeshData &outMeshData, const Ref<Skeleton> &skeleton, AABB &outAABB);
        static void ProcessBoneWeights(aiMesh *assimpMesh, MeshData &outMeshData, std::vector<BoneInfo> &outBoneInfo, std::unordered_map<std::string, uint32_t> &outBoneMapping, const Ref<Skeleton> &skeleton);

        static void ExtractSkeleton(const aiScene *scene, Ref<Skeleton> &skeleton);
        static void ExtractSkeletonRecursive(aiNode *node, i32 parentJointId, Ref<Skeleton> &skeleton, const std::unordered_map<std::string, glm::mat4> &inverseBindMatrices);
        static void SortJointsHierarchically(Ref<Skeleton> &skeleton);
        static void LoadAnimation(const aiScene *scene, std::vector<Ref<SkeletalAnimation>> &animations);
        static void LoadMaterial(const aiScene *scene, aiMaterial *assimpMaterial, const std::string &filepath, Material &material);
        static void LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type);
        static void CalculateWorldTransforms(std::vector<NodeInfo> &nodes);
        static void ClearTextureCache();
    };
}