#pragma once

#include "ignite/core/types.hpp"

#include "vertex_data.hpp"

class Camera;
class DeviceManager;

struct PushConstant2D
{
    glm::mat4 mvpMatrix;
};

struct Renderer2DData
{
    const size_t maxQuadCount = 1024 * 2;
    const size_t maxQuadVerts = maxQuadCount * 4;
    const size_t maxQuadIndices = maxQuadCount * 6;
    u32 quadCount = 0;
    VertexQuad *quadVertexBufferBase = nullptr;
    VertexQuad *quadVertexBufferPtr = nullptr;
    u32 quadIndexCount = 0;
    nvrhi::BufferHandle quadVertexBuffer = nullptr;
    nvrhi::BufferHandle quadIndexBuffer = nullptr;
    nvrhi::ShaderHandle quadVertexShader = nullptr;
    nvrhi::ShaderHandle quadPixelShader = nullptr;
    nvrhi::TextureHandle quadTexture = nullptr;
    nvrhi::SamplerHandle quadTextureSampler = nullptr;
    nvrhi::InputLayoutHandle quadInputLayout = nullptr;
    nvrhi::BindingLayoutHandle quadBindingLayout = nullptr;
    nvrhi::BindingSetHandle quadBindingSet = nullptr;
    nvrhi::GraphicsPipelineHandle quadPipeline = nullptr;

    nvrhi::BufferHandle constantBuffer = nullptr;
    DeviceManager *deviceManager = nullptr;
    nvrhi::ICommandList *commandList = nullptr;
};

class Renderer2D
{
public:
    static void Init(DeviceManager *deviceManager);
    static void Shutdown();

    static void Begin(Camera *camera);
    static void Flush(nvrhi::IFramebuffer *framebuffer);
    static void End();

    static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color);

    static void InitConstantBuffer(nvrhi::IDevice *device);
    static void InitQuadData(nvrhi::IDevice *device, nvrhi::ICommandList *commandList);

    static nvrhi::ICommandList *GetCommandList();
};
