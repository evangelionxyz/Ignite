#pragma once

#include "ignite/asset/asset.hpp"
#include "vertex_data.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "ignite/core/uuid.hpp"

#include <assimp/postprocess.h>
#include <nvrhi/nvrhi.h>
#include <filesystem>

namespace ignite {

    class Shader;

    enum class MeshType
    {
        Static,
        Skeletal
    };

    struct Material
    {
        glm::vec4 color;
        nvrhi::TextureHandle texture = nullptr;
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
        Ref<Material> material;

        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;
        
        std::string name;
        
        glm::mat4 localTransform;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;

        // TODO: bone transform

        i32 meshID;
        i32 parentID = -1;

        void CreateBuffers();
    };

    class Model
    {
    public:
        Model() = default;
        Model(nvrhi::IDevice *device, const std::filesystem::path &filepath);

        void WriteBuffer(nvrhi::CommandListHandle commandList);
        void OnUpdate(f32 deltaTime);
        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, nvrhi::BindingSetHandle bindingSet = nullptr);

        nvrhi::BufferHandle constantBuffer;

    private:
        std::vector<Ref<Mesh>> m_Meshes;
    };

    class ModelLoader
    {
    public:
        static void ProcessNode(const aiScene *scene, aiNode *node, const glm::mat4 &transform, const std::string &filepath, std::vector<Ref<Mesh>> &meshes);
        static void LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const glm::mat4 &transform, const std::string &filepath, std::vector<Ref<Mesh>> &meshes);

    private:
        static Assimp::Importer m_Importer;
    };
}