#include "mesh.hpp"

#include "renderer.hpp"
#include "texture.hpp"

namespace ignite {

    // Material class
    Material::Material()
        : color(glm::vec4(1.0f))
    {
        texture = Renderer::GetWhiteTexture()->GetHandle();
    }

    // Mesh class
    Mesh::Mesh(const std::filesystem::path &filepath, MeshCreateInfo *createInfo)
        : m_CreateInfo(createInfo)
    {

    }

}