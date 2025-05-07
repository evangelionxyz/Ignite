#pragma once

#include "ignite/core/types.hpp"

#include <nvrhi/nvrhi.h>
#include <glm/glm.hpp>

#include "shader.hpp"

namespace ignite {

    struct FramebufferAttachments
    {
        nvrhi::Format format;
    };

    struct RenderTargetCreateInfo
    {
        std::vector<FramebufferAttachments> attachments;
        nvrhi::IDevice *device = nullptr;
        bool depthRead = false;
        bool depthWrite = false;
    };

    class RenderTarget
    {
    public:
        RenderTarget(RenderTargetCreateInfo createInfo);

        void CreateFramebuffers(uint32_t backBufferCount, uint32_t backBufferIndex, const glm::uvec2 &size);
        void Resize(uint32_t width, uint32_t height, uint32_t backBufferIndex);

        nvrhi::FramebufferHandle GetCurrentFramebuffer();
        nvrhi::FramebufferHandle GetFramebuffer(uint32_t index);
        nvrhi::TextureHandle GetColorAttachment(uint32_t index);
        std::vector<nvrhi::TextureHandle> &GetColorAttachments();

        void ClearColorAttachment(nvrhi::CommandListHandle commandList, uint32_t index = 0, const glm::vec3 &clearColor = glm::vec3(0.0f, 0.0f, 0.0f));

    private:
        std::vector<nvrhi::FramebufferHandle> m_Framebuffers;

        uint32_t m_CurrentBackBufferIndex = 0;
        uint32_t m_BackBufferCount = 0;

        std::vector<nvrhi::TextureHandle> m_ColorAttachments;
        nvrhi::TextureHandle m_DepthAttachment;

        RenderTargetCreateInfo m_CreateInfo;

       
    };
}