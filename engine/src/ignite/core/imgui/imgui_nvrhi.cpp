#include "imgui_nvrhi.hpp"
#include "core/logger.hpp"

#include "graphics/shader_factory.hpp"

struct VERTEX_CONSTANT_BUFFER
{
    float MVP[4][4];
};

bool ImGui_NVRHI::UpdateFontTexture()
{
    ImGuiIO &io = ImGui::GetIO();

    // If the font texture exists and is bound to ImGui, we're done.
    // Note: ImGui_Renderer will reset io.Fonts->TexID when new fonts are added.
    if (FontTexture && io.Fonts->TexID)
        return true;

    unsigned char *pixels;
    i32 width, height;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    if (!pixels)
        return false;

    nvrhi::TextureDesc textureDesc;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.format = nvrhi::Format::RGBA8_UNORM;
    textureDesc.debugName = "ImGui font texture";

    FontTexture = m_Device->createTexture(textureDesc);
    LOG_ASSERT(FontTexture, "Failed to create imgui font texture");

    m_CommandList->open();
    
    m_CommandList->beginTrackingTextureState(FontTexture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
    m_CommandList->writeTexture(FontTexture, 0, 0, pixels, width * 4);
    m_CommandList->setPermanentTextureState(FontTexture, nvrhi::ResourceStates::ShaderResource);
    m_CommandList->commitBarriers();

    m_CommandList->close();
    m_Device->executeCommandList(m_CommandList);

    io.Fonts->TexID = (ImTextureID)FontTexture.Get();

    return true;
}

bool ImGui_NVRHI::Init(nvrhi::IDevice *device, Ref<ShaderFactory> shaderFactory)
{
    m_Device = device;
    m_CommandList = m_Device->createCommandList();

    VertexShader = shaderFactory->CreateAutoShader("imgui_vertex", "main", IGNITE_MAKE_PLATFORM_SHADER(g_imgui_vertex), nullptr, nvrhi::ShaderType::Vertex);
    PixelShader = shaderFactory->CreateAutoShader("imgui_pixel", "main", IGNITE_MAKE_PLATFORM_SHADER(g_imgui_vertex), nullptr, nvrhi::ShaderType::Pixel);
    
    if (!VertexShader || !PixelShader)
    {
        LOG_ERROR("Failed to create ImGui Shader");
        return false;
    }

    nvrhi::VertexAttributeDesc vertexAttribLayout[] = 
    {
        {"POSITION", nvrhi::Format::RG32_FLOAT, 1, 0, offsetof(ImDrawVert, pos), sizeof(ImDrawVert), false},
        {"TEXCOORD", nvrhi::Format::RG32_FLOAT, 1, 0, offsetof(ImDrawVert, uv), sizeof(ImDrawVert), false},
        {"COLOR", nvrhi::Format::RGBA8_UNORM, 1, 0, offsetof(ImDrawVert, col), sizeof(ImDrawVert), false}
    };

    ShaderAttribLayout = m_Device->createInputLayout(vertexAttribLayout, std::size(vertexAttribLayout), VertexShader);

    // Create PSO (Pipeline State Object)
    {
        nvrhi::BlendState blendState;
        blendState.targets[0].setBlendEnable(true)
            .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
            .setDestBlend(nvrhi::BlendFactor::InvSrcAlpha)
            .setSrcBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha)
            .setDestBlendAlpha(nvrhi::BlendFactor::Zero);

        auto rasterState = nvrhi::RasterState()
            .setFillSolid()
            .setCullNone()
            .setScissorEnable(true)
            .setDepthClipEnable(true);
        
        auto depthStencilState = nvrhi::DepthStencilState()
            .disableDepthTest()
            .enableDepthWrite()
            .disableStencil()
            .setDepthFunc(nvrhi::ComparisonFunc::Always);

        nvrhi::RenderState renderState;
        renderState.blendState = blendState;
        renderState.depthStencilState = depthStencilState;
        renderState.rasterState = rasterState;

        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::All;
        layoutDesc.bindings = 
        {
            nvrhi::BindingLayoutItem::PushConstants(0, sizeof(float) * 2),
            nvrhi::BindingLayoutItem::Texture_SRV(0),
            nvrhi::BindingLayoutItem::Sampler(0)
        };

        BindingLayout = m_Device->createBindingLayout(layoutDesc);
        BasePSODesc.primType = nvrhi::PrimitiveType::TriangleList;
        BasePSODesc.inputLayout = ShaderAttribLayout;
        BasePSODesc.VS = VertexShader;
        BasePSODesc.PS = PixelShader;
        BasePSODesc.renderState = renderState;
        BasePSODesc.bindingLayouts = { BindingLayout };
    }

    {
        const auto desc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
            .setAllFilters(true);

        FontSampler = m_Device->createSampler(desc);

        LOG_ASSERT(FontSampler, "Failed to create ImGui font sampler");
        if (!FontSampler)
            return false;
    }

    return true;
}

