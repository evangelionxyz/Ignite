#include "environment.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/scene/camera.hpp"
#include "vertex_data.hpp"

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
        trbDesc.byteSize = sizeof(PushConstantGlobal);
        trbDesc.isConstantBuffer = true;
        trbDesc.isVolatile = true;
        trbDesc.debugName = "Skybox transform constant buffer";
        trbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        trbDesc.keepInitialState = true;
        trbDesc.maxVersions = 16;
        m_CameraConstantBuffer = device->createBuffer(trbDesc);
        LOG_ASSERT(m_CameraConstantBuffer, "[Environment] Failed to create camera constant buffer!");

        // create constant buffer
        nvrhi::BufferDesc cbDesc;
        cbDesc.byteSize = sizeof(EnvironmentParams);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.debugName = "Skybox params constant buffer";
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        cbDesc.keepInitialState = true;
        cbDesc.maxVersions = 16;

        m_ParamsConstantBuffer = device->createBuffer(cbDesc);
        LOG_ASSERT(m_ParamsConstantBuffer, "[Environment] Failed to create constant buffer!");

        stbi_set_flip_vertically_on_load(true); // HDRs are typically flipped

        CreateLightingBuffer(device);

        int width, height, channels;
        float *hdrData = stbi_loadf(createInfo.filepath.generic_string().c_str(), 
            &width, 
            &height, 
            &channels, 
            4);

        LOG_ASSERT(hdrData, "[Environment] Failed to load HDR image");

        // create texture (HDR)
        const auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setWidth(width)
            .setHeight(height)
            .setFormat(nvrhi::Format::RGBA32_FLOAT)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("CubemapTexture");

        m_HDRTexture = device->createTexture(textureDesc);
        LOG_ASSERT(m_HDRTexture, "[Environment] Failed to create texture!");

        // create sampler
        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);
        m_Sampler = device->createSampler(samplerDesc);
        LOG_ASSERT(m_Sampler, "[Environment] Failed to create sampler!");

        commandList->open();

        // write texture
        commandList->writeTexture(m_HDRTexture, 0, 0, hdrData, width * sizeof(float) * 4, width * height * sizeof(float) * 4);
        stbi_image_free(hdrData);

        // create binding set
        nvrhi::BindingSetDesc bsDesc;
        bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_CameraConstantBuffer));
        bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, m_ParamsConstantBuffer));
        bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, m_HDRTexture));
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
        PushConstantGlobal camPushConstant;
        camPushConstant.viewProjection = camera->GetViewProjectionMatrix();
        camPushConstant.cameraPosition = glm::vec4(camera->position, 1.0f);
        commandList->writeBuffer(m_CameraConstantBuffer, &camPushConstant, sizeof(PushConstantGlobal));
        
        // write params buffer
        commandList->writeBuffer(m_ParamsConstantBuffer, &params, sizeof(EnvironmentParams));
        commandList->writeBuffer(m_DirLightConstantBuffer, &dirLight, sizeof(DirLight));

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
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1))
            .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
            .addItem(nvrhi::BindingLayoutItem::Sampler(0));
    }

    void Environment::CreateLightingBuffer(nvrhi::IDevice *device)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(DirLight);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.debugName = "Directional light constant buffer";
        desc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        desc.keepInitialState = true;
        desc.maxVersions = 16;

        m_DirLightConstantBuffer = device->createBuffer(desc);
        LOG_ASSERT(m_DirLightConstantBuffer, "[Environment] Failed to create directional light constant buffer!");
    }

}