#include "environment.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/scene/camera.hpp"

#include <stb_image.h>

namespace ignite {

    // clock wise
    std::array<glm::vec3, 24> vertices = {

        glm::vec3( 1.0f,  1.0f,  1.0f), // top right    front  
        glm::vec3( 1.0f,  1.0f, -1.0f), // top right    back
        glm::vec3( 1.0f, -1.0f, -1.0f), // bottom right back
        glm::vec3( 1.0f, -1.0f,  1.0f), // bottom right front

        glm::vec3(-1.0f,  1.0f, -1.0f), // top    left back
        glm::vec3(-1.0f,  1.0f,  1.0f), // top    left front
        glm::vec3(-1.0f, -1.0f,  1.0f), // bottom left front
        glm::vec3(-1.0f, -1.0f, -1.0f), // bottom left back
        
        glm::vec3(-1.0f,  1.0f,  1.0f), // top left  front
        glm::vec3(-1.0f,  1.0f, -1.0f), // top left  back
        glm::vec3( 1.0f,  1.0f, -1.0f), // top right back
        glm::vec3( 1.0f,  1.0f,  1.0f), // top right front

        glm::vec3(-1.0f, -1.0f,  1.0f), // bottom left  front
        glm::vec3( 1.0f, -1.0f,  1.0f), // bottom right front
        glm::vec3( 1.0f, -1.0f, -1.0f), // bottom right back
        glm::vec3(-1.0f, -1.0f, -1.0f), // bottom left  back

        glm::vec3(-1.0f, -1.0f, -1.0f), // bottom left  back
        glm::vec3( 1.0f, -1.0f, -1.0f), // bottom right back
        glm::vec3( 1.0f,  1.0f, -1.0f), // top    right back
        glm::vec3(-1.0f,  1.0f, -1.0f), // top    left  back

        glm::vec3(-1.0f, -1.0f,  1.0f), // bottom left  front
        glm::vec3(-1.0f,  1.0f,  1.0f), // top    left  front
        glm::vec3( 1.0f,  1.0f,  1.0f), // top    right front
        glm::vec3( 1.0f, -1.0f,  1.0f), // bottom right front
    };

    Environment::Environment(nvrhi::IDevice *device, nvrhi::CommandListHandle commandList, const EnvironmentCreateInfo &createInfo, nvrhi::BindingLayoutHandle bindingLayout)
    {
        // create vertex buffer
        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = sizeof(vertices);
        vbDesc.isVertexBuffer = true;
        vbDesc.debugName = "Skybox vertex buffer";
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;

        m_VertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(m_VertexBuffer, "[Environment] Failed to create vertex buffer!");

        // create index buffer
        nvrhi::BufferDesc ibDesc;
        ibDesc.byteSize = sizeof(uint32_t) * 36;
        ibDesc.isIndexBuffer = true;
        ibDesc.debugName = "Skybox index buffer";
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;

        m_IndexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(m_IndexBuffer, "[Environment] Failed to create index buffer!");

        // create model projection buffer
        nvrhi::BufferDesc trbDesc;
        trbDesc.byteSize = sizeof(glm::mat4);
        trbDesc.isConstantBuffer = true;
        trbDesc.debugName = "Skybox transform constant buffer";
        trbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        trbDesc.keepInitialState = true;
        m_ConstantBuffer = device->createBuffer(trbDesc);
        LOG_ASSERT(m_ConstantBuffer, "[Environment] Failed to create constant buffer!");

        
        // create constant buffer
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = sizeof(EnvParamsConstant);
        cbDesc.isConstantBuffer = true;
        cbDesc.debugName = "Skybox params constant buffer";
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        cbDesc.keepInitialState = true;

        m_ParamsConstantBuffer = device->createBuffer(cbDesc);
        LOG_ASSERT(m_ParamsConstantBuffer, "[Environment] Failed to create constant buffer!");


        // create texture
        const auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::TextureCube)
            .setWidth(480) // hardcoded
            .setHeight(480) // harcoded
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setArraySize(6)
            .setMipLevels(1)
            .setDebugName("CubemapTexture");

