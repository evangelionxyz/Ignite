#include "renderer_2d.hpp"
#include "renderer.hpp"

#include <stb_image.h>
#include <ignite/scene/camera.hpp>

#include "shader_factory.hpp"
#include "ignite/core/logger.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"

#include "texture.hpp"

namespace ignite
{
    Renderer2DData *s_r2d = nullptr;
    nvrhi::ICommandList *Renderer2D::renderCommandList = nullptr;
    nvrhi::IFramebuffer *Renderer2D::renderFramebuffer = nullptr;

    void Renderer2D::Init()
    {
        s_r2d = new Renderer2DData();

        nvrhi::IDevice* device = Application::GetRenderDevice();

        const auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(PushConstant2D))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(16);
        s_r2d->constantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(s_r2d->constantBuffer, "[Renderer 2D] Failed to create constant buffer");
    }

    void Renderer2D::Shutdown()
    {
        if (s_r2d)
        {
            delete s_r2d;
        }
    }

    void Renderer2D::InitQuadData(nvrhi::ICommandList *commandList)
    {
        nvrhi::IDevice* device = Application::GetRenderDevice();

        GraphicsPipelineParams params;
        params.enableBlend = true;
        params.depthWrite = true;
        params.depthTest = true;
        params.fillMode = nvrhi::RasterFillMode::Solid;
        params.cullMode = nvrhi::RasterCullMode::Front;

        auto attributes = Vertex2DQuad::GetAttributes();
        GraphicsPiplineCreateInfo pci;
        pci.attributes = attributes.data();
        pci.attributeCount = static_cast<uint32_t>(attributes.size());
        pci.bindingLayoutDesc = Vertex2DQuad::GetBindingLayoutDesc();

        s_r2d->quadBatch.pipeline = GraphicsPipeline::Create(params, &pci);

        auto shaderContext = Renderer::GetShaderLibrary().Get("batch_2d_quad");
        s_r2d->quadBatch.pipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
            .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
            .Build();

        size_t vertAllocSize = s_r2d->quadBatch.maxVertices * sizeof(Vertex2DQuad);
        s_r2d->quadBatch.vertexBufferBase = new Vertex2DQuad[vertAllocSize];

        // create buffers
        const auto vbDesc = nvrhi::BufferDesc()
            .setByteSize(vertAllocSize)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Quad Vertex Buffer");

        s_r2d->quadBatch.vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(s_r2d->quadBatch.vertexBuffer, "[Renderer 2D] Failed to create Renderer 2D Quad Vertex Buffer");

        const auto ibDesc = nvrhi::BufferDesc()
            .setByteSize(s_r2d->quadBatch.maxIndices * sizeof(u32))
            .setIsIndexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Quad Index Buffer");

        s_r2d->quadBatch.indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(s_r2d->quadBatch.indexBuffer, "[Renderer 2D] Failed to create Renderer 2D Quad Index Buffer");

        // create texture
        s_r2d->quadBatch.textureSlots.resize(s_r2d->quadBatch.maxTextureCount);
        s_r2d->quadBatch.textureSlots[0] = Renderer::GetWhiteTexture();

        // create binding set
        nvrhi::BindingSetDesc bindingSetDesc;
        // add constant buffer
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, s_r2d->constantBuffer));

        // then add textures
        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge)
            .setAllFilters(true);

        s_r2d->quadBatch.sampler = device->createSampler(samplerDesc);

        bindingSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, s_r2d->quadBatch.sampler));

        for (u32 i = 0; i < u32(s_r2d->quadBatch.maxTextureCount); ++i)
        {
            bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(i, Renderer::GetWhiteTexture()->GetHandle(), nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, nvrhi::TextureDimension::Texture2D));
        }

        s_r2d->quadBatch.bindingSet = device->createBindingSet(bindingSetDesc, s_r2d->quadBatch.pipeline->GetBindingLayout());
        LOG_ASSERT(s_r2d->quadBatch.bindingSet, "[Renderer 2D] Failed to create binding");

        // write index buffer
        u32 *indices = new u32[s_r2d->quadBatch.maxIndices];

        u32 offset = 0;
        for (u32 i = 0; i < s_r2d->quadBatch.maxIndices; i += 6)
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
        commandList->writeBuffer(s_r2d->quadBatch.indexBuffer, indices, s_r2d->quadBatch.maxIndices * sizeof(u32));        
        commandList->close();
        device->executeCommandList(commandList);
        
        delete[] indices;
        
        s_r2d->quadPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f }; // bottom-left
        s_r2d->quadPositions[1] = { 0.5f,  0.5f, 0.0f, 1.0f }; // top-right
        s_r2d->quadPositions[2] = {-0.5f,  0.5f, 0.0f, 1.0f }; // top-left
        s_r2d->quadPositions[3] = { 0.5f, -0.5f, 0.0f, 1.0f }; // bottom-right
    }

    void Renderer2D::InitLineData(nvrhi::ICommandList* commandList)
    {
        nvrhi::IDevice* device = Application::GetRenderDevice();

        GraphicsPipelineParams params;
        params.enableBlend = false;
        params.depthWrite = true;
        params.depthTest = true;
        params.fillMode = nvrhi::RasterFillMode::Wireframe;
        params.cullMode = nvrhi::RasterCullMode::None;
        params.primitiveType = nvrhi::PrimitiveType::LineList;

        auto attributes = Vertex2DLine::GetAttributes();
        GraphicsPiplineCreateInfo pci;
        pci.attributes = attributes.data();
        pci.attributeCount = static_cast<uint32_t>(attributes.size());
        pci.bindingLayoutDesc = Vertex2DLine::GetBindingLayoutDesc();

        s_r2d->lineBatch.pipeline = GraphicsPipeline::Create(params, &pci);

        auto shaderContext = Renderer::GetShaderLibrary().Get("batch_2d_line");
        
        s_r2d->lineBatch.pipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
            .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
            .Build();

        size_t vertAllocSize = s_r2d->lineBatch.maxVertices * sizeof(Vertex2DLine);
        s_r2d->lineBatch.vertexBufferBase = new Vertex2DLine[vertAllocSize];

        // create buffers
        const auto vbDesc = nvrhi::BufferDesc()
            .setByteSize(vertAllocSize)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Line Vertex Buffer");

        s_r2d->lineBatch.vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(s_r2d->lineBatch.vertexBuffer, "[Renderer 2D] Failed to create Renderer 2D Line Vertex Buffer");

        // create binding set
        nvrhi::BindingSetDesc bindingSetDesc;
        // add constant buffer
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, s_r2d->constantBuffer));

        s_r2d->lineBatch.bindingSet = device->createBindingSet(bindingSetDesc, s_r2d->lineBatch.pipeline->GetBindingLayout());
        LOG_ASSERT(s_r2d->lineBatch.bindingSet, "[Renderer 2D] Failed to create binding");
    }

    void Renderer2D::Begin(Camera *camera, nvrhi::ICommandList* commandList, nvrhi::IFramebuffer* framebuffer)
    {
        // create with the same framebuffer to render
        CreateGraphicsPipeline(framebuffer);

        // Quad data
        s_r2d->quadBatch.indexCount = 0;
        s_r2d->quadBatch.count = 0;
        s_r2d->quadBatch.vertexBufferPtr = s_r2d->quadBatch.vertexBufferBase;

        // Line data
        s_r2d->lineBatch.indexCount = 0;
        s_r2d->lineBatch.count = 0;
        s_r2d->lineBatch.vertexBufferPtr = s_r2d->lineBatch.vertexBufferBase;

        PushConstant2D constants { camera->GetViewProjectionMatrix() };
        commandList->writeBuffer(s_r2d->constantBuffer, &constants, sizeof(constants));

        renderCommandList = commandList;
        renderFramebuffer = framebuffer;
    }

    void Renderer2D::Flush()
    {
        nvrhi::Viewport viewport = renderFramebuffer->getFramebufferInfo().getViewport();

        if (s_r2d->quadBatch.indexCount > 0)
        {
            const size_t bufferSize = reinterpret_cast<uint8_t*>(s_r2d->quadBatch.vertexBufferPtr) - reinterpret_cast<uint8_t*>(s_r2d->quadBatch.vertexBufferBase);
            renderCommandList->writeBuffer(s_r2d->quadBatch.vertexBuffer, s_r2d->quadBatch.vertexBufferBase, bufferSize);

            const auto graphicsState = nvrhi::GraphicsState()
                .setPipeline(s_r2d->quadBatch.pipeline->GetHandle())
                .setFramebuffer(renderFramebuffer)
                .addBindingSet(s_r2d->quadBatch.bindingSet)
                .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
                .addVertexBuffer(nvrhi::VertexBufferBinding{ s_r2d->quadBatch.vertexBuffer, 0, 0 })
                .setIndexBuffer({ s_r2d->quadBatch.indexBuffer, nvrhi::Format::R32_UINT });

            renderCommandList->setGraphicsState(graphicsState);
            nvrhi::DrawArguments args;
            args.vertexCount = s_r2d->quadBatch.indexCount;
            args.instanceCount = 1;

            renderCommandList->drawIndexed(args);
        }

        if (s_r2d->lineBatch.indexCount > 0)
        {
            const size_t bufferSize = reinterpret_cast<uint8_t*>(s_r2d->lineBatch.vertexBufferPtr) - reinterpret_cast<uint8_t*>(s_r2d->lineBatch.vertexBufferBase);
            renderCommandList->writeBuffer(s_r2d->lineBatch.vertexBuffer, s_r2d->lineBatch.vertexBufferBase, bufferSize);

            const auto graphicsState = nvrhi::GraphicsState()
                .setPipeline(s_r2d->lineBatch.pipeline->GetHandle())
                .setFramebuffer(renderFramebuffer)
                .addBindingSet(s_r2d->lineBatch.bindingSet)
                .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
                .addVertexBuffer(nvrhi::VertexBufferBinding{ s_r2d->lineBatch.vertexBuffer, 0, 0 });

            renderCommandList->setGraphicsState(graphicsState);
            nvrhi::DrawArguments args;
            args.vertexCount = s_r2d->lineBatch.indexCount;
            args.instanceCount = 1;

            renderCommandList->draw(args);
        }
    }

    void Renderer2D::End()
    {
    }
    
    void Renderer2D::CreateGraphicsPipeline(nvrhi::IFramebuffer *framebuffer)
    {
        s_r2d->quadBatch.pipeline->CreatePipeline(framebuffer);
        s_r2d->lineBatch.pipeline->CreatePipeline(framebuffer);
    }

    void Renderer2D::DrawBox(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID)
    {
        if (s_r2d->lineBatch.count >= s_r2d->lineBatch.maxCount)
            Renderer2D::Flush();

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
                s_r2d->lineBatch.vertexBufferPtr->position = position;
                s_r2d->lineBatch.vertexBufferPtr->color = color;
                s_r2d->lineBatch.vertexBufferPtr->entityID = entityID;
                s_r2d->lineBatch.vertexBufferPtr++;
                s_r2d->lineBatch.indexCount++;
            }
        }

        s_r2d->lineBatch.count++;
    }

    void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID)
    {
        if (s_r2d->lineBatch.count >= s_r2d->lineBatch.maxCount)
            Renderer2D::Flush();

        static constexpr int indices[8][2] = {
            {0, 1},
            {1, 2},
            {2, 3},
            {3, 0}
        };

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                glm::vec4 position = transform * s_r2d->quadPositions[indices[i][j]];
                s_r2d->lineBatch.vertexBufferPtr->position = position;
                s_r2d->lineBatch.vertexBufferPtr->color = color;
                s_r2d->lineBatch.vertexBufferPtr->entityID = entityID;
                s_r2d->lineBatch.vertexBufferPtr++;
                s_r2d->lineBatch.indexCount++;
            }
        }

        s_r2d->lineBatch.count++;
    }

    void Renderer2D::DrawLine(const std::vector<glm::vec3>& positions, const glm::vec4& color, uint32_t entityID)
    {
        if (s_r2d->lineBatch.count >= s_r2d->lineBatch.maxCount)
            Renderer2D::Flush();

        for (auto& pos : positions)
        {
            s_r2d->lineBatch.vertexBufferPtr->position = pos;
            s_r2d->lineBatch.vertexBufferPtr->color = color;
            s_r2d->lineBatch.vertexBufferPtr->entityID = entityID;
            s_r2d->lineBatch.vertexBufferPtr++;

            s_r2d->lineBatch.indexCount++;
        }

        s_r2d->lineBatch.count++;
    }

    void Renderer2D::DrawLine(const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec4& color, uint32_t entityID)
    {
        if (s_r2d->lineBatch.count >= s_r2d->lineBatch.maxCount)
            Renderer2D::Flush();

        s_r2d->lineBatch.vertexBufferPtr->position = pos0;
        s_r2d->lineBatch.vertexBufferPtr->color = color;
        s_r2d->lineBatch.vertexBufferPtr->entityID = entityID;
        s_r2d->lineBatch.vertexBufferPtr++;

        s_r2d->lineBatch.vertexBufferPtr->position = pos1;
        s_r2d->lineBatch.vertexBufferPtr->color = color;
        s_r2d->lineBatch.vertexBufferPtr->entityID = entityID;
        s_r2d->lineBatch.vertexBufferPtr++;

        s_r2d->lineBatch.indexCount += 2;
        s_r2d->lineBatch.count++;
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
        if (s_r2d->quadBatch.count >= s_r2d->quadBatch.maxCount)
            Renderer2D::Flush();

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
            s_r2d->quadBatch.vertexBufferPtr->position     = transform * s_r2d->quadPositions[i];
            s_r2d->quadBatch.vertexBufferPtr->texCoord     = textureCoords[i];
            s_r2d->quadBatch.vertexBufferPtr->tilingFactor = tilingFactor;
            s_r2d->quadBatch.vertexBufferPtr->color        = color;
            s_r2d->quadBatch.vertexBufferPtr->texIndex     = texIndex;
            s_r2d->quadBatch.vertexBufferPtr->entityID     = entityID;
            s_r2d->quadBatch.vertexBufferPtr++;
        }

        s_r2d->quadBatch.indexCount += 6;
        s_r2d->quadBatch.count++;
    }

    u32 Renderer2D::GetOrInsertTexture(Ref<Texture> texture)
    {
        if (texture == nullptr)
            return 0;

        u32 textureIndex = 0;

        // find texture
        for (u32 i = 0; i < s_r2d->quadBatch.textureSlotIndex; ++i)
        {
            if (*s_r2d->quadBatch.textureSlots[i] == *texture)
            {
                textureIndex = i;
                break;
            }
        }

        // insert if not found
        if (textureIndex == 0)
        {
            if (s_r2d->quadBatch.textureSlotIndex >= s_r2d->quadBatch.maxTextureCount)
            {
                Flush();
                return s_r2d->quadBatch.maxTextureCount;
            }
            
            textureIndex = s_r2d->quadBatch.textureSlotIndex;
            s_r2d->quadBatch.textureSlots[s_r2d->quadBatch.textureSlotIndex] = texture;
            s_r2d->quadBatch.textureSlotIndex++;

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
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, s_r2d->constantBuffer));
        bindingSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, s_r2d->quadBatch.sampler));

        for (u32 i = 0; i < u32(s_r2d->quadBatch.maxTextureCount); ++i)
        {
            Ref<Texture> tex = s_r2d->quadBatch.textureSlots[i];
            if (!tex)
            {
                tex = Renderer::GetWhiteTexture();
            }
            bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(i, tex->GetHandle()));
        }

        s_r2d->quadBatch.bindingSet = device->createBindingSet(bindingSetDesc, s_r2d->quadBatch.pipeline->GetBindingLayout());
    }
}