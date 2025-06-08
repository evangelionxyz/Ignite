#pragma once

#include "lighting.hpp"
#include "texture.hpp"

#include <string>
#include <filesystem>

#include <vector>
#include <nvrhi/nvrhi.h>

namespace ignite {

    class GraphicsPipeline;

    struct EnvironmentParams
    {
        float exposure = 1.0f;
        float gamma = 2.2f;
    };

    class Environment
    {
    public:
        Environment();

        void Render(nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer, const Ref<GraphicsPipeline> &pipeline);
        void LoadTexture(const std::string &filepath);
        void WriteBuffer(nvrhi::ICommandList *commandList);
        void SetSunDirection(float pitch, float yaw);

        bool IsUpdatingTexture() const { return m_IsUpdatingTexture; }

        static Ref<Environment> Create();

        static nvrhi::VertexAttributeDesc GetAttribute();
        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc();

        EnvironmentParams params;
        DirLight dirLight;

        nvrhi::TextureHandle GetHDRTexture() { return m_HDRTexture->GetHandle(); }
        nvrhi::BufferHandle GetParamsBuffer() { return m_ParamsConstantBuffer; }
        nvrhi::BufferHandle GetDirLightBuffer() { return m_DirLightConstantBuffer; }

    private:

        nvrhi::BufferHandle m_VertexBuffer;
        nvrhi::BufferHandle m_IndexBuffer;
        nvrhi::BufferHandle m_ParamsConstantBuffer;
        nvrhi::BufferHandle m_DirLightConstantBuffer;

        nvrhi::BindingSetHandle m_BindingSet;

        Ref<Texture> m_HDRTexture;
        bool m_IsUpdatingTexture = false;

        friend class EnvironmentImporter;


    };
}