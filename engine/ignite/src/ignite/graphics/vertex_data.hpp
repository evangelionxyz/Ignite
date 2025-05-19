#pragma once

#include <nvrhi/nvrhi.h>
#include <glm/glm.hpp>

#include "ignite/core/types.hpp"

namespace ignite
{
#define VERTEX_MAX_BONES 4
#define MAX_BONES 150

    struct CameraBuffer
    {
        glm::mat4 viewProjection;
        glm::vec4 position;
    };

    struct ObjectBuffer
    {
        glm::mat4 transformation;
        glm::mat4 normal;
        glm::mat4 boneTransforms[MAX_BONES];
    };

    struct VertexMesh
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec2 tilingFactor;
        glm::vec4 color;
        uint32_t boneIDs[VERTEX_MAX_BONES] = { 0 };
        f32 weights[VERTEX_MAX_BONES] = { 0.0f };

        static std::array<nvrhi::VertexAttributeDesc, 7> GetAttributes()
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
                    .setName("TILINGFACTOR")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(VertexMesh, tilingFactor))
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexMesh, color))
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("BONEIDS")
                    .setFormat(nvrhi::Format::RGBA32_UINT)
                    .setOffset(offsetof(VertexMesh, boneIDs))
                    .setArraySize(VERTEX_MAX_BONES)
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("WEIGHTS")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexMesh, weights))
                    .setArraySize(VERTEX_MAX_BONES)
                    .setElementStride(sizeof(VertexMesh))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            return nvrhi::BindingLayoutDesc()
                .setVisibility(nvrhi::ShaderType::All)
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0)) // camera
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1)) // directional light
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2)) // environment
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(3)) // model
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(4)) // material
                .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(5)) // debug

                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0)) // diffuse
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1)) // specular
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(2)) // emissive
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(3)) // roughness
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4)) // normals
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5)) // texture cube
                .addItem(nvrhi::BindingLayoutItem::Sampler(0)); // sampler
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
            const uint16_t maxTextureCount = 16;

            nvrhi::BindingLayoutDesc bindingDesc;
            bindingDesc.setVisibility(nvrhi::ShaderType::All);
            bindingDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            bindingDesc.addItem(nvrhi::BindingLayoutItem::Sampler(0));

            // Add each texture binding individually
            for (i32 i = 0; i < maxTextureCount; ++i)
            {
                bindingDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(i));
            }

            return bindingDesc;
        }
    };
}
