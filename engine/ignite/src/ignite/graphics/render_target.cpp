#include "render_target.hpp"
#include <cstdint>
#include <vector>
#include <ignite/core/logger.hpp>
#include <ignite/core/types.hpp>

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

namespace ignite {

    RenderTarget::RenderTarget(RenderTargetCreateInfo createInfo)
        : m_CreateInfo(createInfo)
    {
        m_Width = m_CreateInfo.width;
        m_Height = m_CreateInfo.height;

        for (auto &attachment : m_CreateInfo.attachments)
        {
            bool isDepthAttachment = attachment.format == nvrhi::Format::D32S8 || attachment.format == nvrhi::Format::D16 || attachment.format == nvrhi::Format::D24S8 || attachment.format == nvrhi::Format::D32;
            bool isColorAttachment = attachment.format == nvrhi::Format::RGBA8_UNORM || attachment.format == nvrhi::Format::SRGBA8_UNORM || attachment.format == nvrhi::Format::R32_UINT;
            bool isRenderTarget = true;

            // find depth attachment if framebuffer is not created yet
            if (isDepthAttachment && m_DepthAttachment == nullptr && m_Framebuffers.empty())
            {
                nvrhi::TextureDesc depthDesc = nvrhi::TextureDesc();
                depthDesc.setWidth(m_Width);
                depthDesc.setHeight(m_Height);
                depthDesc.setFormat(attachment.format);
                depthDesc.setDebugName("Render target depth attachment");
                depthDesc.setInitialState(attachment.state);
                depthDesc.setIsRenderTarget(true);
                depthDesc.setKeepInitialState(true);
                depthDesc.setClearValue(nvrhi::Color(1.f));
                depthDesc.setUseClearValue(true);
                depthDesc.setDimension(nvrhi::TextureDimension::Texture2D);

                m_DepthAttachment = m_CreateInfo.device->createTexture(depthDesc);
                LOG_ASSERT(m_DepthAttachment, "Failed to create render target depth attachment");
            }

            // create color attachment if color attachments are empty
            if (isColorAttachment)
            {
                // create color attachment texture
                nvrhi::TextureDesc colorDesc;
                colorDesc.setWidth(m_Width);
                colorDesc.setHeight(m_Height);
                colorDesc.setFormat(attachment.format);
                colorDesc.setDebugName("Render target color attachment texture");
                colorDesc.setInitialState(attachment.state);
                colorDesc.setKeepInitialState(true);
                colorDesc.setIsUAV(false);
                colorDesc.setIsRenderTarget(isRenderTarget);
                colorDesc.setIsTypeless(false);
                colorDesc.setUseClearValue(true);

                nvrhi::TextureHandle colorAttachment = m_CreateInfo.device->createTexture(colorDesc);
                LOG_ASSERT(colorAttachment, "Failed to create render target color attachment texture");

                m_ColorAttachments.push_back(colorAttachment);
            }
        }
    }

    void RenderTarget::CreateSingleFramebuffer()
    {
        m_IsSingleFramebuffer = true;

        Create();
    }

    void RenderTarget::CreateFramebuffers(uint32_t backBufferCount, uint32_t backBufferIndex)
    {
        m_IsSingleFramebuffer = false;

        m_CurrentBackBufferIndex = backBufferIndex;
        m_BackBufferCount = backBufferCount;

        Create();
    }

    void RenderTarget::Resize(uint32_t width, uint32_t height)
    {
        m_Created = false;

        m_Width = width;
        m_Height = height;

        // clear 
        m_Framebuffers.clear();

        // recreate depth attachment
        if (m_DepthAttachment)
        {
            // copy description
            auto depthDesc = m_DepthAttachment->getDesc();
            depthDesc.width = width;
            depthDesc.height = height;

            m_DepthAttachment.Reset();

            m_DepthAttachment = m_CreateInfo.device->createTexture(depthDesc);
            LOG_ASSERT(m_DepthAttachment, "Failed to create render target depth attachment");
        }

        // recreate color attachments
        std::vector<nvrhi::TextureDesc> colorDescs;

        // copy color descriptions
        for (auto &colorAttachment : m_ColorAttachments)
        {
            auto colorDesc = colorAttachment->getDesc();
            colorDesc.width = width;
            colorDesc.height = height;

            colorDescs.push_back(colorDesc);
        }

        // create color attachments
        m_ColorAttachments.clear();
        for (auto &colorDesc : colorDescs)
        {
            nvrhi::TextureHandle colorAttachment = m_CreateInfo.device->createTexture(colorDesc);
            LOG_ASSERT(colorAttachment, "Failed to create render target color attachment texture");

            m_ColorAttachments.push_back(colorAttachment);
        }

        // reset create desc
        m_FramebufferDesc = nvrhi::FramebufferDesc();
    }

    nvrhi::TextureHandle RenderTarget::GetDepthAttachment()
    {
        return m_DepthAttachment;
    }

