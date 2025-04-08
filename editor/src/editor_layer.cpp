#include <editor_layer.hpp>
#include <scene_panel.hpp>

#include "imgui_internal.h"
#include "nvrhi/utils.h"

#define TEST false

// position, color
QuadVertex quadVertices[] = {
    { {-0.5f, -0.5f}, {1, 0, 1, 1} }, // bottom left
    { { 0.5f,  0.5f}, {1, 0, 1, 1} }, // top right
    { {-0.5f,  0.5f}, {1, 0, 1, 1} }, // top left
    { { 0.5f, -0.5f}, {1, 0, 0, 1} }, // bottom right
};

u32 indices[] = { 
    0, 1, 2, 
    0, 3, 1
};

IgniteEditorLayer::IgniteEditorLayer(const std::string &name)
    : Layer(name)
{
}

void IgniteEditorLayer::OnAttach()
{
    Layer::OnAttach();

    m_DeviceManager = Application::GetDeviceManager();
    nvrhi::IDevice *device = m_DeviceManager->GetDevice();

    // create shaders
    sceneGfx.vertexShader = Application::GetShaderFactory()->CreateShader("default_2d_vertex", "main", nullptr, nvrhi::ShaderType::Vertex);
    sceneGfx.pixelShader = Application::GetShaderFactory()->CreateShader("default_2d_pixel", "main", nullptr, nvrhi::ShaderType::Pixel);
    LOG_ASSERT(sceneGfx.vertexShader && sceneGfx.pixelShader, "Failed to create shader");

    // vertex vertex attributes
    nvrhi::VertexAttributeDesc attributes[] = 
    {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(QuadVertex, position))
            .setElementStride(sizeof(QuadVertex)),
        
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGBA32_FLOAT)
            .setOffset(offsetof(QuadVertex, color))
            .setElementStride(sizeof(QuadVertex)),
    };

    sceneGfx.inputLayout = device->createInputLayout(attributes, u32(std::size(attributes)), sceneGfx.vertexShader);

    // create vertex buffer
    auto vertexBufferDesc = nvrhi::BufferDesc()
        .setByteSize(sizeof(quadVertices))
        .setIsVertexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::VertexBuffer)
        .setKeepInitialState(true)
        .setDebugName("Vertex Buffer");
    
    sceneGfx.vertexBuffer = device->createBuffer(vertexBufferDesc);
    LOG_ASSERT(sceneGfx.vertexBuffer, "Failed to create vertex buffer");

    auto indexBufferDesc = nvrhi::BufferDesc()
        .setByteSize(sizeof(indices))
        .setIsIndexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::IndexBuffer)
        .setKeepInitialState(true)
        .setDebugName("Index Buffer");
    sceneGfx.indexBuffer = device->createBuffer(indexBufferDesc);
    LOG_ASSERT(sceneGfx.indexBuffer, "Failed to create index buffer");

    auto framebuffer = m_DeviceManager->GetCurrentFramebuffer();
    LOG_ASSERT(framebuffer, "Framebuffer is null");

    // create graphics pipeline
    if (!sceneGfx.pipeline)
    {
        nvrhi::BlendState blendState;
        blendState.targets[0].setBlendEnable(true);

        auto depthStencilState = nvrhi::DepthStencilState()
            .setDepthWriteEnable(false)
            .setDepthTestEnable(false);

        auto rasterState = nvrhi::RasterState()
            .setCullNone()
            .setFillSolid()
            .setMultisampleEnable(false);

        auto renderState = nvrhi::RenderState()
            .setRasterState(rasterState)
            .setDepthStencilState(depthStencilState)
            .setBlendState(blendState);

        auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
            .setInputLayout(sceneGfx.inputLayout)
            .setVertexShader(sceneGfx.vertexShader)
            .setPixelShader(sceneGfx.pixelShader)
            .setRenderState(renderState)
            .setPrimType(nvrhi::PrimitiveType::TriangleList);
        
        sceneGfx.pipeline = m_DeviceManager->GetDevice()->createGraphicsPipeline(pipelineDesc, framebuffer);
        LOG_ASSERT(sceneGfx.pipeline, "Failed to create graphics pipeline");
    }
    
    // write buffer with command list
    m_CommandList = m_DeviceManager->GetDevice()->createCommandList();
    m_CommandList->open();

    m_CommandList->writeBuffer(sceneGfx.vertexBuffer, quadVertices, sizeof(quadVertices));
    LOG_ASSERT(sceneGfx.vertexBuffer, "Failed to write vertices data to vertex buffer");

    m_CommandList->writeBuffer(sceneGfx.indexBuffer, indices, sizeof(indices));
    LOG_ASSERT(sceneGfx.indexBuffer, "Failed to write indices data to vertex buffer");

    m_CommandList->close();
    m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);

    m_ScenePanel = IPanel::Create<ScenePanel>("Scene Panel");
}

