#include "test_layer.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"

#include <nvrhi/utils.h>

namespace ignite
{
    struct TestVertex
    {
        glm::vec2 position;
        glm::vec4 color;

        static std::array<nvrhi::VertexAttributeDesc, 2> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setBufferIndex(0)
                    .setFormat(nvrhi::Format::RG32_FLOAT)
                    .setOffset(offsetof(TestVertex, position))
                    .setElementStride(sizeof(TestVertex)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA32_FLOAT)
                    .setOffset(offsetof(TestVertex, color))
                    .setElementStride(sizeof(TestVertex))
            };
        }
    };

    static std::array<TestVertex, 3> vertices
    {
        TestVertex{{ -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        TestVertex{{  0.0f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        TestVertex{{  0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f } }
    };

    static std::array<u32, 3> indices
    {
        0, 1, 2
    };

    TestLayer::TestLayer(const std::string &name)
        : Layer(name)
        , m_DeviceManager(nullptr)
        , m_Device(nullptr)
    {
    }

    void TestLayer::OnAttach()
    {
        m_DeviceManager = Application::GetDeviceManager();
        m_Device = m_DeviceManager->GetDevice();

        // create shader factory
        std::filesystem::path shaderPath = ("resources" + GetShaderFolder(m_DeviceManager->GetGraphicsAPI()));
        Ref<vfs::NativeFileSystem> nativeFS = CreateRef<vfs::NativeFileSystem>();
        m_ShaderFactory = CreateRef<ShaderFactory>(m_Device, nativeFS, shaderPath);

        // create shader
        vertexShader = Shader::Create(m_Device, "resources/shaders/test.vertex.hlsl", ShaderStage_Vertex);
        pixelShader = Shader::Create(m_Device, "resources/shaders/test.pixel.hlsl", ShaderStage_Fragment);

        // create binding layout

        // create input layout
        auto attributes = TestVertex::GetAttributes();
        inputLayout = m_Device->createInputLayout(attributes.data(), attributes.size(), vertexShader->GetHandle());
        LOG_ASSERT(inputLayout, "Failed to create input layout");

        // create buffers
        auto vertexBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(vertices))
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Vertex buffer");
        vertexBuffer = m_Device->createBuffer(vertexBufferDesc);
        LOG_ASSERT(vertexBuffer, "Failed to create vertex buffer");

        auto indexBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(indices))
            .setIsIndexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Index buffer");
        indexBuffer = m_Device->createBuffer(indexBufferDesc);
        LOG_ASSERT(indexBuffer, "Failed to create index buffer");

        // create graphics pipeline

        auto blendState = nvrhi::BlendState();
        blendState.targets[0].blendEnable = true;

        auto depthStencilState = nvrhi::DepthStencilState()
            .setDepthWriteEnable(false)
            .setDepthTestEnable(false);

        auto rasterState = nvrhi::RasterState()
            .setCullNone()
            .setFillSolid();

        auto renderState = nvrhi::RenderState()
            .setBlendState(blendState)
            .setDepthStencilState(depthStencilState)
            .setRasterState(rasterState);

        auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
            .setInputLayout(inputLayout)
            .setPrimType(nvrhi::PrimitiveType::TriangleList)
            .setVertexShader(vertexShader->GetHandle())
            .setPixelShader(pixelShader->GetHandle())
            .setRenderState(renderState);

        nvrhi::IFramebuffer *framebuffer = m_DeviceManager->GetCurrentFramebuffer();
        pipeline = m_Device->createGraphicsPipeline(pipelineDesc, framebuffer);

        // write buffer
        commandList = m_Device->createCommandList();

        commandList->open();

        commandList->writeBuffer(vertexBuffer, vertices.data(), sizeof(vertices));
        commandList->writeBuffer(indexBuffer, indices.data(), sizeof(indices));

        commandList->close();
        m_Device->executeCommandList(commandList);
    }

    void TestLayer::OnDetach()
    {
    }

    void TestLayer::OnUpdate(f32 deltaTime)
    {
    }

    void TestLayer::OnEvent(Event& e)
    {
    }

    void TestLayer::OnRender(nvrhi::IFramebuffer *framebuffer)
    {
        Layer::OnRender(framebuffer);

        commandList->open();
        nvrhi::utils::ClearColorAttachment(commandList, framebuffer, 0, nvrhi::Color(0.1f, 0.1f, 0.1f, 1.0f));

        nvrhi::GraphicsState graphicsState = nvrhi::GraphicsState()
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport()))
            .setPipeline(pipeline)
            .setFramebuffer(framebuffer)
            .addVertexBuffer({ vertexBuffer, 0, 0 })
            .setIndexBuffer({ indexBuffer, nvrhi::Format::R32_UINT });

       commandList->setGraphicsState(graphicsState);

        nvrhi::DrawArguments args;
        args.vertexCount = u32(indices.size());
        args.instanceCount = 1;

        commandList->drawIndexed(args);

        commandList->close();
        m_Device->executeCommandList(commandList);
    }

    void TestLayer::OnGuiRender()
    {
        ImGui::ShowDemoWindow();
    }
}