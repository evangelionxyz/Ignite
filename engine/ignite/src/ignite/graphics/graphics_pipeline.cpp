#include "graphics_pipeline.hpp"
#include "ignite/core/logger.hpp"

#include "renderer.hpp"

#include "ignite/core/application.hpp"

namespace ignite {

    GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineParams &params, GraphicsPiplineCreateInfo *createInfo, nvrhi::BindingLayoutHandle bindingLayout)
        : m_Params(params), m_CreateInfo(std::move(createInfo)), m_BindingLayout(bindingLayout)
    {
    }

    GraphicsPipeline& GraphicsPipeline::AddShader(const std::string& filepath, nvrhi::ShaderType type, bool recompile)
    {
        ShaderMake::ShaderType shaderType = GetShaderMakeShaderType(type);
        Ref<ShaderMake::ShaderContext> context = CreateRef<ShaderMake::ShaderContext>(filepath, shaderType, ShaderMake::ShaderContextDesc(), recompile);
        m_ShaderContexts.push_back(std::move(context));

        m_NeedsToCompileShader = true;

        return *this;
    }

    GraphicsPipeline& GraphicsPipeline::AddShader(nvrhi::ShaderHandle& handle, nvrhi::ShaderType type)
    {
        m_Shaders[type] = handle;
        return *this;
    }

    void GraphicsPipeline::CreatePipeline(nvrhi::IFramebuffer *framebuffer)
    {
        if (m_Handle == nullptr)
        {
            // create graphics pipeline
            nvrhi::BlendState blendState;
            blendState.targets[0].blendEnable = m_Params.enableBlend;
            blendState.targets[1].blendEnable = false;
            blendState.targets[1].colorWriteMask = nvrhi::ColorMask::All;

            nvrhi::DepthStencilState depthStencilState;
            depthStencilState.depthWriteEnable = m_Params.depthWrite;
            depthStencilState.depthTestEnable = m_Params.depthTest;
            depthStencilState.depthFunc = m_Params.comparison;

            depthStencilState.stencilEnable = m_Params.enableDepthStencil;
            depthStencilState.frontFaceStencil = m_Params.frontFaceStencilDesc;
            depthStencilState.backFaceStencil = m_Params.backFaceStencilDesc;
            depthStencilState.stencilWriteMask = m_Params.stencilWriteMask;
            depthStencilState.stencilReadMask = m_Params.stencilReadMask;
            depthStencilState.stencilRefValue = m_Params.stencilRefValue;

            nvrhi::RasterState rasterState;

            rasterState.cullMode = m_Params.cullMode;
            rasterState.fillMode = m_Params.fillMode;
            rasterState.setFrontCounterClockwise(false);
            rasterState.setMultisampleEnable(false);

            nvrhi::RenderState renderState;
            renderState.setRasterState(rasterState);
            renderState.setDepthStencilState(depthStencilState);
            renderState.setBlendState(blendState);

            nvrhi::GraphicsPipelineDesc pipelineDesc;

            for (auto& shader : m_Shaders)
            {
                if (shader.first == nvrhi::ShaderType::Vertex)
                    pipelineDesc.setVertexShader(shader.second);
                else if (shader.first == nvrhi::ShaderType::Pixel)
                    pipelineDesc.setPixelShader(shader.second);
            }

            pipelineDesc.setInputLayout(m_InputLayout);
            pipelineDesc.setRenderState(renderState);
            pipelineDesc.primType = m_Params.primitiveType;

            if (m_BindingLayout)
                pipelineDesc.addBindingLayout(m_BindingLayout);

            // create with the same framebuffer to be render
            nvrhi::IDevice* device = Application::GetRenderDevice();

            m_Handle = device->createGraphicsPipeline(pipelineDesc, framebuffer);
            LOG_ASSERT(m_Handle, "Failed to create graphics pipeline");
        }
    }

    void GraphicsPipeline::ResetHandle()
    {
        m_Handle.Reset();
    }

    void GraphicsPipeline::Build()
    {
        nvrhi::IDevice* device = Application::GetRenderDevice();

        if (m_NeedsToCompileShader)
        {
            Renderer::GetShaderLibrary().GetContext()->CompileShader(m_ShaderContexts);

            for (auto& context : m_ShaderContexts)
            {
                nvrhi::ShaderType shaderType = GetNVRHIShaderType(context->GetType());
                m_Shaders[shaderType] = device->createShader(shaderType, context->blob.data.data(), context->blob.dataSize());

                LOG_ASSERT(m_Shaders[shaderType], "[Graphics Pipline] Failed to create shader");
            }

            m_ShaderContexts.clear();
        }

        m_InputLayout = device->createInputLayout(m_CreateInfo->attributes, m_CreateInfo->attributeCount, nullptr);
        LOG_ASSERT(m_InputLayout, "[Graphics Pipeline] Failed to create input layout");
    }

    Ref<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineParams &params, GraphicsPiplineCreateInfo *createInfo, nvrhi::BindingLayoutHandle bindingLayout)
    {
        return CreateRef<GraphicsPipeline>(params, createInfo, bindingLayout);
    }

}