        m_CubeTexture = device->createTexture(textureDesc);
        LOG_ASSERT(m_CubeTexture, "[Environment] Failed to create texture!");

        // create sampler
        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);
        m_Sampler = device->createSampler(samplerDesc);
        LOG_ASSERT(m_Sampler, "[Environment] Failed to create sampler!");

        commandList->open();

        // order +X, -X, +Y, -Y, +Z, -Z.
        for (int face = 0; face < 6; ++face)
        {
            const char *filepath = ((const char **)&createInfo)[face];

            int width, height, channels;

            uint8_t *srcData = stbi_load(filepath, &width, &height, &channels, 4);

            LOG_ASSERT(srcData, "Failed to load image for cubemap face");

            // Define region for one face (array slice)
            nvrhi::TextureSlice slice;
            slice.arraySlice = face;
            slice.mipLevel = 0;

            commandList->writeTexture(m_CubeTexture, slice.arraySlice, slice.mipLevel, srcData, width * 4, width * height * 4);

            stbi_image_free(srcData);
        }

        // create binding set
        nvrhi::BindingSetDesc bsDesc;
        bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_ConstantBuffer));
        bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, m_ParamsConstantBuffer));
        bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, m_CubeTexture));
        bsDesc.addItem(nvrhi::BindingSetItem::Sampler(0, m_Sampler));

        m_BindingSet = device->createBindingSet(bsDesc, bindingLayout);
        LOG_ASSERT(m_BindingSet, "Failed to create binding set");

        // write buffers
        {
            // vertex buffer
            commandList->writeBuffer(m_VertexBuffer, vertices.data(), sizeof(vertices));

            u32 *indices = new u32[36];
            u32 Offset = 0;
            for (u32 i = 0; i < 36; i += 6)
            {
                indices[i + 0] = Offset + 0;
                indices[i + 1] = Offset + 1;
                indices[i + 2] = Offset + 2;

                indices[i + 3] = Offset + 2;
                indices[i + 4] = Offset + 3;
                indices[i + 5] = Offset + 0;

                Offset += 4;
            }

            // index buffer
            commandList->writeBuffer(m_IndexBuffer, indices, sizeof(uint32_t) * 36);

            delete[] indices;
        }

        commandList->close();
        device->executeCommandList(commandList);
    }

    void Environment::Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera)
    {
        glm::mat4 viewProjection = camera->GetViewProjectionMatrix();
        commandList->writeBuffer(m_ConstantBuffer, glm::value_ptr(viewProjection), sizeof(glm::mat4));
        
        // write params buffer
        
        static float t = 0.0f;
        t += 0.001f;
        EnvParamsConstant params;
        params.exposure = glm::abs(glm::sin(t));

        commandList->writeBuffer(m_ParamsConstantBuffer, &params, sizeof(EnvParamsConstant));

        // render
        auto state = nvrhi::GraphicsState();
        state.pipeline = pipeline;
        state.framebuffer = framebuffer;
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport());
        state.addVertexBuffer({ m_VertexBuffer, 0, 0 });
        state.indexBuffer = { m_IndexBuffer, nvrhi::Format::R32_UINT };

        if (m_BindingSet != nullptr)
            state.addBindingSet(m_BindingSet);

        commandList->setGraphicsState(state);

        nvrhi::DrawArguments args;
        args.setVertexCount(36);
        args.instanceCount = 1;

        commandList->drawIndexed(args);
    }

    nvrhi::VertexAttributeDesc Environment::GetAttribute()
    {
        return nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(0)
            .setElementStride(sizeof(glm::vec3));
    }

    nvrhi::BindingLayoutDesc Environment::GetBindingLayoutDesc()
    {
        return nvrhi::BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::All)
            .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(0))
            .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(1))
            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
            .addItem(nvrhi::BindingLayoutItem::Sampler(0));
    }

}