void IgniteEditorLayer::OnDetach()
{
    Layer::OnDetach();
}

void IgniteEditorLayer::OnUpdate(f32 deltaTime)
{
    Layer::OnUpdate(deltaTime);
}

void IgniteEditorLayer::OnEvent(Event &e)
{
    Layer::OnEvent(e);
}

void IgniteEditorLayer::OnRender(nvrhi::IFramebuffer *framebuffer)
{
    Layer::OnRender(framebuffer);
    {
        // render
        m_CommandList->open();
        nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0f));
        auto graphicsState = nvrhi::GraphicsState()
            .setPipeline(sceneGfx.pipeline)
            .setFramebuffer(framebuffer)
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport()))
            .addVertexBuffer( nvrhi::VertexBufferBinding{sceneGfx.vertexBuffer.Get(), 0, 0} )
            .setIndexBuffer({ sceneGfx.indexBuffer, nvrhi::Format::R32_UINT }); // REQUIRED

        m_CommandList->setGraphicsState(graphicsState);

        // draw geometry
        auto drawArguments = nvrhi::DrawArguments();
        drawArguments.vertexCount = std::size(indices);

        m_CommandList->drawIndexed(drawArguments);

        // close and execute the command list
        m_CommandList->close();
        m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);
    }
}

void IgniteEditorLayer::OnGuiRender()
{
    ImGui::Begin("Settings");
    ImGui::ColorEdit3("Clear Color", glm::value_ptr(m_ClearColor));
    ImGui::End();

    return;
    constexpr f32 TITLE_BAR_HEIGHT = 45.0f;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin("##main_dockspace", nullptr, window_flags);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 min_pos = viewport->Pos;
    ImVec2 max_pos = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + TITLE_BAR_HEIGHT);
    draw_list->AddRectFilled(min_pos, max_pos, IM_COL32(40, 40, 40, 255));

    // dockspace
    ImGui::SetCursorScreenPos({viewport->Pos.x, viewport->Pos.y + TITLE_BAR_HEIGHT});
    ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    {
        // scene dockspace
        m_ScenePanel->OnGuiRender();
    }

    ImGui::End(); // end dockspace

    ImGui::Begin("Settings");
    ImGui::ColorEdit3("Clear Color", glm::value_ptr(m_ClearColor));
    ImGui::End();
}

nvrhi::BufferHandle IgniteEditorLayer::CreateGeometryBuffer(nvrhi::ICommandList *commandList, const void *data,
    uint64_t dataSize)
{
    nvrhi::IDevice *device = m_DeviceManager->GetDevice();
    nvrhi::BufferHandle bufHandle;

    nvrhi::BufferDesc desc;
    desc.byteSize = dataSize;
    desc.isIndexBuffer = false;
    desc.canHaveRawViews = true;
    desc.structStride = 0;
    desc.initialState = nvrhi::ResourceStates::CopyDest;
    bufHandle = device->createBuffer(desc);

    if (data)
    {
        commandList->beginTrackingBufferState(bufHandle, nvrhi::ResourceStates::CopyDest);
        commandList->writeBuffer(bufHandle, data, dataSize);
        commandList->setPermanentBufferState(bufHandle, nvrhi::ResourceStates::ShaderResource);
    }

    return bufHandle;
}
