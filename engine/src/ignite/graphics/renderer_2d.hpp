#pragma once

#include "ignite/core/types.hpp"

#include "vertex_data.hpp"

class Camera;
class DeviceManager;
class Texture;

struct PushConstant2D
{
    glm::mat4 mvpMatrix;
};

struct BatchRender
{
    size_t maxCount = 1024 * 2;
    size_t maxVertices = maxCount * 4;
    size_t maxIndices = maxCount * 6;
    VertexQuad *vertexBufferBase = nullptr;
    VertexQuad *vertexBufferPtr = nullptr;
    nvrhi::BufferHandle vertexBuffer = nullptr;
    nvrhi::BufferHandle indexBuffer = nullptr;
    nvrhi::ShaderHandle vertexShader = nullptr;
    nvrhi::ShaderHandle pixelShader = nullptr;
    Ref<Texture> texture = nullptr;
    nvrhi::InputLayoutHandle inputLayout = nullptr;
    nvrhi::BindingLayoutHandle bindingLayout = nullptr;
    nvrhi::BindingSetHandle bindingSet = nullptr;
    nvrhi::GraphicsPipelineHandle pipeline = nullptr;
    u32 indexCount = 0;
    u32 count = 0;

    void Destroy()
    {
        vertexBufferPtr = nullptr;
        if (vertexBufferBase)
            delete[] vertexBufferBase;

        vertexBuffer = nullptr;
        indexBuffer = nullptr;
        vertexShader = nullptr;
        pixelShader = nullptr;
        texture = nullptr;
        inputLayout = nullptr;
        bindingLayout = nullptr;
        bindingSet = nullptr;
        pipeline = nullptr;

        indexCount = 0;
        count = 0;
    }
};

struct Renderer2DData
{
    BatchRender quadBatch;

    nvrhi::IFramebuffer *framebuffer;
    nvrhi::BufferHandle constantBuffer = nullptr;
    DeviceManager *deviceManager = nullptr;
    nvrhi::ICommandList *commandList = nullptr;
};

class Renderer2D
{
public:
    static void Init(DeviceManager *deviceManager);
    static void Shutdown();

    static void Begin(Camera *camera, nvrhi::IFramebuffer *framebuffer);
    static void Flush();
    static void End();

    static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color);

    static void InitConstantBuffer(nvrhi::IDevice *device);
    static void InitQuadData(nvrhi::IDevice *device, nvrhi::ICommandList *commandList);

    static nvrhi::ICommandList *GetCommandList();
};
