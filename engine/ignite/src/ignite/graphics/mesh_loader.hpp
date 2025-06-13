#pragma once

#include "mesh.hpp"

namespace ignite
{
    class MeshLoader
    {
    public:        
        static void ProcessNode(const aiScene *scene, aiNode *node, const std::filesystem::path &filepath, std::vector<Ref<Mesh>> &mesh, std::vector<NodeInfo> &nodes, const Ref<Skeleton> &skeleton, i32 parentNodeID);
        static void LoadSingleMesh(const aiScene *scene, aiMesh *mesh, const uint32_t meshIndex, MeshData &outMeshData, const Ref<Skeleton> &skeleton, AABB &outAABB);
        static void ProcessBoneWeights(aiMesh *assimpMesh, MeshData &outMeshData, std::vector<BoneInfo> &outBoneInfo, std::unordered_map<std::string, uint32_t> &outBoneMapping, const Ref<Skeleton> &skeleton);

        static void ExtractSkeleton(const aiScene *scene, Ref<Skeleton> &skeleton);
        static void ExtractSkeletonRecursive(aiNode *node, i32 parentJointId, Ref<Skeleton> &skeleton, const std::unordered_map<std::string, glm::mat4> &inverseBindMatrices);
        static void SortJointsHierarchically(Ref<Skeleton> &skeleton);
        static void LoadAnimation(const aiScene *scene, std::vector<SkeletalAnimation> &animations);
        static void LoadMaterial(const aiScene *scene, aiMaterial *assimpMaterial, Material &material, const std::filesystem::path &filepath);
        static void LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type, const std::filesystem::path &modelFilepath);
        static void CalculateWorldTransforms(std::vector<NodeInfo> &nodes);
        static void ClearTextureCache();
    };    
}