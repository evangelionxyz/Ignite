#pragma once

#include "lighting.hpp"
#include "texture.hpp"

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
        nvrhi::IDevice *device;
        nvrhi::ICommandList *commandList;
        nvrhi::IBindingLayout *bindingLayout;
    };

    class Environment
    {
    public:
        Environment() = default;
        Environment(const EnvironmentCreateInfo &createInfo);

        void Render(nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera);
        void LoadTexture(const std::string &filepath);
        void WriteTexture();

        void SetSunDirection(float pitch, float yaw);

        static Ref<Environment> Create(const EnvironmentCreateInfo &createInfo);

        static nvrhi::VertexAttributeDesc GetAttribute();
        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc();

        EnvironmentParams params;
        DirLight dirLight;

        nvrhi::TextureHandle GetHDRTexture() { return m_HDRTexture->GetHandle(); }

        nvrhi::BufferHandle GetParamsBuffer() { return m_ParamsConstantBuffer; }
        nvrhi::BufferHandle GetDirLightBuffer() { return m_DirLightConstantBuffer; }
        nvrhi::BufferHandle GetCameraBuffer() { return m_CameraConstantBuffer; }

    private:
        void CreateLightingBuffer();

        EnvironmentCreateInfo m_CreateInfo;

        nvrhi::BufferHandle m_VertexBuffer;
        nvrhi::BufferHandle m_IndexBuffer;

        nvrhi::BufferHandle m_CameraConstantBuffer;
        nvrhi::BufferHandle m_ParamsConstantBuffer;
        nvrhi::BufferHandle m_DirLightConstantBuffer;

        nvrhi::BindingSetHandle m_BindingSet;

        Ref<Texture> m_HDRTexture;
    };
}