bool ImGui_NVRHI::Render(nvrhi::IFramebuffer *framebuffer)
{
    ImDrawData *drawData = ImGui::GetDrawData();
    const ImGuiIO &io = ImGui::GetIO();

    m_CommandList->open();
    m_CommandList->beginMarker("ImGui");

    if (!UpdateGeometry(m_CommandList))
    {
        m_CommandList->close();
        return false;
    }

    drawData->ScaleClipRects(io.DisplayFramebufferScale);

    f32 invDisplaySize[2] = { 1.0f / io.DisplaySize.x, 1.0f / io.DisplaySize.y };

    // setup graphics state
    nvrhi::GraphicsState drawState;
    drawState.framebuffer = framebuffer;
    LOG_ASSERT(drawState.framebuffer, "Invalid framebuffer");

    drawState.pipeline = GetPSO(drawState.framebuffer);

    drawState.viewport.viewports.push_back(
        nvrhi::Viewport(
            io.DisplaySize.x * io.DisplayFramebufferScale.x,
            io.DisplaySize.y * io.DisplayFramebufferScale.y
    ));

    drawState.viewport.scissorRects.resize(1);

    nvrhi::VertexBufferBinding vbufBinding;
    vbufBinding.buffer = VertexBuffer;
    vbufBinding.slot = 0;
    vbufBinding.offset = 0;
    drawState.vertexBuffers.push_back(vbufBinding);

    drawState.indexBuffer.buffer = IndexBuffer;
    drawState.indexBuffer.format = sizeof(ImDrawIdx) == 2 ? nvrhi::Format::R16_UINT : nvrhi::Format::R32_UINT;
    drawState.indexBuffer.offset = 0;

    // render command list
    i32 vtxOffset = 0;
    i32 idxOffset = 0;
    for (i32 n = 0; n < drawData->CmdListsCount; ++n)
    {
        const ImDrawList *cmdList = drawData->CmdLists[n];

        for (i32 i = 0; i < cmdList->CmdBuffer.Size; ++i)
        {
            const ImDrawCmd *pCmd = &cmdList->CmdBuffer[i];

            if (pCmd->UserCallback)
            {
                pCmd->UserCallback(cmdList, pCmd);
            }
            else
            {
                drawState.bindings = { GetBindingSet((nvrhi::ITexture *)pCmd->TextureId) };
                LOG_ASSERT(drawState.bindings[0], "Invalid draw state binding");

                drawState.viewport.scissorRects[0] = nvrhi::Rect(
                    int(pCmd->ClipRect.x),
                    int(pCmd->ClipRect.z),
                    int(pCmd->ClipRect.y),
                    int(pCmd->ClipRect.w)
                );

                nvrhi::DrawArguments drawArguments;
                drawArguments.vertexCount = pCmd->ElemCount;
                drawArguments.startVertexLocation = vtxOffset;
                drawArguments.startIndexLocation = idxOffset;

                m_CommandList->setGraphicsState(drawState);
                m_CommandList->setPushConstants(invDisplaySize, sizeof(invDisplaySize));
                m_CommandList->drawIndexed(drawArguments);
            }
            idxOffset += pCmd->ElemCount;
        }
        vtxOffset += cmdList->VtxBuffer.Size;
    }

    m_CommandList->endMarker();
    m_CommandList->close();
    m_Device->executeCommandList(m_CommandList);

    return true;
}

