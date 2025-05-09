#pragma once

#include "ignite/asset/asset.hpp"
#include "vertex_data.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "ignite/core/uuid.hpp"
#include "ignite/core/buffer.hpp"

#include <assimp/postprocess.h>
#include <nvrhi/nvrhi.h>
#include <filesystem>

namespace ignite {

    class Shader;
    class Camera;

    enum class MeshType
    {
        Static,
        Skeletal
    };

    struct Material
    {
        glm::vec4 baseColor;
        glm::vec4 diffuseColor;
        f32 emissive = 0.0f;

        nvrhi::TextureHandle texture = nullptr;
        nvrhi::SamplerHandle sampler = nullptr;

        bool IsTransparent() const { return _transparent; }
        bool IsReflective() const { return _reflective; }
        bool ShouldWriteTexture() const { return _shouldWriteTexture; }

        void WriteBuffer(nvrhi::CommandListHandle commandList)
        {
            if (_buffer.Data)
            {
                commandList->writeTexture(texture, 0, 0, _buffer.Data, _textureWidth * 4);
            }
        }

    private:
        bool _transparent = false;
        bool _reflective = false;
        bool _shouldWriteTexture = false;
        uint32_t _textureWidth = 0;
        Buffer _buffer;

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
        nvrhi::BufferHandle modelConstantBuffer;
        nvrhi::BufferHandle materialConstantBuffer;
        nvrhi::BindingSetHandle bindingSet;

        // TODO: bone transform

        i32 meshID;
        i32 parentID;

        std::vector<i32> children;

        void CreateBuffers();
    };

    class Model
    {
    public:
        Model() = default;
        Model(nvrhi::IDevice *device, nvrhi::BindingLayoutHandle bindingLayout, const std::filesystem::path &filepath);

        void WriteBuffer(nvrhi::CommandListHandle commandList);
        void OnUpdate(f32 deltaTime);
        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera);

        std::vector<Ref<Mesh>> &GetMeshes() { return m_Meshes; }

        static Ref<Model> Create(nvrhi::IDevice *device, nvrhi::BindingLayoutHandle bindingLayout, const std::filesystem::path &filepath);

        nvrhi::BufferHandle globalConstantBuffer;
    private:

        std::vector<Ref<Mesh>> m_Meshes;
    };

    class ModelLoader
    {
    public:
        static void ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, i32 parentID = -1);
        static void LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const std::string &filepath, std::vector<Ref<Mesh>> &meshes);
        static void LoadMaterial(const aiScene *scene, aiMaterial *material, const std::string &filepath, Ref<Mesh> &mesh);

    private:
        static Assimp::Importer m_Importer;
    };
}