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

    class Shader;
    class Camera;
    class Environment;

    enum class MeshType
    {
        Static,
        Skeletal
    };

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

        friend class ModelLoader;
    };

    struct MeshCreateInfo
    {
        MeshType type = MeshType::Static;
        nvrhi::IDevice *device = nullptr;
    };

    class Mesh
    {
    public:
        Mesh(i32 id = 0);

        std::vector<VertexMesh> vertices;
        std::vector<uint32_t> indices;
        Material material;

        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;
        
        std::string name;
        glm::mat4 localTransform;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BindingSetHandle bindingSet;

        nvrhi::BufferHandle objectBuffer;
        nvrhi::BufferHandle materialBuffer;

        // TODO: bone transform

        i32 meshID;
        i32 parentID;

        std::vector<i32> children;
        void CreateBuffers();
    };

    struct ModelInputTexture
    {
        int index = -1;
        nvrhi::TextureHandle texture;
    };
    
    struct ModelCreateInfo
    {
        nvrhi::IDevice *device;
        nvrhi::IBindingLayout *bindingLayout;
        std::vector<ModelInputTexture> textures;

        nvrhi::BufferHandle cameraBuffer;
        nvrhi::BufferHandle lightBuffer;
        nvrhi::BufferHandle envBuffer;
        nvrhi::BufferHandle debugBuffer;
    };

    class Model
    {
    public:
        Model() = default;
        Model(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo);

        void WriteBuffer(nvrhi::CommandListHandle commandList);
        void OnUpdate(f32 deltaTime);
        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline);

        std::vector<Ref<Mesh>> &GetMeshes() { return m_Meshes; }

        static Ref<Model> Create(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo);

        glm::mat4 transform = glm::mat4(1.0f);
        std::vector<Ref<SkeletalAnimation>> animations;

    private:
        std::vector<Ref<Mesh>> m_Meshes;
        nvrhi::IBindingLayout *m_BindingLayout;
    };

    class ModelLoader
    {
    public:
        static void ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, i32 parentID = -1);
        static void LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const std::string &filepath, std::vector<Ref<Mesh>> &meshes);
        static void LoadMaterial(const aiScene *scene, aiMaterial *material, const std::string &filepath, Ref<Mesh> &mesh);
        static void LoadAnimation(const aiScene *scene, std::vector<Ref<SkeletalAnimation>> &animations);
        static void LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type);
    };
}