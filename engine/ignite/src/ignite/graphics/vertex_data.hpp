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
        u32 boneIDs[VERTEX_MAX_BONES] = { 0 };
        f32 weights[VERTEX_MAX_BONES] = { 0.0f };
        u32 entityID;

        static std::array<nvrhi::VertexAttributeDesc, 8> GetAttributes()
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
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("WEIGHTS")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexMesh, weights))
                    .setElementStride(sizeof(VertexMesh)),
                nvrhi::VertexAttributeDesc()
                    .setName("ENTITYID")
                    .setFormat(nvrhi::Format::R32_UINT)
                    .setOffset(offsetof(VertexMesh, entityID))
                    .setElementStride(sizeof(VertexMesh))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            return nvrhi::BindingLayoutDesc()
                .setVisibility(nvrhi::ShaderType::All)
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0)) // camera
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1)) // model
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2)) // directional light
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(3)) // environment
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(4)) // material

                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0)) // diffuse
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1)) // specular
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(2)) // emissive
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(3)) // roughness
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4)) // normals
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5)) // texture cube
                .addItem(nvrhi::BindingLayoutItem::Sampler(0)); // sampler
        }
    };

    struct Vertex2DQuad
    {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec2 tilingFactor;
        glm::vec4 color;
        u32 texIndex;
        u32 entityID;

        static std::array<nvrhi::VertexAttributeDesc, 6> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setBufferIndex(0)
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(Vertex2DQuad, position))
                    .setElementStride(sizeof(Vertex2DQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXCOORD")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(Vertex2DQuad, texCoord))
                    .setElementStride(sizeof(Vertex2DQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TILINGFACTOR")
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(Vertex2DQuad, tilingFactor))
                    .setElementStride(sizeof(Vertex2DQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(Vertex2DQuad, color))
                    .setElementStride(sizeof(Vertex2DQuad)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXINDEX")
                    .setFormat(nvrhi::Format::R32_UINT)
                    .setOffset(offsetof(Vertex2DQuad, texIndex))
                    .setElementStride(sizeof(Vertex2DQuad)),
                 nvrhi::VertexAttributeDesc()
                    .setName("ENTITYID")
                    .setFormat(nvrhi::Format::R32_UINT)
                    .setOffset(offsetof(Vertex2DQuad, entityID))
                    .setElementStride(sizeof(Vertex2DQuad))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            nvrhi::BindingLayoutDesc bindingDesc;
            bindingDesc.setVisibility(nvrhi::ShaderType::All);
            bindingDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            bindingDesc.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            bindingDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
            return bindingDesc;
        }
    };

    struct Vertex2DLine
    {
        glm::vec3 position;
        glm::vec4 color;
        u32 entityID;

        static std::array<nvrhi::VertexAttributeDesc, 3> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setBufferIndex(0)
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(Vertex2DLine, position))
                    .setElementStride(sizeof(Vertex2DLine)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(Vertex2DLine, color))
                    .setElementStride(sizeof(Vertex2DLine)),
                nvrhi::VertexAttributeDesc()
                    .setName("ENTITYID")
                    .setFormat(nvrhi::Format::R32_UINT)
                    .setOffset(offsetof(Vertex2DLine, entityID))
                    .setElementStride(sizeof(Vertex2DLine))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            nvrhi::BindingLayoutDesc bindingDesc;
            bindingDesc.setVisibility(nvrhi::ShaderType::All);
            bindingDesc.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            return bindingDesc;
        }
    };

    struct VertexMeshOutline
    {
        glm::vec3 position;
        u32 boneIDs[VERTEX_MAX_BONES] = { 0 };
        f32 weights[VERTEX_MAX_BONES] = { 0.0f };

        static std::array<nvrhi::VertexAttributeDesc, 3> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(VertexMeshOutline, position))
                    .setElementStride(sizeof(VertexMeshOutline)),
                nvrhi::VertexAttributeDesc()
                    .setName("BONEIDS")
                    .setFormat(nvrhi::Format::RGBA32_UINT)
                    .setOffset(offsetof(VertexMeshOutline, boneIDs))
                    .setElementStride(sizeof(VertexMeshOutline)),
                nvrhi::VertexAttributeDesc()
                    .setName("WEIGHTS")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(VertexMeshOutline, weights))
                    .setElementStride(sizeof(VertexMeshOutline))
            };
        }

        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc()
        {
            return nvrhi::BindingLayoutDesc()
                .setVisibility(nvrhi::ShaderType::All)
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0)) // camera
                .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1)); // model
        }
    };
}
