#include "mesh.hpp"

#include "environment.hpp"

namespace ignite
{
    void Mesh::CreateBuffers()
    {
        nvrhi::IDevice *device = Application::GetDeviceManager()->GetDevice();

        // create vertex buffer
        nvrhi::BufferDesc vbDesc = nvrhi::BufferDesc();
        vbDesc.isVertexBuffer = true;
        vbDesc.byteSize = sizeof(VertexMesh) * data.vertices.size();
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;
        vbDesc.debugName = "[Mesh] vertex buffer";

        vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(vertexBuffer, "[Mesh] Failed to create Vertex Buffer");

        // create index buffer
        nvrhi::BufferDesc ibDesc = nvrhi::BufferDesc();
        ibDesc.isIndexBuffer = true;
        ibDesc.byteSize = sizeof(uint32_t) * data.indices.size();
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;
        ibDesc.debugName = "[Mesh] index buffer";

        indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(indexBuffer, "[Mesh] Failed to create Index Buffer");

        // Constant buffer
        auto constantBufferDesc = nvrhi::BufferDesc()
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(16)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer);

        // create per mesh constant buffers
        constantBufferDesc.setDebugName("Mesh constant buffer");
        constantBufferDesc.setByteSize(sizeof(ObjectBuffer));
        objectBufferHandle = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(objectBufferHandle, "[Model] Failed to create object constant buffer");

        constantBufferDesc.setDebugName("Material constant buffer");
        constantBufferDesc.setByteSize(sizeof(MaterialData));
        materialBufferHandle = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(materialBufferHandle, "[Model] Failed to create material constant buffer");
    }
    
    void Mesh::CreateBindingSet()
    {
        nvrhi::IDevice *device = Application::GetRenderDevice();

        auto desc = nvrhi::BindingSetDesc();
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, Renderer::GetCameraBufferHandle()));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, objectBufferHandle));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, environment->GetDirLightBuffer()));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(3, environment->GetParamsBuffer()));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(4, materialBufferHandle));

        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, material.textures[aiTextureType_BASE_COLOR].handle ? material.textures[aiTextureType_BASE_COLOR].handle : Renderer::GetWhiteTexture()->GetHandle()));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(1, material.textures[aiTextureType_SPECULAR].handle ? material.textures[aiTextureType_SPECULAR].handle : Renderer::GetWhiteTexture()->GetHandle()));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(2, material.textures[aiTextureType_EMISSIVE].handle ? material.textures[aiTextureType_EMISSIVE].handle : Renderer::GetWhiteTexture()->GetHandle()));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(3, material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle ? material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle : Renderer::GetWhiteTexture()->GetHandle()));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(4, material.textures[aiTextureType_NORMALS].handle ? material.textures[aiTextureType_NORMALS].handle : Renderer::GetWhiteTexture()->GetHandle()));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(5, environment->GetHDRTexture()));
        desc.addItem(nvrhi::BindingSetItem::Sampler(0, material.sampler ? material.sampler : Renderer::GetWhiteTexture()->GetSampler()));

        auto newBindingSet = device->createBindingSet(desc, Renderer::GetBindingLayout(GPipeline::MESH));
        LOG_ASSERT(newBindingSet, "Failed to create binding set");

        if (newBindingSet)
        {
            bindingSets[GPipeline::MESH] = newBindingSet;
        }
    }

    void Mesh::WriteBuffers(uint32_t entityID)
    {
        nvrhi::IDevice *device = Application::GetRenderDevice();
        nvrhi::CommandListHandle commandList = device->createCommandList();

        commandList->open();

        commandList->writeBuffer(indexBuffer, data.indices.data(), sizeof(uint32_t) * data.indices.size());

        // default
        for (auto &vertex : data.vertices)
        {
            vertex.entityID = entityID;
        }
        commandList->writeBuffer(vertexBuffer, data.vertices.data(), sizeof(VertexMesh) * data.vertices.size());

        // write textures
        if (material.ShouldWriteTexture())
        {
            material.WriteBuffer(commandList);
        }

        commandList->close();
        device->executeCommandList(commandList);
    }

    void Mesh::UpdateTexture(Ref<Texture> texture, aiTextureType type)
    {
        if (!texture)
            return;

        if (type == aiTextureType_BASE_COLOR)
        {
            material.sampler = texture->GetSampler();
        }

        material.textures[type].handle = texture->GetHandle();
        CreateBindingSet();
    }
}
