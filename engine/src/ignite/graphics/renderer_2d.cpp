#include "renderer_2d.hpp"

#include <stb_image.h>
#include <ignite/scene/camera.hpp>

#include "shader_factory.hpp"
#include "ignite/core/logger.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"

Renderer2DData s_R2DData;

void Renderer2D::Init(DeviceManager *deviceManager)
{
    s_R2DData.deviceManager = deviceManager;
    nvrhi::IDevice *device = s_R2DData.deviceManager->GetDevice();

    InitConstantBuffer(device);
}

void Renderer2D::Shutdown()
{
    delete[] s_R2DData.quadVertexBufferBase;
}

void Renderer2D::InitConstantBuffer(nvrhi::IDevice *device)
{
    const auto constantBufferDesc = nvrhi::BufferDesc()
        .setByteSize(sizeof(PushConstant2D))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(16);
    s_R2DData.constantBuffer = device->createBuffer(constantBufferDesc);
    LOG_ASSERT(s_R2DData.constantBuffer, "Failed to create constant buffer");
}

void Renderer2D::InitQuadData(nvrhi::IDevice *device, nvrhi::ICommandList *commandList)
{
    s_R2DData.commandList = commandList;

    size_t vertAllocSize = s_R2DData.maxQuadVerts * sizeof(VertexQuad);
    s_R2DData.quadVertexBufferBase = new VertexQuad[vertAllocSize];

    // create shaders
    s_R2DData.quadVertexShader = Application::GetShaderFactory()->CreateShader("default_2d_vertex", "main", nullptr, nvrhi::ShaderType::Vertex);
    s_R2DData.quadPixelShader = Application::GetShaderFactory()->CreateShader("default_2d_pixel", "main", nullptr, nvrhi::ShaderType::Pixel);
    LOG_ASSERT(s_R2DData.quadVertexShader && s_R2DData.quadPixelShader, "Failed to create shaders");

    const auto attributes = VertexQuad::GetAttributes();
    s_R2DData.quadInputLayout = device->createInputLayout(attributes.data(), attributes.size(), s_R2DData.quadVertexShader);
    LOG_ASSERT(s_R2DData.quadInputLayout, "Failed to create input layout");

    const auto layoutDesc = VertexQuad::GetBindingLayoutDesc();
    s_R2DData.quadBindingLayout = device->createBindingLayout(layoutDesc);
    LOG_ASSERT(s_R2DData.quadBindingLayout, "Failed to create input layout");

    // create buffers
    const auto vbDesc = nvrhi::BufferDesc()
        .setByteSize(vertAllocSize)
        .setIsVertexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::VertexBuffer)
        .setKeepInitialState(true)
        .setDebugName("Renderer 2D Quad Vertex Buffer");

    s_R2DData.quadVertexBuffer = device->createBuffer(vbDesc);
    LOG_ASSERT(s_R2DData.quadVertexBuffer, "Failed to create Renderer 2D Quad Vertex Buffer");

    const auto ibDesc = nvrhi::BufferDesc()
        .setByteSize(s_R2DData.maxQuadIndices * sizeof(u32))
        .setIsIndexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::IndexBuffer)
        .setKeepInitialState(true)
        .setDebugName("Renderer 2D Quad Index Buffer");

    s_R2DData.quadIndexBuffer = device->createBuffer(ibDesc);
    LOG_ASSERT(s_R2DData.quadIndexBuffer, "Failed to create Renderer 2D Quad Index Buffer");

    // FIXME: Temporary texture creation
    i32 textureWidth, textureHeight, textureNumChannels;
    stbi_uc *pixelData = stbi_load("Resources/textures/test.png", &textureWidth, &textureHeight, &textureNumChannels, 4);
    LOG_ASSERT(pixelData, "Failed to write pixel data to texture");

    const auto textureDesc = nvrhi::TextureDesc()
        .setDimension(nvrhi::TextureDimension::Texture2D)
        .setWidth(textureWidth)
        .setHeight(textureHeight)
        .setFormat(nvrhi::Format::RGBA8_UNORM)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setDebugName("Geometry Texture");

    s_R2DData.quadTexture = device->createTexture(textureDesc);
    LOG_ASSERT(s_R2DData.quadTexture, "Failed to create texture");

    // create sampler
    const auto samplerDesc = nvrhi::SamplerDesc()
        .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
        .setAllFilters(true);

    s_R2DData.quadTextureSampler = device->createSampler(samplerDesc);
    LOG_ASSERT(s_R2DData.quadTextureSampler, "Failed to create texture");

    // create binding set
    auto bindingSetDesc = nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, s_R2DData.quadTexture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, s_R2DData.quadTextureSampler))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, s_R2DData.constantBuffer));

    s_R2DData.quadBindingSet = device->createBindingSet(bindingSetDesc, s_R2DData.quadBindingLayout);
    LOG_ASSERT(s_R2DData.quadBindingSet, "Failed to create binding");

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
        .setInputLayout(s_R2DData.quadInputLayout)
        .setVertexShader(s_R2DData.quadVertexShader)
        .setPixelShader(s_R2DData.quadPixelShader)
        .addBindingLayout(s_R2DData.quadBindingLayout)
        .setRenderState(renderState)
        .setPrimType(nvrhi::PrimitiveType::TriangleList);

    auto framebuffer = s_R2DData.deviceManager->GetCurrentFramebuffer();
    s_R2DData.quadPipeline = device->createGraphicsPipeline(pipelineDesc, framebuffer);
    LOG_ASSERT(s_R2DData.quadPipeline, "Failed to create graphics pipeline");

    commandList->open();

    // write index buffer
    u32 *indices = new u32[s_R2DData.maxQuadIndices];

    u32 offset = 0;
    for (u32 i = 0; i < s_R2DData.maxQuadIndices; i += 6)
    {
        indices[0 + i] = offset + 0;
        indices[1 + i] = offset + 1;
        indices[2 + i] = offset + 2;

        indices[3 + i] = offset + 0;
        indices[4 + i] = offset + 3;
        indices[5 + i] = offset + 1;

        offset += 4;
    }

    commandList->writeBuffer(s_R2DData.quadIndexBuffer, indices, s_R2DData.maxQuadIndices * sizeof(u32));
    delete[] indices;

    // write texture buffer
    size_t rowPitchSize = textureWidth * textureNumChannels;
    commandList->writeTexture(s_R2DData.quadTexture, 0, 0, pixelData, rowPitchSize);
    LOG_ASSERT(commandList, "Failed to write texture to command list");
    commandList->close();
    device->executeCommandList(commandList);
}

