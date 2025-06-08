#include "renderer_2d.hpp"

#include <stb_image.h>

#include "shader_factory.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"
#include "graphics_pipeline.hpp"
#include "ignite/scene/camera.hpp"

#include "texture.hpp"

namespace ignite
{
    nvrhi::ICommandList *Renderer2D::renderCommandList = nullptr;
    nvrhi::IFramebuffer *Renderer2D::renderFramebuffer = nullptr;
    ignite::Renderer2DData *Renderer2D::s_Data = nullptr;

    void Renderer2D::Init()
    {
        s_Data = new Renderer2DData();
    }

    void Renderer2D::Shutdown()
    {
        if (s_Data)
        {
            delete s_Data;
        }
    }

    void Renderer2D::InitQuadData(nvrhi::ICommandList *commandList)
    {
        nvrhi::IDevice* device = Application::GetRenderDevice();

        size_t vertAllocSize = s_Data->quadBatch.maxVertices * sizeof(Vertex2DQuad);
        s_Data->quadBatch.vertexBufferBase = new Vertex2DQuad[vertAllocSize];

        // create buffers
        const auto desc = nvrhi::BufferDesc()
            .setByteSize(vertAllocSize)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Quad Vertex Buffer");

        s_Data->quadBatch.vertexBuffer = device->createBuffer(desc);
        LOG_ASSERT(s_Data->quadBatch.vertexBuffer, "[Renderer 2D] Failed to create Renderer 2D Quad Vertex Buffer");

        const auto ibDesc = nvrhi::BufferDesc()
            .setByteSize(s_Data->quadBatch.maxIndices * sizeof(u32))
            .setIsIndexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Quad Index Buffer");

        s_Data->quadBatch.indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(s_Data->quadBatch.indexBuffer, "[Renderer 2D] Failed to create Renderer 2D Quad Index Buffer");

        // create texture
        s_Data->quadBatch.textureSlots.resize(s_Data->quadBatch.maxTextureCount);
        s_Data->quadBatch.textureSlots[0] = Renderer::GetWhiteTexture();

        // then add textures
        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge)
            .setAllFilters(true);

        s_Data->quadBatch.sampler = device->createSampler(samplerDesc);

