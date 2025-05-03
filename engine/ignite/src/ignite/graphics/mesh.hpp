#pragma once

#include "ignite/asset/asset.hpp"
#include "vertex_data.hpp"

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

        Material();
    };

    struct MeshCreateInfo
    {
        MeshType type = MeshType::Static;
        nvrhi::IDevice *device = nullptr;

        Material material = Material();
        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;
    };

    class Mesh : public Asset
    {
    public:
        Mesh(const std::filesystem::path &filepath, MeshCreateInfo *createInfo);

        MeshCreateInfo GetCreateInfo() const { return *m_CreateInfo; }

    private:
        MeshCreateInfo *m_CreateInfo = nullptr;
        std::vector<VertexMesh> vertices;
    };

}