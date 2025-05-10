#pragma once

#include "ignite/core/types.hpp"

#include <nvrhi/nvrhi.h>

namespace ignite {

    struct GraphicsPiplineCreateInfo
    {
        nvrhi::BindingLayoutDesc bindingLayoutDesc;
        nvrhi::VertexAttributeDesc *attributes;
        uint32_t attributeCount = 0;
    };

    struct GraphicsPipelineParams
    {
        std::string vertexShaderFilepath;
        std::string pixelShaderFilepath;
        
        nvrhi::RasterCullMode cullMode = nvrhi::RasterCullMode::Front;
        nvrhi::ComparisonFunc comparison = nvrhi::ComparisonFunc::LessOrEqual;
        nvrhi::PrimitiveType primitiveType = nvrhi::PrimitiveType::TriangleList;

        bool enableBlend = true;
        bool depthWrite = false;
        bool depthTest = false;
        bool recompileShader = false;
    };

    class GraphicsPipeline
    {
    public:
        GraphicsPipeline() = default;
        GraphicsPipeline(nvrhi::IDevice *device, const GraphicsPipelineParams &params, const GraphicsPiplineCreateInfo &createInfo);

        void Create(nvrhi::IDevice *device, nvrhi::IFramebuffer *framebuffer);

        nvrhi::GraphicsPipelineHandle GetHandle() { return m_Handle; }
        nvrhi::ShaderHandle GetVertexShader() { return m_VertexShader; }
        nvrhi::ShaderHandle GetPixelShader() { return m_PixelShader; }
        nvrhi::InputLayoutHandle GetInputLayout() { return m_InputLayout; }
        nvrhi::BindingLayoutHandle GetBindingLayout() { return m_BindingLayout; }

        static Ref<GraphicsPipeline> Create(nvrhi::IDevice *device, const GraphicsPipelineParams &params, const GraphicsPiplineCreateInfo &createInfo);

    private:

        void CreateShaders(nvrhi::IDevice *device);

        nvrhi::GraphicsPipelineHandle m_Handle;
        nvrhi::ShaderHandle m_VertexShader;
        nvrhi::ShaderHandle m_PixelShader;
        nvrhi::InputLayoutHandle m_InputLayout;
        nvrhi::BindingLayoutHandle m_BindingLayout;
        GraphicsPipelineParams m_Params;
    };
}