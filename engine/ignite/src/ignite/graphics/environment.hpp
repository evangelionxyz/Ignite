#pragma once

#include "lighting.hpp"
#include "texture.hpp"

#include <string>
#include <filesystem>

#include <vector>
#include <nvrhi/nvrhi.h>

namespace ignite {

    class Camera;
    class GraphicsPipeline;

    struct EnvironmentParams
    {
        float exposure = 1.0f;
        float gamma = 2.2f;
    };

    class Environment
    {
    public:
        Environment() = default;
        Environment(nvrhi::IDevice *device);

        void Render(nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer, const Ref<GraphicsPipeline> &pipeline, Camera *camera);
        void LoadTexture(nvrhi::IDevice *device, const std::string &filepath, nvrhi::BindingLayoutHandle bindingLayout);

        void WriteBuffer(nvrhi::ICommandList *commandList);

        void SetSunDirection(float pitch, float yaw);

        static Ref<Environment> Create(nvrhi::IDevice *device);

        static nvrhi::VertexAttributeDesc GetAttribute();
        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc();

        EnvironmentParams params;
        DirLight dirLight;

        nvrhi::TextureHandle GetHDRTexture() { return m_HDRTexture->GetHandle(); }

        nvrhi::BufferHandle GetParamsBuffer() { return m_ParamsConstantBuffer; }
        nvrhi::BufferHandle GetDirLightBuffer() { return m_DirLightConstantBuffer; }
        nvrhi::BufferHandle GetCameraBuffer() { return m_CameraConstantBuffer; }

    private:

        nvrhi::BufferHandle m_VertexBuffer;
        nvrhi::BufferHandle m_IndexBuffer;

        nvrhi::BufferHandle m_CameraConstantBuffer;
        nvrhi::BufferHandle m_ParamsConstantBuffer;
        nvrhi::BufferHandle m_DirLightConstantBuffer;

        nvrhi::BindingSetHandle m_BindingSet;

        Ref<Texture> m_HDRTexture;
    };
}