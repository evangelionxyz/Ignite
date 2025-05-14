#pragma once

#include "mesh.hpp"

namespace ignite {
    class Model
    {
    public:
        Model() = default;
        Model(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo);

        void SetEnvironmentTexture(nvrhi::TextureHandle envTexture);
        void CreateBindingSet(nvrhi::BindingLayoutHandle bindingLayout);

        void WriteBuffer(nvrhi::CommandListHandle commandList);
        void OnUpdate(f32 deltaTime);
        void Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, const Ref<GraphicsPipeline> &pipeline);
        const std::filesystem::path &GetFilepath() const { return m_Filepath; }
        static Ref<Model> Create(const std::filesystem::path &filepath, const ModelCreateInfo &createInfo);

        Ref<SkeletalAnimation> GetActiveAnimation();
        bool IsPlayingAnimation();

        glm::mat4 transform = glm::mat4(1.0f);
        std::vector<Ref<SkeletalAnimation>> animations;
        std::vector<NodeInfo> nodes;
        std::vector<glm::mat4> boneTransforms;
        Skeleton skeleton;
        std::vector<Ref<Mesh>> meshes;
        i32 activeAnimIndex = -1;

    private:
        ModelCreateInfo m_CreateInfo;
        nvrhi::TextureHandle m_EnvironmentTexture;
        std::filesystem::path m_Filepath;
    };
}
