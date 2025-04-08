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
    if (fontTexture && io.Fonts->TexID)
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

    fontTexture = device->createTexture(textureDesc);
    LOG_ASSERT(fontTexture, "Failed to create imgui font texture");

    commandList->open();
    
    commandList->beginTrackingTextureState(fontTexture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
    commandList->writeTexture(fontTexture, 0, 0, pixels, width * 4);
    commandList->setPermanentTextureState(fontTexture, nvrhi::ResourceStates::ShaderResource);
    commandList->commitBarriers();

    commandList->close();
    device->executeCommandList(commandList);

    io.Fonts->TexID = (ImTextureID)fontTexture.Get();

    return true;
}

bool ImGui_NVRHI::Init(nvrhi::IDevice *device, Ref<ShaderFactory> shaderFactory)
{
    this->device = device;
    commandList = device->createCommandList();

    vertexShader = shaderFactory->CreateAutoShader("imgui_vertex", "main", IGNITE_MAKE_PLATFORM_SHADER(g_imgui_vertex), nullptr, nvrhi::ShaderType::Vertex);
    pixelShader = shaderFactory->CreateAutoShader("imgui_pixel", "main", IGNITE_MAKE_PLATFORM_SHADER(g_imgui_vertex), nullptr, nvrhi::ShaderType::Pixel);
    
    if (!vertexShader || !pixelShader)
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

    attributeLayout = device->createInputLayout(vertexAttribLayout, std::size(vertexAttribLayout), vertexShader);

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

        bindingLayout = device->createBindingLayout(layoutDesc);
        graphicsPipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;
        graphicsPipelineDesc.inputLayout = attributeLayout;
        graphicsPipelineDesc.VS = vertexShader;
        graphicsPipelineDesc.PS = pixelShader;
        graphicsPipelineDesc.renderState = renderState;
        graphicsPipelineDesc.bindingLayouts = { bindingLayout };
    }

    {
        const auto desc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Wrap)
            .setAllFilters(true);

        fontSampler = device->createSampler(desc);

        LOG_ASSERT(fontSampler, "Failed to create ImGui font sampler");
        if (!fontSampler)
            return false;
    }

    return true;
}

bool ImGui_NVRHI::Render(nvrhi::IFramebuffer *framebuffer)
{
    ImDrawData *drawData = ImGui::GetDrawData();
    const ImGuiIO &io = ImGui::GetIO();

    commandList->open();
    commandList->beginMarker("ImGui");

    if (!UpdateGeometry(commandList))
    {
        commandList->close();
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
    vbufBinding.buffer = vertexBuffer;
    vbufBinding.slot = 0;
    vbufBinding.offset = 0;
    drawState.vertexBuffers.push_back(vbufBinding);

    drawState.indexBuffer.buffer = indexBuffer;
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

                commandList->setGraphicsState(drawState);
                commandList->setPushConstants(invDisplaySize, sizeof(invDisplaySize));
                commandList->drawIndexed(drawArguments);
            }
            idxOffset += pCmd->ElemCount;
        }
        vtxOffset += cmdList->VtxBuffer.Size;
    }

    commandList->endMarker();
    commandList->close();
    device->executeCommandList(commandList);

    return true;
}

void ImGui_NVRHI::BackBufferResizing()
{
    graphicsPipeline = nullptr;
}

bool ImGui_NVRHI::ReallocateBuffer(nvrhi::BufferHandle &buffer, size_t requiredSize, size_t reallocateSize, bool isIndexBuffer)
{
    if (buffer == nullptr || size_t(buffer->getDesc().byteSize) < requiredSize)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = static_cast<u32>(reallocateSize);
        desc.debugName = indexBuffer ? "ImGui index buffer" : "ImGui vertex buffer";
        desc.canHaveUAVs = false;
        desc.isVertexBuffer = !isIndexBuffer;
        desc.isIndexBuffer = isIndexBuffer;
        desc.isDrawIndirectArgs = false;
        desc.isVolatile = false;
        desc.initialState = isIndexBuffer ? nvrhi::ResourceStates::IndexBuffer : nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;

        buffer = device->createBuffer(desc);

        if (!buffer)
            return false;
    }

    return true;
}

nvrhi::IGraphicsPipeline *ImGui_NVRHI::GetPSO(nvrhi::IFramebuffer *framebuffer)
{
    if (graphicsPipeline)
        return graphicsPipeline;

    graphicsPipeline = device->createGraphicsPipeline(graphicsPipelineDesc, framebuffer);
    LOG_ASSERT(graphicsPipeline, "Failed to create ImGui PSO");

    
    return graphicsPipeline;
}

nvrhi::IBindingSet *ImGui_NVRHI::GetBindingSet(nvrhi::ITexture *texture)
{
    auto iter = bindingsCache.find(texture);
    if (iter != bindingsCache.end())
        return iter->second;
    
    nvrhi::BindingSetDesc desc;
    desc.bindings = 
    {
        nvrhi::BindingSetItem::PushConstants(0, sizeof(float) * 2),
        nvrhi::BindingSetItem::Texture_SRV(0, texture),
        nvrhi::BindingSetItem::Sampler(0, fontSampler)
    };

    nvrhi::BindingSetHandle binding;
    binding = device->createBindingSet(desc, bindingLayout);
    LOG_ASSERT(binding, "Failed to create ImGui binding set");

    bindingsCache[texture] = binding;
    return binding;
}

bool ImGui_NVRHI::UpdateGeometry(nvrhi::ICommandList *commandList)
{
    ImDrawData *drawData = ImGui::GetDrawData();

    if (!ReallocateBuffer(vertexBuffer, drawData->TotalVtxCount * sizeof(ImDrawVert),
        (drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert),
        false))
    {
        return false;
    }

    if (!ReallocateBuffer(indexBuffer, drawData->TotalIdxCount * sizeof(ImDrawIdx),
        (drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx),
        true))
    {
        return false;
    }

    imguiVertexBuffer.resize(vertexBuffer->getDesc().byteSize / sizeof(ImDrawVert));
    imguiIndexBuffer.resize(indexBuffer->getDesc().byteSize / sizeof(ImDrawIdx));

    ImDrawVert *vtxDst = &imguiVertexBuffer[0];
    ImDrawIdx *idxDst = &imguiIndexBuffer[0];

    for (i32 n = 0; n < drawData->CmdListsCount; ++n)
    {
        const ImDrawList *cmdList = drawData->CmdLists[n];
        memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

        vtxDst += cmdList->VtxBuffer.Size;
        idxDst += cmdList->IdxBuffer.Size;
    }

    commandList->writeBuffer(vertexBuffer, &imguiVertexBuffer[0], vertexBuffer->getDesc().byteSize);
    commandList->writeBuffer(indexBuffer, &imguiIndexBuffer[0], indexBuffer->getDesc().byteSize);

    return true;
}

void ImGui_NVRHI::Shutdown()
{
    fontTexture = nullptr;
    fontSampler = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;

    vertexShader = nullptr;
    pixelShader = nullptr;

    commandList = nullptr;
}