#include "ignite/ignite.hpp"
#include "editor_layer.hpp"
#include "imgui_internal.h"
#include "scene_panel.hpp"

#include <stb_image.h>

#include "ignite/graphics/renderer_2d.hpp"

struct PushConstants
{
    glm::mat4 mvp;
} constants;

struct Vertex2D
{
    glm::vec2 position;
    glm::vec2 texCoord;
    glm::vec4 color;

    static std::array<nvrhi::VertexAttributeDesc, 3> GetAttributes()
    {
        return
        {
            nvrhi::VertexAttributeDesc()
                .setName("POSITION")
                .setFormat(nvrhi::Format::RG32_FLOAT)
                .setOffset(offsetof(Vertex2D, position))
                .setElementStride(sizeof(Vertex2D)),
            nvrhi::VertexAttributeDesc()
                .setName("TEXCOORD")
                .setFormat(nvrhi::Format::RG32_FLOAT)
                .setOffset(offsetof(Vertex2D, texCoord))
                .setElementStride(sizeof(Vertex2D)),
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::RGBA32_FLOAT)
                .setOffset(offsetof(Vertex2D, color))
                .setElementStride(sizeof(Vertex2D))
        };
    }
};

Vertex2D quadVertices[] = {
    { {-0.5f, -0.5f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }, // bottom left
    { { 0.5f,  0.5f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }, // top right
    { {-0.5f,  0.5f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }, // top left
    { { 0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} }, // bottom right
};

u32 indices[] = { 
    0, 1, 2, 
    0, 3, 1
};

glm::vec3 cameraPosition = { 0.0f, 0.0f, 1.0f };
glm::vec3 modelPosition;

IgniteEditorLayer::IgniteEditorLayer(const std::string &name)
    : Layer(name)
{
}

void IgniteEditorLayer::OnAttach()
{
    Layer::OnAttach();

    m_DeviceManager = Application::GetDeviceManager();
    nvrhi::IDevice *device = m_DeviceManager->GetDevice();
#if 0
    // create shaders
    sceneGfx.vertexShader = Application::GetShaderFactory()->CreateShader("default_2d_vertex", "main", nullptr, nvrhi::ShaderType::Vertex);
    sceneGfx.pixelShader = Application::GetShaderFactory()->CreateShader("default_2d_pixel", "main", nullptr, nvrhi::ShaderType::Pixel);
    LOG_ASSERT(sceneGfx.vertexShader && sceneGfx.pixelShader, "Failed to create shader");

    // vertex vertex attributes
    const auto attributes = Vertex2D::GetAttributes();
    sceneGfx.inputLayout = device->createInputLayout(attributes.data(), attributes.size(), sceneGfx.vertexShader);
    
    // create binding layout
    auto layoutDesc = nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All)
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0))
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1));

    sceneGfx.bindingLayout = device->createBindingLayout(layoutDesc);

    // create constant buffer
    auto constantBufferDesc = nvrhi::BufferDesc()
        .setByteSize(sizeof(PushConstants))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(16);
    sceneGfx.constantBuffer = device->createBuffer(constantBufferDesc);
    LOG_ASSERT(sceneGfx.constantBuffer, "Failed to create constant buffer");

     // create texture

    i32 width, height, channels;
    stbi_uc *pixelData = stbi_load("Resources/textures/test.png", &width, &height, &channels, 4);
    LOG_ASSERT(pixelData, "Failed to write pixel data to texture");

    auto textureDesc = nvrhi::TextureDesc()
        .setDimension(nvrhi::TextureDimension::Texture2D)
        .setWidth(width)
        .setHeight(height)
        .setFormat(nvrhi::Format::RGBA8_UNORM)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setDebugName("Geometry Texture");

    sceneGfx.texture = device->createTexture(textureDesc);
    LOG_ASSERT(sceneGfx.texture, "Failed to create texture");

    // create sampler
    auto samplerDesc = nvrhi::SamplerDesc()
        .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
        .setAllFilters(true);

    sceneGfx.sampler = device->createSampler(samplerDesc);
    LOG_ASSERT(sceneGfx.sampler, "Failed to create texture");

    // create binding set
    auto bindingSetDesc = nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, sceneGfx.texture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, sceneGfx.sampler))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, sceneGfx.constantBuffer));

    sceneGfx.bindingSet = device->createBindingSet(bindingSetDesc, sceneGfx.bindingLayout);
    LOG_ASSERT(sceneGfx.bindingSet, "Failed to create binding");

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
            .addBindingLayout(sceneGfx.bindingLayout)
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

    if (pixelData)
    {
        size_t rowPitchSize = width * channels;
        m_CommandList->writeTexture(sceneGfx.texture, 0, 0, pixelData, rowPitchSize);
        LOG_ASSERT(sceneGfx.texture, "Failed to write pixel data to texture");
    }
    

    m_CommandList->close();
    m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);
#endif

    // write buffer with command list
    m_CommandList = m_DeviceManager->GetDevice()->createCommandList();
    Renderer2D::InitQuadData(m_DeviceManager->GetDevice(), m_CommandList);

    m_ScenePanel = IPanel::Create<ScenePanel>("Scene Panel");
    m_ScenePanel->CreateRenderTarget(device, 1280.0f, 720.0f);
}

