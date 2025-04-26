#include "test_layer.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/device/device_manager.hpp"

#include <nvrhi/utils.h>

namespace ignite
{
    struct RenderData
    {
        nvrhi::ShaderHandle vertexShader;
        nvrhi::ShaderHandle pixelShader;

        nvrhi::InputLayoutHandle inputLayout;
        nvrhi::BindingLayoutHandle bindingLayout;

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;

        nvrhi::GraphicsPipelineDesc pipelineDesc;
        nvrhi::GraphicsPipelineHandle pipeline;
    };

    static RenderData s_data;

    struct TestVertex
    {
        glm::vec3 position;
        glm::vec3 color;

        static std::array<nvrhi::VertexAttributeDesc, 2> GetAttributes()
        {
            return
            {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setBufferIndex(0)
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(TestVertex, position))
                    .setElementStride(sizeof(TestVertex)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(TestVertex, color))
                    .setElementStride(sizeof(TestVertex))
            };
        }
    };

    static std::array<TestVertex, 3> vertices
    {
        TestVertex{{ -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        TestVertex{{  0.0f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        TestVertex{{  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 1.0f } }
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
        s_data.vertexShader = m_ShaderFactory->CreateShader("test_2d_vert", "main", nullptr, nvrhi::ShaderType::Vertex);
        s_data.pixelShader = m_ShaderFactory->CreateShader("test_2d_pixel", "main", nullptr, nvrhi::ShaderType::Pixel);
        LOG_ASSERT(s_data.vertexShader && s_data.pixelShader, "Failed to create shader");

        // create binding layout

        // create input layout
        auto attributes = TestVertex::GetAttributes();
        s_data.inputLayout = m_Device->createInputLayout(attributes.data(), attributes.size(), s_data.vertexShader);
        LOG_ASSERT(s_data.inputLayout, "Failed to create input layout");

        // create buffers
        auto vertexBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(vertices))
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Vertex buffer");
        s_data.vertexBuffer = m_Device->createBuffer(vertexBufferDesc);
        LOG_ASSERT(s_data.vertexBuffer, "Failed to create vertex buffer");

        auto indexBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(indices))
            .setIsIndexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Index buffer");
        s_data.indexBuffer = m_Device->createBuffer(indexBufferDesc);
        LOG_ASSERT(s_data.indexBuffer, "Failed to create index buffer");

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
            .setInputLayout(s_data.inputLayout)
            .setPrimType(nvrhi::PrimitiveType::TriangleList)
            .setVertexShader(s_data.vertexShader)
            .setPixelShader(s_data.pixelShader)
            .setRenderState(renderState);

        nvrhi::IFramebuffer *framebuffer = m_DeviceManager->GetCurrentFramebuffer();
        s_data.pipeline = m_Device->createGraphicsPipeline(pipelineDesc, framebuffer);

        // write buffer
        m_CommandList = m_Device->createCommandList();

        m_CommandList->open();

        m_CommandList->writeBuffer(s_data.vertexBuffer, vertices.data(), sizeof(vertices));
        m_CommandList->writeBuffer(s_data.indexBuffer, indices.data(), sizeof(indices));

        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void TestLayer::OnDetach()
    {
        s_data.vertexBuffer.Reset();
        s_data.indexBuffer.Reset();
        s_data.pixelShader.Reset();
        s_data.vertexShader.Reset();
        s_data.pipeline.Reset();
        s_data.inputLayout.Reset();
        s_data.bindingLayout.Reset();
        m_CommandList.Reset();
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

        m_CommandList->open();
        nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.1f, 0.1f, 0.1f, 1.0f));

        nvrhi::GraphicsState graphicsState = nvrhi::GraphicsState()
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport()))
            .setPipeline(s_data.pipeline)
            .setFramebuffer(framebuffer)
            .addVertexBuffer({ s_data.vertexBuffer, 0, 0 })
            .setIndexBuffer({ s_data.indexBuffer, nvrhi::Format::R32_UINT });

       m_CommandList->setGraphicsState(graphicsState);

        nvrhi::DrawArguments args;
        args.vertexCount = u32(indices.size());
        args.instanceCount = 1;

        m_CommandList->drawIndexed(args);

        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void TestLayer::OnGuiRender()
    {
    }
}