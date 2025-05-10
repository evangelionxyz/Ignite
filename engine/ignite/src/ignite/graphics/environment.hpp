#pragma once

#include "lighting.hpp"

#include <string>
#include <vector>
#include <nvrhi/nvrhi.h>

namespace ignite {

    class Camera;

    struct EnvironmentParams
    {
        float exposure = 4.5f;
        float gamma = 2.2f;
    };

    struct EnvironmentCreateInfo
    {
        // order +X, -X, +Y, -Y, +Z, -Z.

        const char *rightFilepath = nullptr;
        const char *leftFilepath = nullptr;
        const char *topFilepath = nullptr;
        const char *bottomFilepath = nullptr;
        const char *frontFilepath = nullptr;
        const char *backFilepath = nullptr;
    };

    class Environment
    {
    public:
        Environment() = default;
        Environment(nvrhi::IDevice *device, nvrhi::CommandListHandle commandList, const EnvironmentCreateInfo &createInfo, nvrhi::BindingLayoutHandle bindingLayout);

        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera);

        static nvrhi::VertexAttributeDesc GetAttribute();
        static nvrhi::BindingLayoutDesc GetBindingLayoutDesc();

        EnvironmentParams params;
        DirLight dirLight;

    private:
        nvrhi::BufferHandle m_ConstantBuffer;
        nvrhi::BufferHandle m_VertexBuffer;
        nvrhi::BufferHandle m_IndexBuffer;
        nvrhi::BufferHandle m_ParamsConstantBuffer;
        nvrhi::BindingSetHandle m_BindingSet;

        nvrhi::TextureHandle m_CubeTexture;
        nvrhi::SamplerHandle m_Sampler;

    };
}