void IgniteEditorLayer::OnDetach()
{
    Layer::OnDetach();

    sceneGfx.vertexBuffer = nullptr;
    sceneGfx.indexBuffer = nullptr;
    sceneGfx.pipeline = nullptr;
    sceneGfx.vertexShader = nullptr;
    sceneGfx.pixelShader = nullptr;

    m_CommandList = nullptr;
}

void IgniteEditorLayer::OnUpdate(const f32 deltaTime)
{
    Layer::OnUpdate(deltaTime);

    // update transformation
    m_ScenePanel->OnUpdate(deltaTime);

    constants.mvp = m_ScenePanel->GetViewportCamera()->GetViewProjectionMatrix() * glm::translate(glm::mat4(1.0f), modelPosition);
}

void IgniteEditorLayer::OnEvent(Event &e)
{
    Layer::OnEvent(e);
    m_ScenePanel->OnEvent(e);
}

void IgniteEditorLayer::OnRender(nvrhi::IFramebuffer *framebuffer)
{
    Layer::OnRender(framebuffer);

    // main scene rendering
    m_CommandList->open();
    m_CommandList->clearTextureFloat(m_ScenePanel->GetRT()->texture,
                nvrhi::AllSubresources, nvrhi::Color(
                    m_ScenePanel->GetRT()->clearColor.r,
                    m_ScenePanel->GetRT()->clearColor.g,
                    m_ScenePanel->GetRT()->clearColor.b,
                    1.0f)
                );

    nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

    Renderer2D::Begin(m_ScenePanel->GetViewportCamera());

    i32 gridSize = 20;
    for (i32 y = -gridSize; y < gridSize; y++)
    {
        for (i32 x = -gridSize; x < gridSize; x++)
        {
            f32 xPos = x + (x * 0.25f);
            f32 yPos = y + (y * 0.25f);

            // Normalize x and y to range [0, 1]
            f32 red   = (x + 10) / 20.0f; // x in [-10,10] -> [0,1]
            f32 blue  = (y + 10) / 20.0f; // y in [-10,10] -> [0,1]
            f32 green = 1.0f - glm::distance(m_ScenePanel->GetViewportCamera()->position, glm::vec3(xPos, yPos, 0.0f)) / 20.0f;

            glm::vec4 color = { red, green, blue, 1.0f };

            Renderer2D::DrawQuad({ xPos, yPos, 0.0f }, { 1.0f, 1.0f }, color);
        }
    }

    Renderer2D::Flush(m_ScenePanel->GetRT()->framebuffer);
    Renderer2D::End();

    m_CommandList->close();
    m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);
    {
#if 0
        // render
        m_CommandList->open();

        m_CommandList->clearTextureFloat(m_ScenePanel->GetRT()->texture,
            nvrhi::AllSubresources, nvrhi::Color(
                m_ScenePanel->GetRT()->clearColor.r,
                m_ScenePanel->GetRT()->clearColor.g,
                m_ScenePanel->GetRT()->clearColor.b,
                1.0f)
            );

        nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));


        // fill constant buffer
        m_CommandList->writeBuffer(sceneGfx.constantBuffer, &constants, sizeof(constants));

        const nvrhi::Viewport viewport = m_ScenePanel->GetRT()->framebuffer->getFramebufferInfo().getViewport();

        const auto graphicsState = nvrhi::GraphicsState()
            .setPipeline(sceneGfx.pipeline)
            .setFramebuffer(m_ScenePanel->GetRT()->framebuffer)
            .addBindingSet(sceneGfx.bindingSet)
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
            .addVertexBuffer( nvrhi::VertexBufferBinding{sceneGfx.vertexBuffer.Get(), 0, 0} )
            .setIndexBuffer({ sceneGfx.indexBuffer, nvrhi::Format::R32_UINT });

        m_CommandList->setGraphicsState(graphicsState);

        // draw geometry
        auto drawArguments = nvrhi::DrawArguments();
        drawArguments.vertexCount = std::size(indices);

        m_CommandList->drawIndexed(drawArguments);

        // close and execute the command list
        m_CommandList->close();
        m_DeviceManager->GetDevice()->executeCommandList(m_CommandList);
#endif
    }
}

void IgniteEditorLayer::OnGuiRender()
{
    constexpr f32 TITLE_BAR_HEIGHT = 45.0f;

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin("##main_dockspace", nullptr, windowFlags);
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 minPos = viewport->Pos;
    const ImVec2 maxPos = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + TITLE_BAR_HEIGHT);
    draw_list->AddRectFilled(minPos, maxPos, IM_COL32(40, 40, 40, 255));

    // dockspace
    ImGui::SetCursorScreenPos({viewport->Pos.x, viewport->Pos.y + TITLE_BAR_HEIGHT});
    ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    {
        // scene dockspace
        m_ScenePanel->OnGuiRender();
    }

    ImGui::End(); // end dockspace
}