    nvrhi::FramebufferHandle RenderTarget::GetCurrentFramebuffer()
    {
        if (m_IsSingleFramebuffer)
            m_CurrentBackBufferIndex = 0;

        if (m_CurrentBackBufferIndex < m_Framebuffers.size())
            return m_Framebuffers[m_CurrentBackBufferIndex];

        LOG_ASSERT(false, "[Render target] Invalid current back buffer index!");
        return nullptr;
    }

    nvrhi::FramebufferHandle RenderTarget::GetFramebuffer(uint32_t index)
    {
        if (m_IsSingleFramebuffer)
            index = 0;

        if (m_Framebuffers.size() > index)
            return m_Framebuffers[index];

        LOG_ASSERT(false, "[Render target] Framebuffer index out of bound!");
        return nullptr;
    }

    nvrhi::TextureHandle RenderTarget::GetColorAttachment(uint32_t index)
    {
        if (m_ColorAttachments.size() > index)
            return m_ColorAttachments[index];

        LOG_ASSERT(false, "[Render target] Color attachments index out of bound!");
        return nullptr;
    }

    std::vector<nvrhi::TextureHandle> &RenderTarget::GetColorAttachments()
    {
        return m_ColorAttachments;
    }

    void RenderTarget::ClearColorAttachmentFloat(nvrhi::CommandListHandle commandList, uint32_t attachmentIndex, const glm::vec3 &clearColor) const
    {
        if (attachmentIndex >= m_ColorAttachments.size())
        {
            attachmentIndex = glm::max(static_cast<int>(m_ColorAttachments.size()) - 1, 0);
            LOG_ASSERT(false, "[Render target] Color attachments index out of bound!");
        }

        i32 backBufferIndex = m_CurrentBackBufferIndex;
        if (m_IsSingleFramebuffer)
            backBufferIndex = 0;

        nvrhi::TextureHandle texture = m_ColorAttachments[attachmentIndex];
        const nvrhi::Format format = texture->getDesc().format;

        nvrhi::utils::ClearColorAttachment(
            commandList,
            m_Framebuffers[backBufferIndex],
            attachmentIndex,
            nvrhi::Color(clearColor.x, clearColor.y, clearColor.z, 1.0f)
        );
    }

    void RenderTarget::ClearColorAttachmentUint(nvrhi::CommandListHandle commandList, uint32_t attachmentIndex, uint32_t clearColor) const
    {
        if (attachmentIndex >= m_ColorAttachments.size())
        {
            attachmentIndex = glm::max((int)m_ColorAttachments.size() - 1, 0);
            LOG_ASSERT(false, "[Render target] Color attachments index out of bound!");
        }

        nvrhi::TextureHandle texture = m_ColorAttachments[attachmentIndex];
        const nvrhi::Format format = texture->getDesc().format;

        bool isUint = format == nvrhi::Format::R32_UINT || format == nvrhi::Format::RGBA8_UINT || format == nvrhi::Format::R8_UINT;
        LOG_ASSERT(isUint, "[Render Target] Color attachment is not UINT type!");

        commandList->clearTextureUInt(texture, nvrhi::AllSubresources, clearColor);
    }

    void RenderTarget::ClearDepthAttachment(nvrhi::CommandListHandle commandList, float depth, uint32_t stencil) const
    {
        i32 backBufferIndex = m_CurrentBackBufferIndex;
        if (m_IsSingleFramebuffer)
            backBufferIndex = 0;

        nvrhi::utils::ClearDepthStencilAttachment(commandList, m_Framebuffers[backBufferIndex], depth, stencil);
    }

    void RenderTarget::Create()
    {
        // create framebuffer
        if (!m_Created)
        {
            if (m_DepthAttachment != nullptr)
                m_FramebufferDesc.setDepthAttachment(m_DepthAttachment);

            // add color attachments
            for (auto &colorAttachment : m_ColorAttachments)
                m_FramebufferDesc.addColorAttachment(colorAttachment);
        }

        if (m_IsSingleFramebuffer)
        {
            m_Framebuffers.resize(1);
            if (m_Framebuffers[0] == nullptr)
            {
                m_Framebuffers[0] = m_CreateInfo.device->createFramebuffer(m_FramebufferDesc);
                LOG_ASSERT(m_Framebuffers[0], "Failed to create render target framebuffer");
            }
        }
        else
        {
            m_Framebuffers.resize(m_BackBufferCount);
            if (m_Framebuffers[m_CurrentBackBufferIndex] == nullptr)
            {
                m_Framebuffers[m_CurrentBackBufferIndex] = m_CreateInfo.device->createFramebuffer(m_FramebufferDesc);
                LOG_ASSERT(m_Framebuffers[m_CurrentBackBufferIndex], "Failed to create render target framebuffer");
            }
        }

        m_Created = true;
    }

}