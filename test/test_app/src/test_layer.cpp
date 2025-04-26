#include "test_layer.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"

#include <nvrhi/utils.h>

namespace ignite
{
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
        s_data.vertexShader = Shader::Create(m_Device, "resources/shaders/glsl/test_2d.vert", ShaderStage_Vertex);
        s_data.pixelShader = Shader::Create(m_Device, "resources/shaders/glsl/test_2d.frag", ShaderStage_Fragment);

        // create binding layout

        // create input layout
        auto attributes = TestVertex::GetAttributes();
        s_data.inputLayout = m_Device->createInputLayout(attributes.data(), attributes.size(), s_data.vertexShader->GetHandle());
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
            .setVertexShader(s_data.vertexShader->GetHandle())
            .setPixelShader(s_data.pixelShader->GetHandle())
            .setRenderState(renderState);

        nvrhi::IFramebuffer *framebuffer = m_DeviceManager->GetCurrentFramebuffer();
        s_data.pipeline = m_Device->createGraphicsPipeline(pipelineDesc, framebuffer);

        // write buffer
        s_data.commandList = m_Device->createCommandList();

        s_data.commandList->open();

        s_data.commandList->writeBuffer(s_data.vertexBuffer, vertices.data(), sizeof(vertices));
        s_data.commandList->writeBuffer(s_data.indexBuffer, indices.data(), sizeof(indices));

        s_data.commandList->close();
        m_Device->executeCommandList(s_data.commandList);
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

        s_data.commandList->open();
        nvrhi::utils::ClearColorAttachment(s_data.commandList, framebuffer, 0, nvrhi::Color(0.1f, 0.1f, 0.1f, 1.0f));

        nvrhi::GraphicsState graphicsState = nvrhi::GraphicsState()
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport()))
            .setPipeline(s_data.pipeline)
            .setFramebuffer(framebuffer)
            .addVertexBuffer({ s_data.vertexBuffer, 0, 0 })
            .setIndexBuffer({ s_data.indexBuffer, nvrhi::Format::R32_UINT });

       s_data.commandList->setGraphicsState(graphicsState);

        nvrhi::DrawArguments args;
        args.vertexCount = u32(indices.size());
        args.instanceCount = 1;

        s_data.commandList->drawIndexed(args);

        s_data.commandList->close();
        m_Device->executeCommandList(s_data.commandList);
    }

    void TestLayer::OnGuiRender()
    {
    }
}