void ImGui_NVRHI::BackBufferResizing()
{
    PSO = nullptr;
}

bool ImGui_NVRHI::ReallocateBuffer(nvrhi::BufferHandle &buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer)
{
    if (buffer == nullptr || size_t(buffer->getDesc().byteSize) < requiredSize)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = u32(reallocateSize);
        desc.debugName = IndexBuffer ? "ImGui index buffer" : "ImGui vertex buffer";
        desc.canHaveUAVs = false;
        desc.isVertexBuffer = !isIndexBuffer;
        desc.isIndexBuffer = isIndexBuffer;
        desc.isDrawIndirectArgs = false;
        desc.isVolatile = false;
        desc.initialState = isIndexBuffer ? nvrhi::ResourceStates::IndexBuffer : nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;

        buffer = m_Device->createBuffer(desc);

        if (!buffer)
            return false;
    }

    return true;
}

nvrhi::IGraphicsPipeline *ImGui_NVRHI::GetPSO(nvrhi::IFramebuffer *framebuffer)
{
    if (PSO)
        return PSO;

    PSO = m_Device->createGraphicsPipeline(BasePSODesc, framebuffer);
    LOG_ASSERT(PSO, "Failed to create ImGui PSO");

    
    return PSO;
}

nvrhi::IBindingSet *ImGui_NVRHI::GetBindingSet(nvrhi::ITexture *texture)
{
    auto iter = BindingsCache.find(texture);
    if (iter != BindingsCache.end())
        return iter->second;
    
    nvrhi::BindingSetDesc desc;
    desc.bindings = 
    {
        nvrhi::BindingSetItem::PushConstants(0, sizeof(float) * 2),
        nvrhi::BindingSetItem::Texture_SRV(0, texture),
        nvrhi::BindingSetItem::Sampler(0, FontSampler)
    };

    nvrhi::BindingSetHandle binding;
    binding = m_Device->createBindingSet(desc, BindingLayout);
    LOG_ASSERT(binding, "Failed to create ImGui binding set");

    BindingsCache[texture] = binding;
    return binding;
}

bool ImGui_NVRHI::UpdateGeometry(nvrhi::ICommandList *commandList)
{
    ImDrawData *drawData = ImGui::GetDrawData();

    if (!ReallocateBuffer(VertexBuffer, drawData->TotalVtxCount * sizeof(ImDrawVert),
        (drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert),
        false))
    {
        return false;
    }

    if (!ReallocateBuffer(IndexBuffer, drawData->TotalIdxCount * sizeof(ImDrawIdx),
        (drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx),
        true))
    {
        return false;
    }

    VtxBuffer.resize(VertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
    IdxBuffer.resize(IndexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

    ImDrawVert *vtxDst = &VtxBuffer[0];
    ImDrawIdx *idxDst = &IdxBuffer[0];

    for (i32 n = 0; n < drawData->CmdListsCount; ++n)
    {
        const ImDrawList *cmdList = drawData->CmdLists[n];
        memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

        vtxDst += cmdList->VtxBuffer.Size;
        idxDst += cmdList->IdxBuffer.Size;
    }

    commandList->writeBuffer(VertexBuffer, &VtxBuffer[0], VertexBuffer->getDesc().byteSize);
    commandList->writeBuffer(IndexBuffer, &IdxBuffer[0], IndexBuffer->getDesc().byteSize);

    return true;
}

void ImGui_NVRHI::Shutdown()
{
    FontTexture = nullptr;
    FontSampler = nullptr;
    VertexBuffer = nullptr;
    IndexBuffer = nullptr;

    VertexShader = nullptr;
    PixelShader = nullptr;

    m_CommandList = nullptr;
}