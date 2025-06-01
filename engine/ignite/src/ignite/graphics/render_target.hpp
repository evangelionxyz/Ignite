#pragma once

#include "ignite/core/types.hpp"

#include "shader.hpp"

#include <nvrhi/nvrhi.h>
#include <glm/glm.hpp>

namespace ignite {

    struct FramebufferAttachments
    {
        nvrhi::Format format;
        nvrhi::ResourceStates state =  nvrhi::ResourceStates::Unknown;
    };

    struct RenderTargetCreateInfo
    {
        std::vector<FramebufferAttachments> attachments;
        nvrhi::IDevice *device = nullptr;

        uint32_t width = 1280;
        uint32_t height = 720;
    };

    class RenderTarget
    {
    public:
        RenderTarget(RenderTargetCreateInfo createInfo);

        void CreateSingleFramebuffer();
        void CreateFramebuffers(uint32_t backBufferCount, uint32_t backBufferIndex);
        void Resize(uint32_t width, uint32_t height);

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        nvrhi::TextureHandle GetDepthAttachment();
        nvrhi::FramebufferHandle GetCurrentFramebuffer();
        nvrhi::FramebufferHandle GetFramebuffer(uint32_t index);
        nvrhi::TextureHandle GetColorAttachment(uint32_t index);
        std::vector<nvrhi::TextureHandle> &GetColorAttachments();

        void ClearColorAttachmentFloat(nvrhi::CommandListHandle commandList, uint32_t attachmentIndex = 0, const glm::vec3 &clearColor = glm::vec3(0.0f, 0.0f, 0.0f));
        void ClearColorAttachmentUint(nvrhi::CommandListHandle commandList, uint32_t attachmentIndex = 0, uint32_t clearColor = 0);

        void ClearDepthAttachment(nvrhi::CommandListHandle commandList, float depth, uint32_t stencil);

    private:

        void Create();

        std::vector<nvrhi::FramebufferHandle> m_Framebuffers;

        uint32_t m_CurrentBackBufferIndex = 0;
        uint32_t m_BackBufferCount = 0;
        uint32_t m_Width, m_Height;

        std::vector<nvrhi::TextureHandle> m_ColorAttachments;
        nvrhi::FramebufferDesc m_FramebufferDesc;
        nvrhi::TextureHandle m_DepthAttachment;
        bool m_IsSingleFramebuffer = false;
        bool m_Created = false;

        RenderTargetCreateInfo m_CreateInfo;

       
    };
}