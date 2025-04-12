#pragma once

#include <nvrhi/nvrhi.h>
#include <glm/glm.hpp>

namespace ignite
{
    struct VertexMesh
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec4 color;

        std::array<nvrhi::VertexAttributeDesc, 4> GetAttributes()
        {
            return 
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(VertexMesh, position))
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("NORMAL")
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(VertexMesh, normal))
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXCOORD")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(VertexMesh, texCoord))
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexMesh, color))
                    .setElementStride(sizeof(VertexMesh))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            return nvrhi::BindingLayoutDesc()
                .setVisibility(nvrhi::ShaderType::All)
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
                .addItem(nvrhi::BindingLayoutItem::Sampler(0))
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
        }
    };

    struct VertexQuad
    {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec2 tilingFactor;
        glm::vec4 color;
        u32 texIndex;

        static std::array<nvrhi::VertexAttributeDesc, 5> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setBufferIndex(0)
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(VertexQuad, position))
                    .setElementStride(sizeof(VertexQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXCOORD")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(VertexQuad, texCoord))
                    .setElementStride(sizeof(VertexQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TILINGFACTOR")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(VertexQuad, tilingFactor))
                    .setElementStride(sizeof(VertexQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexQuad, color))
                    .setElementStride(sizeof(VertexQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXINDEX")
                    .setFormat(nvrhi::Format::R32_UINT)
                    .setOffset(offsetof(VertexQuad, texIndex))
                    .setElementStride(sizeof(VertexQuad))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            const i32 maxTextureCount = 16;

            nvrhi::BindingLayoutDesc bindingDesc;
            bindingDesc.setVisibility(nvrhi::ShaderType::All);
            bindingDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));
            bindingDesc.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            for (i32 i = 0; i < maxTextureCount; ++i)
            {
                bindingDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(i));
            }
            return bindingDesc;
        }
    };
}
