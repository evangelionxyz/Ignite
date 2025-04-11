#pragma once

#include <nvrhi/nvrhi.h>
#include <glm/glm.hpp>
namespace ignite
{
    struct VertexQuad
    {
        glm::vec2 position;
        glm::vec2 texCoord;
        glm::vec4 color;

        static std::array<nvrhi::VertexAttributeDesc, 3> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(VertexQuad, position))
                    .setElementStride(sizeof(VertexQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXCOORD")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(VertexQuad, texCoord))
                    .setElementStride(sizeof(VertexQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexQuad, color))
                    .setElementStride(sizeof(VertexQuad))
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
}
