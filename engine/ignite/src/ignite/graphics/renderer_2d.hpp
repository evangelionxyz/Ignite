#pragma once

#include "ignite/core/types.hpp"
#include "vertex_data.hpp"

#include "shader.hpp"

namespace ignite
{
    class DeviceManager;
    class Texture;
    class Camera;

    struct PushConstant2D
    {
        glm::mat4 mvpMatrix;
    };

    struct BatchRender
    {
        const size_t maxCount = 1024 * 3;
        const size_t maxVertices = maxCount * 4;
        const size_t maxIndices = maxCount * 6;
        const i32 maxTextureCount = 16;

        VertexQuad *vertexBufferBase = nullptr;
        VertexQuad *vertexBufferPtr = nullptr;
        nvrhi::BufferHandle vertexBuffer = nullptr;
        nvrhi::BufferHandle indexBuffer = nullptr;
        Ref<Shader> vertexShader;
        Ref<Shader> pixelShader;
        nvrhi::InputLayoutHandle inputLayout = nullptr;
        nvrhi::BindingLayoutHandle bindingLayout = nullptr;
        nvrhi::GraphicsPipelineHandle pipeline = nullptr;
        nvrhi::SamplerHandle sampler = nullptr;
        nvrhi::BindingSetHandle bindingSet = nullptr;
        std::vector<Ref<Texture>> textureSlots;
        u8 textureSlotIndex = 1; // 0 for white texture
        u32 indexCount = 0;
        u32 count = 0;

        ~BatchRender()
        {
            vertexBufferPtr = nullptr;
            if (vertexBufferBase)
                delete[] vertexBufferBase;

            indexCount = 0;
            count = 0;
        }
    };

    struct Renderer2DData
    {
        BatchRender quadBatch;
        glm::vec4 quadPositions[4];

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

        static void CreateGraphicsPipeline(nvrhi::IDevice *device, nvrhi::IFramebuffer *framebuffer);

        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, f32 rotation, const glm::vec4 &color, Ref<Texture> texture = nullptr, const glm::vec2 &tilingFactor = glm::vec2(1.0f));
        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color, Ref<Texture> texture = nullptr, const glm::vec2 &tilingFactor = glm::vec2(1.0f));
        static void DrawQuad(const glm::mat4 &transform, const glm::vec4 &color, Ref<Texture> texture = nullptr, const glm::vec2 &tilingFactor = glm::vec2(1.0f));

        static void InitConstantBuffer(nvrhi::IDevice *device);
        static void InitQuadData(nvrhi::IDevice *device, nvrhi::ICommandList *commandList);

        static u32 GetOrInsertTexture(Ref<Texture> texture);
        static void UpdateTextureBindings();

        static nvrhi::ICommandList *GetCommandList();
    };
}
