#pragma once

#include "vertex_data.hpp"
#include "material.hpp"

#include "ignite/asset/asset.hpp"
#include "ignite/core/uuid.hpp"
#include "ignite/core/uuid.hpp"
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
    };

    struct NodeInfo
    {
        i32 id = -1;
        i32 parentID = -1;

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

    class Mesh
    {
    public:
        std::vector<VertexMesh> vertices;
        std::vector<uint32_t> indices;
        Material material;

        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;
        
        std::string name;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BindingSetHandle bindingSet;

        nvrhi::BufferHandle objectBuffer;
        nvrhi::BufferHandle materialBuffer;

        glm::mat4 localTransform;
        glm::mat4 worldTransform;

        i32 nodeParentID = -1;
        i32 nodeID = -1; // ID of the bone this mesh is attached to

        std::vector<BoneInfo> boneInfo; // Bone weights and indices
        std::unordered_map<std::string, uint32_t> boneMapping; // Maps bone namse to indices

        void CreateConstantBuffers(nvrhi::IDevice *device);
        void CreateBuffers();
    };

    class EntityMesh
    {
    public:
        std::vector<VertexMesh> vertices;
        std::vector<uint32_t> indices;

        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BindingSetHandle bindingSet;

        nvrhi::BufferHandle objectBuffer;
        nvrhi::BufferHandle materialBuffer;

        std::vector<BoneInfo> boneInfo; // Bone weights and indices
        std::unordered_map<std::string, uint32_t> boneMapping; // Maps bone namse to indices

        void CreateConstantBuffers(nvrhi::IDevice *device);
        void CreateBuffers();
    };
    
    class MeshLoader
    {
    public:

        static void ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &mesh, std::vector<NodeInfo> &nodes, const Skeleton &skeleton, i32 parentNodeID);
        template<typename T>
        static void LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, Ref<T> &meshes, const std::string &filepath, const Skeleton &skeleton);

        template<typename T>
        static void ProcessBoneWeights(aiMesh *assimpMesh, Ref<T> &mesh, const Skeleton &skeleton);

        static void ExtractSkeleton(const aiScene *scene, Skeleton &skeleton);
        static void ExtractSkeletonRecursive(aiNode *node, i32 parentJointId, Skeleton &skeleton, const std::unordered_map<std::string, glm::mat4> &inverseBindMatrices);
        static void SortJointsHierchically(Skeleton &skeleton);
        static void LoadAnimation(const aiScene *scene, std::vector<Ref<SkeletalAnimation>> &animations);
        static void LoadMaterial(const aiScene *scene, aiMaterial *assimpMaterial, const std::string &filepath, Material &material);
        static void LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type);
        static void CalculateWorldTransforms(std::vector<NodeInfo> &nodes, std::vector<Ref<Mesh>> &meshes);
        static void ClearTextureCache();
    };
}