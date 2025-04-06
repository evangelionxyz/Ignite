#pragma once

#include "core/types.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <stdint.h>

#include <nvrhi/nvrhi.h>

#include <imgui.h>

class ShaderFactory;

struct ImGui_NVRHI
{
    nvrhi::DeviceHandle m_Device;
    nvrhi::CommandListHandle m_CommandList;
    nvrhi::ShaderHandle VertexShader;
    nvrhi::ShaderHandle PixelShader;
    nvrhi::InputLayoutHandle ShaderAttribLayout;

    nvrhi::TextureHandle FontTexture;
    nvrhi::SamplerHandle FontSampler;

    nvrhi::BufferHandle VertexBuffer;
    nvrhi::BufferHandle IndexBuffer;

    nvrhi::BindingLayoutHandle BindingLayout;
    nvrhi::GraphicsPipelineDesc BasePSODesc;

    nvrhi::GraphicsPipelineHandle PSO;
    std::unordered_map<nvrhi::ITexture *, nvrhi::BindingSetHandle> BindingsCache;

    std::vector<ImDrawVert> VtxBuffer;
    std::vector<ImDrawIdx> IdxBuffer;

    bool Init(nvrhi::IDevice *device, Ref<ShaderFactory> shaderFactory);
    void Shutdown();
    bool UpdateFontTexture();
    bool Render(nvrhi::IFramebuffer *framebuffer);
    void BackBufferResizing();

private:
    bool ReallocateBuffer(nvrhi::BufferHandle &buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer);
    nvrhi::IGraphicsPipeline *GetPSO(nvrhi::IFramebuffer *framebuffer);
    nvrhi::IBindingSet *GetBindingSet(nvrhi::ITexture *texture);
    bool UpdateGeometry(nvrhi::ICommandList *commandList);
};