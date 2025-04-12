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
    Renderer2DData s_Data;

    void Renderer2D::Init(DeviceManager *deviceManager)
    {
        s_Data.deviceManager = deviceManager;
        nvrhi::IDevice *device = s_Data.deviceManager->GetDevice();

        InitConstantBuffer(device);
    }

    void Renderer2D::Shutdown()
    {
        s_Data.quadBatch.Destroy();
    }

    void Renderer2D::InitConstantBuffer(nvrhi::IDevice *device)
    {
        const auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(PushConstant2D))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(16);
        s_Data.constantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(s_Data.constantBuffer, "Failed to create constant buffer");
    }

    void Renderer2D::InitQuadData(nvrhi::IDevice *device, nvrhi::ICommandList *commandList)
    {
        s_Data.commandList = commandList;

        size_t vertAllocSize = s_Data.quadBatch.maxVertices * sizeof(VertexQuad);
        s_Data.quadBatch.vertexBufferBase = new VertexQuad[vertAllocSize];

        // create shaders
        s_Data.quadBatch.vertexShader = Application::GetShaderFactory()->CreateShader("default_2d_vertex", "main", nullptr, nvrhi::ShaderType::Vertex);
        s_Data.quadBatch.pixelShader = Application::GetShaderFactory()->CreateShader("default_2d_pixel", "main", nullptr, nvrhi::ShaderType::Pixel);
        LOG_ASSERT(s_Data.quadBatch.vertexShader && s_Data.quadBatch.pixelShader, "Failed to create shaders");

        const auto attributes = VertexQuad::GetAttributes();
        s_Data.quadBatch.inputLayout = device->createInputLayout(attributes.data(), attributes.size(), s_Data.quadBatch.vertexShader);
        LOG_ASSERT(s_Data.quadBatch.inputLayout, "Failed to create input layout");

        const auto layoutDesc = VertexQuad::GetBindingLayoutDesc();
        s_Data.quadBatch.bindingLayout = device->createBindingLayout(layoutDesc);
        LOG_ASSERT(s_Data.quadBatch.bindingLayout, "Failed to create input layout");

        // create buffers
        const auto vbDesc = nvrhi::BufferDesc()
            .setByteSize(vertAllocSize)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Quad Vertex Buffer");

        s_Data.quadBatch.vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(s_Data.quadBatch.vertexBuffer, "Failed to create Renderer 2D Quad Vertex Buffer");

        const auto ibDesc = nvrhi::BufferDesc()
            .setByteSize(s_Data.quadBatch.maxIndices * sizeof(u32))
            .setIsIndexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Renderer 2D Quad Index Buffer");

        s_Data.quadBatch.indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(s_Data.quadBatch.indexBuffer, "Failed to create Renderer 2D Quad Index Buffer");

        // create texture
        s_Data.quadBatch.textureSlots.resize(s_Data.quadBatch.maxTextureCount);
        s_Data.quadBatch.textureSlots[0] = Renderer::whiteTexture;

        // create binding set
        nvrhi::BindingSetDesc bindingSetDesc;
        // add constant buffer
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, s_Data.constantBuffer));

        // then add textures
        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);

        s_Data.quadBatch.sampler = device->createSampler(samplerDesc);
        bindingSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, s_Data.quadBatch.sampler));

        for (u32 i = 0; i < s_Data.quadBatch.maxTextureCount; ++i)
        {
            bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(i, Renderer::whiteTexture->GetHandle()));
        }

        s_Data.quadBatch.bindingSet = device->createBindingSet(bindingSetDesc, s_Data.quadBatch.bindingLayout);
        LOG_ASSERT(s_Data.quadBatch.bindingSet, "Failed to create binding");

        // create graphics pipline
        nvrhi::BlendState blendState;
        blendState.targets[0].setBlendEnable(true);

        auto depthStencilState = nvrhi::DepthStencilState()
            .setDepthWriteEnable(false)
            .setDepthTestEnable(false);

        auto rasterState = nvrhi::RasterState()
            .setCullNone()
            .setFillSolid()
            .setMultisampleEnable(false);

        auto renderState = nvrhi::RenderState()
            .setRasterState(rasterState)
            .setDepthStencilState(depthStencilState)
            .setBlendState(blendState);

        auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
            .setInputLayout(s_Data.quadBatch.inputLayout)
            .setVertexShader(s_Data.quadBatch.vertexShader)
            .setPixelShader(s_Data.quadBatch.pixelShader)
            .addBindingLayout(s_Data.quadBatch.bindingLayout)
            .setRenderState(renderState)
            .setPrimType(nvrhi::PrimitiveType::TriangleList);

        auto framebuffer = s_Data.deviceManager->GetCurrentFramebuffer();
        s_Data.quadBatch.pipeline = device->createGraphicsPipeline(pipelineDesc, framebuffer);
        LOG_ASSERT(s_Data.quadBatch.pipeline, "Failed to create graphics pipeline");


        // write index buffer
        u32 *indices = new u32[s_Data.quadBatch.maxIndices];

        u32 offset = 0;
        for (u32 i = 0; i < s_Data.quadBatch.maxIndices; i += 6)
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
        commandList->writeBuffer(s_Data.quadBatch.indexBuffer, indices, s_Data.quadBatch.maxIndices * sizeof(u32));        
        commandList->close();
        device->executeCommandList(commandList);
        
        delete[] indices;
    }

    void Renderer2D::Begin(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        s_Data.framebuffer = framebuffer;

        s_Data.quadBatch.indexCount = 0;
        s_Data.quadBatch.count = 0;
        s_Data.quadBatch.vertexBufferPtr = s_Data.quadBatch.vertexBufferBase;

        PushConstant2D constants { camera->GetViewProjectionMatrix() };
        s_Data.commandList->writeBuffer(s_Data.constantBuffer, &constants, sizeof(constants));
    }

    void Renderer2D::Flush()
    {
        nvrhi::Viewport viewport = s_Data.framebuffer->getFramebufferInfo().getViewport();

        if (s_Data.quadBatch.indexCount > 0)
        {
            const size_t bufferSize = reinterpret_cast<uint8_t*>(s_Data.quadBatch.vertexBufferPtr) - reinterpret_cast<uint8_t*>(s_Data.quadBatch.vertexBufferBase);
            s_Data.commandList->writeBuffer(s_Data.quadBatch.vertexBuffer, s_Data.quadBatch.vertexBufferBase, bufferSize);

            const auto quadGraphicsState = nvrhi::GraphicsState()
                .setPipeline(s_Data.quadBatch.pipeline)
                .setFramebuffer(s_Data.framebuffer)
                .addBindingSet(s_Data.quadBatch.bindingSet)
                .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
                .addVertexBuffer(nvrhi::VertexBufferBinding{s_Data.quadBatch.vertexBuffer, 0, 0})
                .setIndexBuffer({s_Data.quadBatch.indexBuffer, nvrhi::Format::R32_UINT});

            s_Data.commandList->setGraphicsState(quadGraphicsState);
            nvrhi::DrawArguments args;
            args.vertexCount = s_Data.quadBatch.indexCount;
            args.instanceCount = 1;

            s_Data.commandList->drawIndexed(args);
        }
    }

    void Renderer2D::End()
    {
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color, Ref<Texture> texture)
    {
        if (s_Data.quadBatch.count >= s_Data.quadBatch.maxCount)
            Renderer2D::Flush();

        static constexpr u32 quadVertexCount = 4;
        static constexpr glm::vec2 textureCoords[] = {
            { 0.0f, 1.0f },
            { 1.0f, 0.0f },
            { 0.0f, 0.0f },
            { 1.0f, 1.0f }
        };

        glm::vec2 quadPositions[4];
        quadPositions[0] = position + glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 1.0f); // bottom-left
        quadPositions[1] = position + glm::vec3( size.x * 0.5f,  size.y * 0.5f, 1.0f); // top-right
        quadPositions[2] = position + glm::vec3(-size.x * 0.5f,  size.y * 0.5f, 1.0f); // top-left
        quadPositions[3] = position + glm::vec3( size.x * 0.5f, -size.y * 0.5f, 1.0f); // bottom-right

        u32 texIndex = GetOrInsertTexture(texture);

        for (u32 i = 0; i < quadVertexCount; ++i)
        {
            s_Data.quadBatch.vertexBufferPtr->position = quadPositions[i];
            s_Data.quadBatch.vertexBufferPtr->texCoord = textureCoords[i];
            s_Data.quadBatch.vertexBufferPtr->color = color;
            s_Data.quadBatch.vertexBufferPtr->texIndex = texIndex;
            s_Data.quadBatch.vertexBufferPtr++;
        }

        s_Data.quadBatch.indexCount += 6;
        s_Data.quadBatch.count++;
    }

    u32 Renderer2D::GetOrInsertTexture(Ref<Texture> texture)
    {
        if (texture == nullptr)
            return 0;

        u32 textureIndex = 0;

        // find texture
        for (u32 i = 0; i < s_Data.quadBatch.textureSlotIndex; ++i)
        {
            if (*s_Data.quadBatch.textureSlots[i] == *texture)
            {
                textureIndex = i;
                break;
            }
        }

        // insert if not found
        if (textureIndex == 0)
        {
            if (s_Data.quadBatch.textureSlotIndex >= s_Data.quadBatch.maxTextureCount)
            {
                Flush();
                return s_Data.quadBatch.maxTextureCount;
            }
            
            textureIndex = s_Data.quadBatch.textureSlotIndex;
            s_Data.quadBatch.textureSlots[s_Data.quadBatch.textureSlotIndex] = texture;
            s_Data.quadBatch.textureSlotIndex++;

            UpdateTextureBindings();
        }

        return textureIndex;
    }

    void Renderer2D::UpdateTextureBindings()
    {
        nvrhi::BindingSetDesc bindingSetDesc;
        bindingSetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, s_Data.constantBuffer));
        bindingSetDesc.addItem(nvrhi::BindingSetItem::Sampler(0, s_Data.quadBatch.sampler));

        for (u32 i = 0; i < s_Data.quadBatch.maxTextureCount; ++i)
        {
            Ref<Texture> tex = s_Data.quadBatch.textureSlots[i];
            if (!tex)
            {
                tex = Renderer::whiteTexture;
            }
            bindingSetDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(i, tex->GetHandle()));
        }

        s_Data.quadBatch.bindingSet = s_Data.deviceManager->GetDevice()->createBindingSet(bindingSetDesc, s_Data.quadBatch.bindingLayout);
    }

    nvrhi::ICommandList *Renderer2D::GetCommandList()
    {
        return s_Data.commandList;
    }
}