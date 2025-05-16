#pragma once

#include "mesh.hpp"

#include "ignite/asset/asset.hpp"

namespace ignite {

    struct ModelCreateInfo
    {
        nvrhi::IDevice *device;
        nvrhi::BufferHandle cameraBuffer;
        nvrhi::BufferHandle lightBuffer;
        nvrhi::BufferHandle envBuffer;
        nvrhi::BufferHandle debugBuffer;
    };

    class Model : public Asset
    {
    public:
        Model() = default;
        Model(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo);

        void SetEnvironmentTexture(nvrhi::TextureHandle envTexture);
        void CreateBindingSet(nvrhi::BindingLayoutHandle bindingLayout);
        void WriteBuffer(nvrhi::ICommandList *commandList);

        void OnUpdate(f32 deltaTime);
        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, const Ref<GraphicsPipeline> &pipeline);
        const std::filesystem::path &GetFilepath() const { return m_Filepath; }
        static Ref<Model> Create(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo);

        Ref<SkeletalAnimation> GetActiveAnimation();
        bool IsPlayingAnimation();
        bool IsBufferWritten() const { return m_BufferWritten; }

        glm::mat4 transform = glm::mat4(1.0f);
        std::vector<Ref<SkeletalAnimation>> animations;
        std::vector<NodeInfo> nodes;
        std::vector<glm::mat4> boneTransforms;
        Skeleton skeleton;
        std::vector<Ref<Mesh>> meshes;
        i32 activeAnimIndex = -1;

        static AssetType StaticType() { return AssetType::Model; }
        virtual AssetType GetType() override { return StaticType(); };

    private:

        ModelCreateInfo m_CreateInfo;
        nvrhi::TextureHandle m_EnvironmentTexture;
        std::filesystem::path m_Filepath;

        Assimp::Importer m_Importer;

        bool m_BufferWritten = false;
    };
}
