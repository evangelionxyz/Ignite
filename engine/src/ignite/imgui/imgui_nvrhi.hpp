#pragma once

#include "ignite/core/types.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <stdint.h>

#include <nvrhi/nvrhi.h>

#include <imgui.h>

namespace ignite
{
    class ShaderFactory;

    struct ImGui_NVRHI
    {
        nvrhi::DeviceHandle device;
        nvrhi::CommandListHandle commandList;
        nvrhi::ShaderHandle vertexShader;
        nvrhi::ShaderHandle pixelShader;
        nvrhi::InputLayoutHandle attributeLayout;

        nvrhi::TextureHandle fontTexture;
        nvrhi::SamplerHandle fontSampler;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;

        nvrhi::BindingLayoutHandle bindingLayout;
        nvrhi::GraphicsPipelineDesc graphicsPipelineDesc;

        nvrhi::GraphicsPipelineHandle graphicsPipeline;
        std::unordered_map<nvrhi::ITexture *, nvrhi::BindingSetHandle> bindingsCache;

        std::vector<ImDrawVert> imguiVertexBuffer;
        std::vector<ImDrawIdx> imguiIndexBuffer;

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
}
