#pragma once

#include "lighting.hpp"

#include <string>
#include <filesystem>

#include <vector>
#include <nvrhi/nvrhi.h>

namespace ignite {

    class Camera;

    struct EnvironmentParams
    {
        float exposure = 1.0f;
        float gamma = 0.9f;
    };

    struct EnvironmentCreateInfo
    {
        std::filesystem::path filepath;
    };

    class Environment
    {
    public:
        Environment() = default;
        Environment(nvrhi::IDevice *device, nvrhi::CommandListHandle commandList, const EnvironmentCreateInfo &createInfo, nvrhi::BindingLayoutHandle bindingLayout);

        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera);

        void SetSunDirection(float pitch, float yaw);

        static nvrhi::VertexAttributeDesc GetAttribute();
        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc();

        EnvironmentParams params;
        DirLight dirLight;

        nvrhi::TextureHandle GetHDRTexture() { return m_HDRTexture; }

        nvrhi::BufferHandle GetParamsBuffer() { return m_ParamsConstantBuffer; }
        nvrhi::BufferHandle GetDirLightBuffer() { return m_DirLightConstantBuffer; }
        nvrhi::BufferHandle GetCameraBuffer() { return m_CameraConstantBuffer; }

    private:
        void CreateLightingBuffer(nvrhi::IDevice *device);

        nvrhi::BufferHandle m_VertexBuffer;
        nvrhi::BufferHandle m_IndexBuffer;

        nvrhi::BufferHandle m_CameraConstantBuffer;
        nvrhi::BufferHandle m_ParamsConstantBuffer;
        nvrhi::BufferHandle m_DirLightConstantBuffer;

        nvrhi::BindingSetHandle m_BindingSet;

        nvrhi::TextureHandle m_HDRTexture;
        nvrhi::SamplerHandle m_Sampler;

    };
}