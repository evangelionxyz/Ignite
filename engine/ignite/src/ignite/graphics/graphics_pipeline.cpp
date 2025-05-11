#include "graphics_pipeline.hpp"
#include "ignite/core/logger.hpp"
#include "renderer.hpp"

#include <ShaderMake/ShaderMake.h>

namespace ignite {

    GraphicsPipeline::GraphicsPipeline(nvrhi::IDevice *device, const GraphicsPipelineParams &params, const GraphicsPiplineCreateInfo &createInfo)
        : m_Params(params)
    {
        CreateShaders(device);

        m_InputLayout = device->createInputLayout(createInfo.attributes, createInfo.attributeCount, nullptr); // vertex buffer is only for DX11
        LOG_ASSERT(m_InputLayout, "[Graphics Pipeline] Failed to create binding layout");

        m_BindingLayout = device->createBindingLayout(createInfo.bindingLayoutDesc);
        LOG_ASSERT(m_BindingLayout, "[Graphics Pipeline] Failed to create binding layout");
    }

    void GraphicsPipeline::Create(nvrhi::IDevice *device, nvrhi::IFramebuffer *framebuffer)
    {
        if (m_Handle == nullptr)
        {
            // create graphics pipeline
            nvrhi::BlendState blendState;
            blendState.targets[0].blendEnable = m_Params.enableBlend;

            nvrhi::DepthStencilState depthStencilState;
            depthStencilState.depthWriteEnable = m_Params.depthWrite;
            depthStencilState.depthTestEnable = m_Params.depthTest;
            depthStencilState.depthFunc = m_Params.comparison; // use 1.0 for far depth

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
            pipelineDesc.setVertexShader(m_VertexShader);
            pipelineDesc.setPixelShader(m_PixelShader);
            pipelineDesc.setInputLayout(m_InputLayout);
            pipelineDesc.setRenderState(renderState);
            pipelineDesc.primType = m_Params.primitiveType;

            pipelineDesc.addBindingLayout(m_BindingLayout);

            // create with the same framebuffer to be render
            m_Handle = device->createGraphicsPipeline(pipelineDesc, framebuffer);
            LOG_ASSERT(m_Handle, "Failed to create graphics pipeline");
        }
    }

    void GraphicsPipeline::ResetHandle()
    {
        m_Handle.Reset();
    }

    void GraphicsPipeline::CreateShaders(nvrhi::IDevice *device)
    {
        ShaderMake::ShaderContextDesc shaderDesc = ShaderMake::ShaderContextDesc();
        Ref<ShaderMake::ShaderContext> vsContext = CreateRef<ShaderMake::ShaderContext>(m_Params.vertexShaderFilepath, ShaderMake::ShaderType::Vertex, shaderDesc, m_Params.recompileShader);
        Ref<ShaderMake::ShaderContext> psContext = CreateRef<ShaderMake::ShaderContext>(m_Params.pixelShaderFilepath, ShaderMake::ShaderType::Pixel, shaderDesc, m_Params.recompileShader);

        Renderer::GetShaderContext()->CompileShader({ vsContext, psContext });

        m_VertexShader = device->createShader(nvrhi::ShaderType::Vertex, vsContext->blob.data.data(), vsContext->blob.dataSize());
        m_PixelShader = device->createShader(nvrhi::ShaderType::Pixel, psContext->blob.data.data(), psContext->blob.dataSize());
        LOG_ASSERT(m_VertexShader && m_PixelShader, "[Graphics Pipline] Failed to create shaders");
    }

    Ref<GraphicsPipeline> GraphicsPipeline::Create(nvrhi::IDevice *device, const GraphicsPipelineParams &params, const GraphicsPiplineCreateInfo &createInfo)
    {
        return CreateRef<GraphicsPipeline>(device, params, createInfo);
    }

}