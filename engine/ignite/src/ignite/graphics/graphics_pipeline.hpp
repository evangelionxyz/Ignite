#pragma once

#include "ignite/core/types.hpp"
#include "shader.hpp"

#include <nvrhi/nvrhi.h>
#include <vector>
#include <unordered_map>

namespace ignite {

    struct GraphicsPiplineCreateInfo
    {
        nvrhi::BindingLayoutDesc bindingLayoutDesc;
        nvrhi::VertexAttributeDesc *attributes;
        uint32_t attributeCount = 0;
    };

    struct GraphicsPipelineParams
    {
        nvrhi::RasterCullMode cullMode = nvrhi::RasterCullMode::Front;
        nvrhi::ComparisonFunc comparison = nvrhi::ComparisonFunc::LessOrEqual;
        nvrhi::PrimitiveType primitiveType = nvrhi::PrimitiveType::TriangleList;
        nvrhi::RasterFillMode fillMode = nvrhi::RasterFillMode::Solid;

        bool enableBlend = true;
        bool depthWrite = false;
        bool depthTest = false;
        bool recompileShader = false;
    };

    class GraphicsPipeline
    {
    public:
        GraphicsPipeline() = default;
        GraphicsPipeline(const GraphicsPipelineParams &params, GraphicsPiplineCreateInfo *createInfo);

        GraphicsPipeline& AddShader(const std::string& filepath, nvrhi::ShaderType type, bool recompile = false);
        void CompileShaders();

        void Create(nvrhi::IFramebuffer *framebuffer);
        void ResetHandle();

        nvrhi::GraphicsPipelineHandle GetHandle() { return m_Handle; }
        nvrhi::InputLayoutHandle GetInputLayout() { return m_InputLayout; }
        nvrhi::BindingLayoutHandle GetBindingLayout() { return m_BindingLayout; }

        nvrhi::ShaderHandle GetShader(nvrhi::ShaderType type)
        {
            if (m_Shaders.contains(type))
                return m_Shaders[type];

            return nullptr;
        }

        static Ref<GraphicsPipeline> Create(const GraphicsPipelineParams &params, GraphicsPiplineCreateInfo *createInfo);

        GraphicsPipelineParams &GetParams() { return m_Params; }

    private:
        nvrhi::GraphicsPipelineHandle m_Handle;

        std::unordered_map<nvrhi::ShaderType, nvrhi::ShaderHandle> m_Shaders;
        std::vector<Ref<ShaderMake::ShaderContext>> m_ShaderContexts;

        nvrhi::InputLayoutHandle m_InputLayout;
        nvrhi::BindingLayoutHandle m_BindingLayout;
        GraphicsPipelineParams m_Params;
        GraphicsPiplineCreateInfo *m_CreateInfo;
    };
}