        // create binding set
        nvrhi::BindingSetDesc bindingSetDesc;
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, Renderer::GetCameraBufferHandle()));
        bindingSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, s_Data->quadBatch.sampler));
        bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, Renderer::GetWhiteTexture()->GetHandle(), nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, nvrhi::TextureDimension::Texture2D));

        s_Data->quadBindingSet = device->createBindingSet(bindingSetDesc, Renderer::GetBindingLayout(GPipeline::QUAD2D));
        LOG_ASSERT(s_Data->quadBindingSet, "[Renderer 2D] Failed to create binding set");


        // write index buffer
        u32 *indices = new u32[s_Data->quadBatch.maxIndices];

        u32 offset = 0;
        for (u32 i = 0; i < s_Data->quadBatch.maxIndices; i += 6)
        {
            indices[0 + i] = offset + 0;
            indices[1 + i] = offset + 1;
            indices[2 + i] = offset + 2;

            indices[3 + i] = offset + 0;
            indices[4 + i] = offset + 3;
            indices[5 + i] = offset + 1;

            offset += 4;
        }

        commandList->open();
        commandList->writeBuffer(s_Data->quadBatch.indexBuffer, indices, s_Data->quadBatch.maxIndices * sizeof(u32));        
        commandList->close();
        device->executeCommandList(commandList);
        
        delete[] indices;
        
        s_Data->quadPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f }; // bottom-left
        s_Data->quadPositions[1] = { 0.5f,  0.5f, 0.0f, 1.0f }; // top-right
        s_Data->quadPositions[2] = {-0.5f,  0.5f, 0.0f, 1.0f }; // top-left
        s_Data->quadPositions[3] = { 0.5f, -0.5f, 0.0f, 1.0f }; // bottom-right
    }

    void Renderer2D::InitLineData(nvrhi::ICommandList* commandList)
    {
        nvrhi::IDevice* device = Application::GetRenderDevice();

        size_t vertAllocSize = s_Data->lineBatch.maxVertices * sizeof(Vertex2DLine);
        s_Data->lineBatch.vertexBufferBase = new Vertex2DLine[vertAllocSize];

        // create buffers
        const auto vbDesc = nvrhi::BufferDesc()
            .setByteSize(vertAllocSize)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Line Vertex Buffer");

        s_Data->lineBatch.vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(s_Data->lineBatch.vertexBuffer, "[Renderer 2D] Failed to create Renderer 2D Line Vertex Buffer");

        // create binding set
        nvrhi::BindingSetDesc bindingSetDesc;
        // add constant buffer
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, Renderer::GetCameraBufferHandle()));

        s_Data->lineBindingSet = device->createBindingSet(bindingSetDesc, Renderer::GetBindingLayout(GPipeline::LINE));
        LOG_ASSERT(s_Data->lineBindingSet, "[Renderer 2D] Failed to create binding");
    }

    void Renderer2D::Begin(nvrhi::ICommandList* commandList, nvrhi::IFramebuffer* framebuffer)
    {
        // Quad data
        s_Data->quadBatch.indexCount = 0;
        s_Data->quadBatch.count = 0;
        s_Data->quadBatch.vertexBufferPtr = s_Data->quadBatch.vertexBufferBase;

        // Line data
        s_Data->lineBatch.indexCount = 0;
        s_Data->lineBatch.count = 0;
        s_Data->lineBatch.vertexBufferPtr = s_Data->lineBatch.vertexBufferBase;

        renderCommandList = commandList;
        renderFramebuffer = framebuffer;
    }

    void Renderer2D::Flush(Ref<GraphicsPipeline> quadPipeline, Ref<GraphicsPipeline> linePipeline)
    {
        nvrhi::Viewport viewport = renderFramebuffer->getFramebufferInfo().getViewport();

        if (s_Data->quadBatch.indexCount > 0)
        {
            const size_t bufferSize = reinterpret_cast<uint8_t*>(s_Data->quadBatch.vertexBufferPtr) - reinterpret_cast<uint8_t*>(s_Data->quadBatch.vertexBufferBase);
            renderCommandList->writeBuffer(s_Data->quadBatch.vertexBuffer, s_Data->quadBatch.vertexBufferBase, bufferSize);

            const auto graphicsState = nvrhi::GraphicsState()
                .setPipeline(quadPipeline->GetHandle())
                .setFramebuffer(renderFramebuffer)
                .addBindingSet(s_Data->quadBindingSet)
                .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
                .addVertexBuffer(nvrhi::VertexBufferBinding{ s_Data->quadBatch.vertexBuffer, 0, 0 })
                .setIndexBuffer({ s_Data->quadBatch.indexBuffer, nvrhi::Format::R32_UINT });

            renderCommandList->setGraphicsState(graphicsState);
            nvrhi::DrawArguments args;
            args.vertexCount = s_Data->quadBatch.indexCount;
            args.instanceCount = 1;

            renderCommandList->drawIndexed(args);
        }

        if (s_Data->lineBatch.indexCount > 0)
        {
            const size_t bufferSize = reinterpret_cast<uint8_t*>(s_Data->lineBatch.vertexBufferPtr) - reinterpret_cast<uint8_t*>(s_Data->lineBatch.vertexBufferBase);
            renderCommandList->writeBuffer(s_Data->lineBatch.vertexBuffer, s_Data->lineBatch.vertexBufferBase, bufferSize);

            const auto graphicsState = nvrhi::GraphicsState()
                .setPipeline(linePipeline->GetHandle())
                .setFramebuffer(renderFramebuffer)
                .addBindingSet(s_Data->lineBindingSet)
                .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
                .addVertexBuffer(nvrhi::VertexBufferBinding{ s_Data->lineBatch.vertexBuffer, 0, 0 });

            renderCommandList->setGraphicsState(graphicsState);
            nvrhi::DrawArguments args;
            args.vertexCount = s_Data->lineBatch.indexCount;
            args.instanceCount = 1;

            renderCommandList->draw(args);
        }
    }

    void Renderer2D::End()
    {
        s_Data->quadBatch.indexCount = 0;
        s_Data->quadBatch.count = 0;

        s_Data->lineBatch.indexCount = 0;
        s_Data->lineBatch.count = 0;
    }
    
    void Renderer2D::DrawBox(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID)
    {
        if (s_Data->lineBatch.count >= s_Data->lineBatch.maxCount)
            Renderer2D::End();

        static glm::vec4 cubeVertices[8] = {
            {-0.5f, -0.5f, -0.5f, 1.0f}, // 0
            { 0.5f, -0.5f, -0.5f, 1.0f}, // 1
            { 0.5f,  0.5f, -0.5f, 1.0f}, // 2
            {-0.5f,  0.5f, -0.5f, 1.0f}, // 3
            {-0.5f, -0.5f,  0.5f, 1.0f}, // 4
            { 0.5f, -0.5f,  0.5f, 1.0f}, // 5
            { 0.5f,  0.5f,  0.5f, 1.0f}, // 6
            {-0.5f,  0.5f,  0.5f, 1.0f}  // 7
        };

        static int edgeIndices[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
            {0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
        };

        for (int i = 0; i < 12; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                glm::vec4 position = transform * cubeVertices[edgeIndices[i][j]];
                s_Data->lineBatch.vertexBufferPtr->position = position;
                s_Data->lineBatch.vertexBufferPtr->color = color;
                s_Data->lineBatch.vertexBufferPtr->entityID = entityID;
                s_Data->lineBatch.vertexBufferPtr++;
                s_Data->lineBatch.indexCount++;
            }
        }

        s_Data->lineBatch.count++;
    }

    void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID)
    {
        if (s_Data->lineBatch.count >= s_Data->lineBatch.maxCount)
            Renderer2D::End();

        static constexpr int indices[8][2] = {
            {0, 2},
            {2, 1},
            {1, 3},
            {3, 0}
        };

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                glm::vec4 position = transform * s_Data->quadPositions[indices[i][j]];
                s_Data->lineBatch.vertexBufferPtr->position = position;
                s_Data->lineBatch.vertexBufferPtr->color = color;
                s_Data->lineBatch.vertexBufferPtr->entityID = entityID;
                s_Data->lineBatch.vertexBufferPtr++;
                s_Data->lineBatch.indexCount++;
            }
        }

        s_Data->lineBatch.count++;
    }

    void Renderer2D::DrawLine(const std::vector<glm::vec3>& positions, const glm::vec4& color, uint32_t entityID)
    {
        if (s_Data->lineBatch.count >= s_Data->lineBatch.maxCount)
            Renderer2D::End();

        for (auto& pos : positions)
        {
            s_Data->lineBatch.vertexBufferPtr->position = pos;
            s_Data->lineBatch.vertexBufferPtr->color = color;
            s_Data->lineBatch.vertexBufferPtr->entityID = entityID;
            s_Data->lineBatch.vertexBufferPtr++;

            s_Data->lineBatch.indexCount++;
        }

        s_Data->lineBatch.count++;
    }

    void Renderer2D::DrawLine(const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec4& color, uint32_t entityID)
    {
        if (s_Data->lineBatch.count >= s_Data->lineBatch.maxCount)
            Renderer2D::End();

        s_Data->lineBatch.vertexBufferPtr->position = pos0;
        s_Data->lineBatch.vertexBufferPtr->color = color;
        s_Data->lineBatch.vertexBufferPtr->entityID = entityID;
        s_Data->lineBatch.vertexBufferPtr++;

        s_Data->lineBatch.vertexBufferPtr->position = pos1;
        s_Data->lineBatch.vertexBufferPtr->color = color;
        s_Data->lineBatch.vertexBufferPtr->entityID = entityID;
        s_Data->lineBatch.vertexBufferPtr++;

        s_Data->lineBatch.indexCount += 2;
        s_Data->lineBatch.count++;
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, f32 rotation, const glm::vec4 &color, Ref<Texture> texture, const glm::vec2 &tilingFactor, uint32_t entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) 
            * glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f }) 
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
        DrawQuad(transform, color, texture, tilingFactor, entityID);
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color, Ref<Texture> texture, const glm::vec2 &tilingFactor, uint32_t entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
        DrawQuad(transform, color, texture, tilingFactor, entityID);
    }
    
    void Renderer2D::DrawQuad(const glm::mat4 &transform, const glm::vec4 &color, Ref<Texture> texture, const glm::vec2 &tilingFactor, uint32_t entityID)
    {
        if (s_Data->quadBatch.count >= s_Data->quadBatch.maxCount)
            Renderer2D::End();

        static constexpr u32 quadVertexCount = 4;
        static constexpr glm::vec2 textureCoords[] = {
            { 0.0f, 1.0f },
            { 1.0f, 0.0f },
            { 0.0f, 0.0f },
            { 1.0f, 1.0f }
        };

        u32 texIndex = GetOrInsertTexture(texture);

        for (u32 i = 0; i < quadVertexCount; ++i)
        {
            s_Data->quadBatch.vertexBufferPtr->position     = transform * s_Data->quadPositions[i];
            s_Data->quadBatch.vertexBufferPtr->texCoord     = textureCoords[i];
            s_Data->quadBatch.vertexBufferPtr->tilingFactor = tilingFactor;
            s_Data->quadBatch.vertexBufferPtr->color        = color;
            s_Data->quadBatch.vertexBufferPtr->texIndex     = texIndex;
            s_Data->quadBatch.vertexBufferPtr->entityID     = entityID;
            s_Data->quadBatch.vertexBufferPtr++;
        }

        s_Data->quadBatch.indexCount += 6;
        s_Data->quadBatch.count++;
    }

    u32 Renderer2D::GetOrInsertTexture(Ref<Texture> texture)
    {
        if (texture == nullptr)
            return 0;

        u32 textureIndex = 0;

        // find texture
        for (u32 i = 0; i < s_Data->quadBatch.textureSlotIndex; ++i)
        {
            if (*s_Data->quadBatch.textureSlots[i] == *texture)
            {
                textureIndex = i;
                break;
            }
        }

        // insert if not found
        if (textureIndex == 0)
        {
            if (s_Data->quadBatch.textureSlotIndex >= s_Data->quadBatch.maxTextureCount)
            {
                End();
                return s_Data->quadBatch.maxTextureCount;
            }
            
            textureIndex = s_Data->quadBatch.textureSlotIndex;
            s_Data->quadBatch.textureSlots[s_Data->quadBatch.textureSlotIndex] = texture;
            s_Data->quadBatch.textureSlotIndex++;

            // command list already open in editor layer
            texture->Write(renderCommandList);

            UpdateTextureBindings();
        }

        return textureIndex;
    }

    void Renderer2D::UpdateTextureBindings()
    {
        nvrhi::IDevice* device = Application::GetRenderDevice();

        nvrhi::BindingSetDesc bindingSetDesc;
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, Renderer::GetCameraBufferHandle()));
        bindingSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, s_Data->quadBatch.sampler));

        for (u32 i = 0; i < u32(s_Data->quadBatch.maxTextureCount); ++i)
        {
            Ref<Texture> tex = s_Data->quadBatch.textureSlots[i];
            if (!tex)
            {
                tex = Renderer::GetWhiteTexture();
            }
            bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(i, tex->GetHandle()));
        }

        s_Data->quadBindingSet = device->createBindingSet(bindingSetDesc, Renderer::GetBindingLayout(GPipeline::QUAD2D));
    }
}