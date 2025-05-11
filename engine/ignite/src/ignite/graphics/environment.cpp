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

    Environment::Environment(const EnvironmentCreateInfo &createInfo)
        : m_CreateInfo(createInfo)
    {
        // create vertex buffer
        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = sizeof(vertices);
        vbDesc.isVertexBuffer = true;
        vbDesc.debugName = "Skybox vertex buffer";
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;

        m_VertexBuffer = m_CreateInfo.device->createBuffer(vbDesc);
        LOG_ASSERT(m_VertexBuffer, "[Environment] Failed to create vertex buffer!");

        // create index buffer
        nvrhi::BufferDesc ibDesc;
        ibDesc.byteSize = sizeof(uint32_t) * 36;
        ibDesc.isIndexBuffer = true;
        ibDesc.debugName = "Skybox index buffer";
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;

        m_IndexBuffer = m_CreateInfo.device->createBuffer(ibDesc);
        LOG_ASSERT(m_IndexBuffer, "[Environment] Failed to create index buffer!");

        // create model projection buffer
        nvrhi::BufferDesc trbDesc;
        trbDesc.byteSize = sizeof(CameraBuffer);
        trbDesc.isConstantBuffer = true;
        trbDesc.isVolatile = true;
        trbDesc.debugName = "Skybox transform constant buffer";
        trbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        trbDesc.keepInitialState = true;
        trbDesc.maxVersions = 16;
        m_CameraConstantBuffer = m_CreateInfo.device->createBuffer(trbDesc);
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

        m_ParamsConstantBuffer = m_CreateInfo.device->createBuffer(cbDesc);
        LOG_ASSERT(m_ParamsConstantBuffer, "[Environment] Failed to create constant buffer!");

        CreateLightingBuffer();

        m_CreateInfo.commandList->open();

        // write buffers
        {
            // vertex buffer
            m_CreateInfo.commandList->writeBuffer(m_VertexBuffer, vertices.data(), sizeof(vertices));

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
            m_CreateInfo.commandList->writeBuffer(m_IndexBuffer, indices, sizeof(uint32_t) * 36);

            delete[] indices;
        }

        m_CreateInfo.commandList->close();
        m_CreateInfo.device->executeCommandList(m_CreateInfo.commandList);
    }

    void Environment::Render(nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera)
    {
        CameraBuffer camPushConstant;
        camPushConstant.viewProjection = camera->GetViewProjectionMatrix();
        camPushConstant.position = glm::vec4(camera->position, 1.0f);
        m_CreateInfo.commandList->writeBuffer(m_CameraConstantBuffer, &camPushConstant, sizeof(CameraBuffer));
        
        // write params buffer
        m_CreateInfo.commandList->writeBuffer(m_ParamsConstantBuffer, &params, sizeof(EnvironmentParams));
        m_CreateInfo.commandList->writeBuffer(m_DirLightConstantBuffer, &dirLight, sizeof(DirLight));

        // render
        auto state = nvrhi::GraphicsState();
        state.pipeline = pipeline;
        state.framebuffer = framebuffer;
        state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport());
        state.addVertexBuffer({ m_VertexBuffer, 0, 0 });
        state.indexBuffer = { m_IndexBuffer, nvrhi::Format::R32_UINT };

        if (m_BindingSet != nullptr)
            state.addBindingSet(m_BindingSet);

        m_CreateInfo.commandList->setGraphicsState(state);

        nvrhi::DrawArguments args;
        args.setVertexCount(36);
        args.instanceCount = 1;

        m_CreateInfo.commandList->drawIndexed(args);
    }

    void Environment::LoadTexture(const std::string &filepath)
    {
        TextureCreateInfo textureCI;
        textureCI.device = m_CreateInfo.device;
        textureCI.dimension = nvrhi::TextureDimension::Texture2D;
        textureCI.format = nvrhi::Format::RGBA32_FLOAT;
        textureCI.flip = true; // usually HDR textures are flipped

        m_HDRTexture = Texture::Create(filepath, textureCI);

        // create binding set after load the texture
        nvrhi::BindingSetDesc bsDesc;
        bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_CameraConstantBuffer));
        bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, m_ParamsConstantBuffer));
        bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, m_HDRTexture->GetHandle()));
        bsDesc.addItem(nvrhi::BindingSetItem::Sampler(0, m_HDRTexture->GetSampler()));

        m_BindingSet = m_CreateInfo.device->createBindingSet(bsDesc, m_CreateInfo.bindingLayout);
        LOG_ASSERT(m_BindingSet, "Failed to create binding set");
    }

    void Environment::WriteTexture()
    {
        m_CreateInfo.commandList->open();
        m_HDRTexture->Write(m_CreateInfo.commandList);
        m_CreateInfo.commandList->close();
        m_CreateInfo.device->executeCommandList(m_CreateInfo.commandList);
    }

    void Environment::SetSunDirection(float pitch, float yaw)
    {
        float pitchR = glm::radians(pitch); // elevation
        float yawR = glm::radians(yaw); // azimuth

        glm::vec3 dir;
        dir.x = cos(pitchR) * sin(yawR);
        dir.y = sin(pitchR);
        dir.z = cos(pitchR) * cos(yawR);

        dirLight.direction = glm::vec4(glm::normalize(dir), 0.0f);
    }

    Ref<Environment> Environment::Create(const EnvironmentCreateInfo &createInfo)
    {
        return CreateRef<Environment>(createInfo);
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

    void Environment::CreateLightingBuffer()
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(DirLight);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.debugName = "Directional light constant buffer";
        desc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        desc.keepInitialState = true;
        desc.maxVersions = 16;

        m_DirLightConstantBuffer = m_CreateInfo.device->createBuffer(desc);
        LOG_ASSERT(m_DirLightConstantBuffer, "[Environment] Failed to create directional light constant buffer!");
    }

}