#pragma once

#include "ignite/asset/asset.hpp"
#include "vertex_data.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "ignite/core/uuid.hpp"
#include "ignite/core/buffer.hpp"
#include "ignite/animation/skeletal_animation.hpp"

#include <assimp/postprocess.h>
#include <nvrhi/nvrhi.h>
#include <filesystem>

namespace ignite {

#define ASSIMP_IMPORTER_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

    class Shader;
    class Camera;
    class Environment;
    class GraphicsPipeline;

    struct MaterialData
    {
        glm::vec4 baseColor;
        f32 metallic = 0.0f;
        f32 roughness = 1.0f;
        f32 emissive = 0.0f;
    };

    struct Material
    {
        MaterialData data;

        struct TextureData
        {
            Buffer buffer;
            uint32_t rowPitch = 0;
            nvrhi::TextureHandle handle;
        };

        std::unordered_map<aiTextureType, TextureData> textures;

        nvrhi::SamplerHandle sampler = nullptr;

        bool IsTransparent() const { return _transparent; }
        bool IsReflective() const { return _reflective; }
        bool ShouldWriteTexture() const { return _shouldWriteTexture; }

        void WriteBuffer(nvrhi::CommandListHandle commandList)
        {
            for (auto [type, texData] : textures)
            {
                if (texData.buffer.Data)
                {
                    commandList->writeTexture(texData.handle, 0, 0, texData.buffer.Data, texData.rowPitch);
                }
            }
        }

    private:
        bool _transparent = false;
        bool _reflective = false;
        bool _shouldWriteTexture = false;

        friend class MeshLoader;
    };

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
        i32 parentID = -1;
        std::string name;
        glm::mat4 localTransform;
        glm::mat4 worldTransform;
        std::vector<i32> childrenIDs;
        std::vector<i32> meshIndices;  // Meshes owned by this node
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

        i32 nodeID = -1; // ID of the bone this mesh is attached to

        void CreateConstantBuffers(nvrhi::IDevice *device);
        void CreateBuffers();
    };
    
    struct ModelCreateInfo
    {
        nvrhi::IDevice *device;
        nvrhi::BufferHandle cameraBuffer;
        nvrhi::BufferHandle lightBuffer;
        nvrhi::BufferHandle envBuffer;
        nvrhi::BufferHandle debugBuffer;
    };

    class MeshLoader
    {
    public:
        static void ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, std::vector<NodeInfo> &nodes, const Skeleton &skeleton, i32 parentNodeID);
        static void LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, const Skeleton &skeleton);
        static void ProcessBodeWeights(aiMesh *mesh, std::vector<VertexMesh> &vertices, const Skeleton &skeleton);
        static void ExtractSkeleton(const aiScene *scene, Skeleton &skeleton);
        static void ExtractSkeletonRecursive(aiNode *node, i32 parentJointId, Skeleton &skeleton, const std::unordered_map<std::string, glm::mat4> &inverseBindMatrices);
        static void SortJointsHierchically(Skeleton &skeleton);
        static void LoadMaterial(const aiScene *scene, aiMaterial *material, const std::string &filepath, Ref<Mesh> &mesh);
        static void LoadAnimation(const aiScene *scene, std::vector<Ref<SkeletalAnimation>> &animations);
        static void LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type);
        static void CalculateWorldTransforms(std::vector<NodeInfo> &nodes, std::vector<Ref<Mesh>> &meshes);
        static void ClearTextureCache();
    };
}