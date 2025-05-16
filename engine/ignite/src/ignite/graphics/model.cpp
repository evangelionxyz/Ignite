#include "model.hpp"

#include "ignite/core/logger.hpp"
#include "graphics_pipeline.hpp"
#include "renderer.hpp"
#include "texture.hpp"

namespace ignite {
    // Model class
    Model::Model(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo)
        : m_Filepath(filepath), m_CreateInfo(createInfo)
    {
        LOG_ASSERT(std::filesystem::exists(filepath), "[Mesh Loader] File does not exists!");
        const aiScene *scene = m_Importer.ReadFile(filepath.generic_string(), ASSIMP_IMPORTER_FLAGS);

        LOG_ASSERT(scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode,
            "[Model] Failed to load {}: {}", filepath, m_Importer.GetErrorString());

        if (scene->HasAnimations())
        {
            MeshLoader::LoadAnimation(scene, animations);

            // Process Skeleton
            MeshLoader::ExtractSkeleton(scene, skeleton);
            MeshLoader::SortJointsHierchically(skeleton);
        }

        meshes.resize(scene->mNumMeshes);
        for (size_t i = 0; i < meshes.size(); ++i)
        {
            meshes[i] = CreateRef<Mesh>();
        }

        MeshLoader::ProcessNode(scene, scene->mRootNode, filepath.generic_string(), meshes, nodes, skeleton, -1);
        MeshLoader::CalculateWorldTransforms(nodes, meshes);

        for (auto &mesh : meshes)
        {
            // create buffers
            mesh->CreateConstantBuffers(m_CreateInfo.device);
        }

        MeshLoader::ClearTextureCache();
    }

    void Model::SetEnvironmentTexture(nvrhi::TextureHandle envTexture)
    {
        m_EnvironmentTexture = envTexture;
    }

    void Model::CreateBindingSet(nvrhi::BindingLayoutHandle bindingLayout)
    {
        for (auto &mesh : meshes)
        {
            auto desc = nvrhi::BindingSetDesc();

            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_CreateInfo.cameraBuffer));
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, m_CreateInfo.lightBuffer));
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, m_CreateInfo.envBuffer));
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(3, mesh->objectBuffer));
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(4, mesh->materialBuffer));
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(5, m_CreateInfo.debugBuffer));

            desc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, mesh->material.textures[aiTextureType_DIFFUSE].handle));
            desc.addItem(nvrhi::BindingSetItem::Texture_SRV(1, mesh->material.textures[aiTextureType_SPECULAR].handle));
            desc.addItem(nvrhi::BindingSetItem::Texture_SRV(2, mesh->material.textures[aiTextureType_EMISSIVE].handle));
            desc.addItem(nvrhi::BindingSetItem::Texture_SRV(3, mesh->material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle));
            desc.addItem(nvrhi::BindingSetItem::Texture_SRV(4, mesh->material.textures[aiTextureType_NORMALS].handle));
            desc.addItem(nvrhi::BindingSetItem::Texture_SRV(5, m_EnvironmentTexture ? m_EnvironmentTexture : Renderer::GetBlackTexture()->GetHandle()));
            desc.addItem(nvrhi::BindingSetItem::Sampler(0, mesh->material.sampler));

            mesh->bindingSet = m_CreateInfo.device->createBindingSet(desc, bindingLayout);
            LOG_ASSERT(mesh->bindingSet, "Failed to create binding set");
        }
    }

    void Model::WriteBuffer(nvrhi::ICommandList *commandList)
    {
        if (m_BufferWritten)
            return;

        for (auto &mesh : meshes)
        {
            // m_CreateInfo.commandList->open();
            commandList->writeBuffer(mesh->vertexBuffer, mesh->vertices.data(), sizeof(VertexMesh) * mesh->vertices.size());
            commandList->writeBuffer(mesh->indexBuffer, mesh->indices.data(), sizeof(uint32_t) * mesh->indices.size());

            // write textures
            if (mesh->material.ShouldWriteTexture())
            {
                mesh->material.WriteBuffer(commandList);
            }
        }

        m_BufferWritten = true;
    }

    void Model::OnUpdate(f32 deltaTime)
    {
    }

    void Model::Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, const Ref<GraphicsPipeline> &pipeline)
    {
        nvrhi::Viewport viewport = framebuffer->getFramebufferInfo().getViewport();

        for (auto &mesh : meshes)
        {
            // write material constant buffer
            commandList->writeBuffer(mesh->materialBuffer, &mesh->material.data, sizeof(mesh->material.data));

            // write model constant buffer

            ObjectBuffer modelPushConstant;

            glm::mat4 meshTransform = mesh->worldTransform;

            if (activeAnimIndex >= 0 && animations[activeAnimIndex]->isPlaying && mesh->nodeID != -1)
            {
                auto it = skeleton.nameToJointMap.find(nodes[mesh->nodeID].name);
                if (it != skeleton.nameToJointMap.end())
                {
                    Joint &joint = skeleton.joints[it->second];
                    meshTransform = joint.globalTransform * joint.inverseBindPose * mesh->worldTransform;
                }
            }

            modelPushConstant.transformation = transform * meshTransform;

            const size_t numBones = std::min(boneTransforms.size(), static_cast<size_t>(MAX_BONES));
            for (size_t i = 0; i < numBones; ++i)
            {
                modelPushConstant.boneTransforms[i] = boneTransforms[i];
            }

            // Set remaining transforms to identity
            for (size_t i = numBones; i < MAX_BONES; ++i)
            {
                modelPushConstant.boneTransforms[i] = glm::mat4(1.0f);
            }

            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(modelPushConstant.transformation)));
            modelPushConstant.normal = glm::mat4(normalMat3);
            commandList->writeBuffer(mesh->objectBuffer, &modelPushConstant, sizeof(ObjectBuffer));

            // render
            auto meshGraphicsState = nvrhi::GraphicsState();
            meshGraphicsState.pipeline = pipeline->GetHandle();
            meshGraphicsState.framebuffer = framebuffer;
            meshGraphicsState.viewport = nvrhi::ViewportState().addViewportAndScissorRect(viewport);
            meshGraphicsState.addVertexBuffer({ mesh->vertexBuffer, 0, 0 });
            meshGraphicsState.indexBuffer = { mesh->indexBuffer, nvrhi::Format::R32_UINT };

            if (mesh->bindingSet != nullptr)
                meshGraphicsState.addBindingSet(mesh->bindingSet);

            commandList->setGraphicsState(meshGraphicsState);

            nvrhi::DrawArguments args;
            args.setVertexCount((uint32_t)mesh->indices.size());
            args.instanceCount = 1;

            commandList->drawIndexed(args);
        }
    }

    Ref<Model> Model::Create(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo)
    {
        return CreateRef<Model>(filepath, createInfo);
    }

    Ref<SkeletalAnimation> Model::GetActiveAnimation()
    {
        if (animations.empty() || activeAnimIndex == -1 || activeAnimIndex >= animations.size())
            return nullptr;
        return animations[activeAnimIndex];
    }

    bool Model::IsPlayingAnimation()
    {
        const Ref<SkeletalAnimation> &anim = GetActiveAnimation();
        if (!anim)
            return false;

        return anim->isPlaying;
    }

    
}