void Renderer2D::Begin(Camera *camera)
{
    s_R2DData.quadIndexCount = 0;
    s_R2DData.quadCount = 0;
    s_R2DData.quadVertexBufferPtr = s_R2DData.quadVertexBufferBase;

    PushConstant2D constants { camera->GetViewProjectionMatrix() };
    s_R2DData.commandList->writeBuffer(s_R2DData.constantBuffer, &constants, sizeof(constants));
}

void Renderer2D::Flush(nvrhi::IFramebuffer *framebuffer)
{
    nvrhi::Viewport viewport = framebuffer->getFramebufferInfo().getViewport();

    if (s_R2DData.quadIndexCount > 0)
    {
        const size_t bufferSize = reinterpret_cast<uint8_t*>(s_R2DData.quadVertexBufferPtr) - reinterpret_cast<uint8_t*>(s_R2DData.quadVertexBufferBase);
        s_R2DData.commandList->writeBuffer(s_R2DData.quadVertexBuffer, s_R2DData.quadVertexBufferBase, bufferSize);

        const auto quadGraphicsState = nvrhi::GraphicsState()
            .setPipeline(s_R2DData.quadPipeline)
            .setFramebuffer(framebuffer)
            .addBindingSet(s_R2DData.quadBindingSet)
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
            .addVertexBuffer(nvrhi::VertexBufferBinding{s_R2DData.quadVertexBuffer, 0, 0})
            .setIndexBuffer({s_R2DData.quadIndexBuffer, nvrhi::Format::R32_UINT});

        s_R2DData.commandList->setGraphicsState(quadGraphicsState);
        nvrhi::DrawArguments args;
        args.vertexCount = s_R2DData.quadIndexCount;
        args.instanceCount = 1;

        s_R2DData.commandList->drawIndexed(args);
    }
}

void Renderer2D::End()
{
}

void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
{
    if (s_R2DData.quadCount >= s_R2DData.maxQuadCount)
    {
        //Renderer2D::Flush();
        return;
    }

    static constexpr u32 quadVertexCount = 4;
    static constexpr glm::vec2 textureCoords[] = {
        { 0.0f, 1.0f },
        { 1.0f, 0.0f },
        { 0.0f, 0.0f },
        { 1.0f, 1.0f }
    };

    glm::vec2 quadPositions[4];
    quadPositions[0] = position + glm::vec3(-size.x * 0.5f, -size.y * 0.5f, 0.0f); // bottom-left
    quadPositions[1] = position + glm::vec3( size.x * 0.5f,  size.y * 0.5f, 0.0f); // top-right
    quadPositions[2] = position + glm::vec3(-size.x * 0.5f,  size.y * 0.5f, 0.0f); // top-left
    quadPositions[3] = position + glm::vec3( size.x * 0.5f, -size.y * 0.5f, 0.0f); // bottom-right

    for (u32 i = 0; i < quadVertexCount; ++i)
    {
        s_R2DData.quadVertexBufferPtr->position = quadPositions[i];
        s_R2DData.quadVertexBufferPtr->texCoord = textureCoords[i];
        s_R2DData.quadVertexBufferPtr->color = color;
        s_R2DData.quadVertexBufferPtr++;
    }

    s_R2DData.quadIndexCount += 6;
    s_R2DData.quadCount++;
}

nvrhi::ICommandList *Renderer2D::GetCommandList()
{
    return s_R2DData.commandList;
}
