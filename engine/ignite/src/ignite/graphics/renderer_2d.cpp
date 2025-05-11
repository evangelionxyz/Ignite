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

    void Renderer2D::Init(nvrhi::IDevice *device)
    {
        s_r2d = new Renderer2DData();

        s_r2d->device = device;
        InitConstantBuffer(s_r2d->device);
    }

    void Renderer2D::Shutdown()
    {
        if (s_r2d)
        {
            delete s_r2d;
        }
    }

    void Renderer2D::InitConstantBuffer(nvrhi::IDevice *device)
    {
        const auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(PushConstant2D))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(16);
        s_r2d->constantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(s_r2d->constantBuffer, "[Renderer 2D] Failed to create constant buffer");
    }

    void Renderer2D::InitQuadData(nvrhi::IDevice *device, nvrhi::ICommandList *commandList)
    {
        s_r2d->commandList = commandList;

        size_t vertAllocSize = s_r2d->quadBatch.maxVertices * sizeof(VertexQuad);
        s_r2d->quadBatch.vertexBufferBase = new VertexQuad[vertAllocSize];

        VPShader *vpShader = Renderer::GetDefaultShader("quadBatch2D");
        s_r2d->quadBatch.vertexShader = vpShader->vertex;
        s_r2d->quadBatch.pixelShader = vpShader->pixel;

        const auto attributes = VertexQuad::GetAttributes();
        s_r2d->quadBatch.inputLayout = device->createInputLayout(attributes.data(), u32(attributes.size()), s_r2d->quadBatch.vertexShader);
        LOG_ASSERT(s_r2d->quadBatch.inputLayout, "[Renderer 2D] Failed to create input layout");

        const auto layoutDesc = VertexQuad::GetBindingLayoutDesc();
        s_r2d->quadBatch.bindingLayout = device->createBindingLayout(layoutDesc);
        LOG_ASSERT(s_r2d->quadBatch.bindingLayout, "[Renderer 2D] Failed to create binding layout");

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

        s_r2d->quadBatch.bindingSet = device->createBindingSet(bindingSetDesc, s_r2d->quadBatch.bindingLayout);
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

    void Renderer2D::Begin(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        // create with the same framebuffer to render
        CreateGraphicsPipeline(s_r2d->device, framebuffer);

        s_r2d->framebuffer = framebuffer;

        s_r2d->quadBatch.indexCount = 0;
        s_r2d->quadBatch.count = 0;
        s_r2d->quadBatch.vertexBufferPtr = s_r2d->quadBatch.vertexBufferBase;

        PushConstant2D constants { camera->GetViewProjectionMatrix() };
        s_r2d->commandList->writeBuffer(s_r2d->constantBuffer, &constants, sizeof(constants));
    }

    void Renderer2D::Flush()
    {
        nvrhi::Viewport viewport = s_r2d->framebuffer->getFramebufferInfo().getViewport();

        if (s_r2d->quadBatch.indexCount > 0)
        {
            const size_t bufferSize = reinterpret_cast<uint8_t*>(s_r2d->quadBatch.vertexBufferPtr) - reinterpret_cast<uint8_t*>(s_r2d->quadBatch.vertexBufferBase);
            s_r2d->commandList->writeBuffer(s_r2d->quadBatch.vertexBuffer, s_r2d->quadBatch.vertexBufferBase, bufferSize);

            const auto quadGraphicsState = nvrhi::GraphicsState()
                .setPipeline(s_r2d->quadBatch.pipeline)
                .setFramebuffer(s_r2d->framebuffer)
                .addBindingSet(s_r2d->quadBatch.bindingSet)
                .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
                .addVertexBuffer(nvrhi::VertexBufferBinding{s_r2d->quadBatch.vertexBuffer, 0, 0})
                .setIndexBuffer({s_r2d->quadBatch.indexBuffer, nvrhi::Format::R32_UINT});

            s_r2d->commandList->setGraphicsState(quadGraphicsState);
            nvrhi::DrawArguments args;
            args.vertexCount = s_r2d->quadBatch.indexCount;
            args.instanceCount = 1;

            s_r2d->commandList->drawIndexed(args);
        }
    }

    void Renderer2D::End()
    {
    }
    
    void Renderer2D::CreateGraphicsPipeline(nvrhi::IDevice *device, nvrhi::IFramebuffer *framebuffer)
    {
        if (s_r2d->quadBatch.pipeline == nullptr)
        {
            // create graphics pipeline
            nvrhi::BlendState blendState;
            blendState.targets[0].setBlendEnable(true);

            auto depthStencilState = nvrhi::DepthStencilState()
                .setDepthWriteEnable(true)
                .setDepthTestEnable(true)
                .setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual); // use 1.0 for far depth

            auto rasterState = nvrhi::RasterState()
                .setCullNone();

            auto renderState = nvrhi::RenderState()
                .setRasterState(rasterState)
                .setDepthStencilState(depthStencilState)
                .setBlendState(blendState);

            auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
                .setInputLayout(s_r2d->quadBatch.inputLayout)
                .setVertexShader(s_r2d->quadBatch.vertexShader)
                .setPixelShader(s_r2d->quadBatch.pixelShader)
                .addBindingLayout(s_r2d->quadBatch.bindingLayout)
                .setRenderState(renderState)
                .setPrimType(nvrhi::PrimitiveType::TriangleList);

            s_r2d->quadBatch.pipeline = device->createGraphicsPipeline(pipelineDesc, framebuffer);
            LOG_ASSERT(s_r2d->quadBatch.pipeline, "[Renderer 2D] Failed to create graphics pipeline");
        }
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, f32 rotation, const glm::vec4 &color, Ref<Texture> texture, const glm::vec2 &tilingFactor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) 
            * glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f }) 
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
        DrawQuad(transform, color, texture, tilingFactor);
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color, Ref<Texture> texture, const glm::vec2 &tilingFactor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
        DrawQuad(transform, color, texture, tilingFactor);
    }
    
    void Renderer2D::DrawQuad(const glm::mat4 &transform, const glm::vec4 &color, Ref<Texture> texture, const glm::vec2 &tilingFactor)
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

            // command list already open in editor layer
            texture->Write(s_r2d->commandList);
            
            textureIndex = s_r2d->quadBatch.textureSlotIndex;
            s_r2d->quadBatch.textureSlots[s_r2d->quadBatch.textureSlotIndex] = texture;
            s_r2d->quadBatch.textureSlotIndex++;

            UpdateTextureBindings();
        }

        return textureIndex;
    }

    void Renderer2D::UpdateTextureBindings()
    {
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

        s_r2d->quadBatch.bindingSet = s_r2d->device->createBindingSet(bindingSetDesc, s_r2d->quadBatch.bindingLayout);
    }

    nvrhi::ICommandList *Renderer2D::GetCommandList()
    {
        return s_r2d->commandList;
    }
}