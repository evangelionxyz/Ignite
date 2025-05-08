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
    }

    void RenderTarget::CreateSingleFramebuffer(const glm::uvec2 &size)
    {
        m_IsSingleFramebuffer = true;

        for (auto attachment : m_CreateInfo.attachments)
        {
            bool isDepthAttachment = attachment.format == nvrhi::Format::D32S8 || attachment.format == nvrhi::Format::D16 || attachment.format == nvrhi::Format::D24S8 || attachment.format == nvrhi::Format::D32;
            bool isColorAttachment = attachment.format == nvrhi::Format::RGBA8_UNORM || attachment.format == nvrhi::Format::SRGBA8_UNORM;

            // find depth attachment if framebuffer is not created yet

            if (isDepthAttachment && m_DepthAttachment == nullptr && m_Framebuffers.empty())
            {
                nvrhi::TextureDesc depthDesc = nvrhi::TextureDesc();
                depthDesc.setWidth(size.x);
                depthDesc.setHeight(size.y);
                depthDesc.setFormat(attachment.format);
                depthDesc.setDebugName("Render target depth attachment");
                depthDesc.setInitialState(m_CreateInfo.depthWrite ? nvrhi::ResourceStates::DepthWrite : nvrhi::ResourceStates::Unknown);
                depthDesc.setIsRenderTarget(true);
                depthDesc.setIsTypeless(true);
                depthDesc.setKeepInitialState(m_CreateInfo.depthWrite);
                depthDesc.setClearValue(nvrhi::Color(0.f));
                depthDesc.setUseClearValue(true);
                depthDesc.setDimension(nvrhi::TextureDimension::Texture2D);

                m_DepthAttachment = m_CreateInfo.device->createTexture(depthDesc);
                LOG_ASSERT(m_DepthAttachment, "Failed to create render target depth attachment");
            }

            // create color attachment if color attachments are empty
            if (isColorAttachment && m_ColorAttachments.empty())
            {
                // create color attachment texture
                const auto colorDesc = nvrhi::TextureDesc()
                    .setWidth(size.x)
                    .setHeight(size.y)
                    .setFormat(attachment.format)
                    .setDebugName("Render target color attachment texture")
                    .setInitialState(nvrhi::ResourceStates::RenderTarget)
                    .setKeepInitialState(true)
                    .setIsUAV(false)
                    .setIsRenderTarget(true)
                    .setIsTypeless(false)
                    .setUseClearValue(true);

                nvrhi::TextureHandle colorAttachment = m_CreateInfo.device->createTexture(colorDesc);
                LOG_ASSERT(colorAttachment, "Failed to create render target color attachment texture");

                m_ColorAttachments.push_back(colorAttachment);
            }
        }

        // create framebuffer
        auto fbDesc = nvrhi::FramebufferDesc();

        // set depth attachment if available
        if (m_DepthAttachment != nullptr)
            fbDesc.setDepthAttachment(m_DepthAttachment);

        // add color attachments
        for (auto &colorAttachment : m_ColorAttachments)
            fbDesc.addColorAttachment(colorAttachment);

        m_Framebuffers.resize(1);

        if (m_Framebuffers[0] == nullptr)
        {
            m_Framebuffers[0] = m_CreateInfo.device->createFramebuffer(fbDesc);
            LOG_ASSERT(m_Framebuffers[0], "Failed to create render target framebuffer");
        }
    }

    void RenderTarget::CreateFramebuffers(uint32_t backBufferCount, uint32_t backBufferIndex, const glm::uvec2 &size)
    {
        for (auto attachment : m_CreateInfo.attachments)
        {
            bool isDepthAttachment = attachment.format == nvrhi::Format::D32S8 || attachment.format == nvrhi::Format::D16 || attachment.format == nvrhi::Format::D24S8 || attachment.format == nvrhi::Format::D32;
            bool isColorAttachment = attachment.format == nvrhi::Format::RGBA8_UNORM || attachment.format == nvrhi::Format::SRGBA8_UNORM;

            // find depth attachment if framebuffer is not created yet

            if (isDepthAttachment && m_DepthAttachment == nullptr && m_Framebuffers.empty())
            {
                nvrhi::TextureDesc depthDesc = nvrhi::TextureDesc();
                depthDesc.setWidth(size.x);
                depthDesc.setHeight(size.y);
                depthDesc.setFormat(attachment.format);
                depthDesc.setDebugName("Render target depth attachment");
                depthDesc.setInitialState(m_CreateInfo.depthWrite ? nvrhi::ResourceStates::DepthWrite : nvrhi::ResourceStates::Unknown);
                depthDesc.setIsRenderTarget(true);
                depthDesc.setIsTypeless(true);
                depthDesc.setKeepInitialState(m_CreateInfo.depthWrite);
                depthDesc.setClearValue(nvrhi::Color(0.f));
                depthDesc.setUseClearValue(true);
                depthDesc.setDimension(nvrhi::TextureDimension::Texture2D);

                m_DepthAttachment = m_CreateInfo.device->createTexture(depthDesc);
                LOG_ASSERT(m_DepthAttachment, "Failed to create render target depth attachment");
            }

            // create color attachment if color attachments are empty
            if (isColorAttachment && m_ColorAttachments.empty())
            {
                // create color attachment texture
                const auto colorDesc = nvrhi::TextureDesc()
                    .setWidth(size.x)
                    .setHeight(size.y)
                    .setFormat(attachment.format)
                    .setDebugName("Render target color attachment texture")
                    .setInitialState(nvrhi::ResourceStates::RenderTarget)
                    .setKeepInitialState(true)
                    .setIsUAV(false)
                    .setIsRenderTarget(true)
                    .setIsTypeless(false)
                    .setUseClearValue(true);

                nvrhi::TextureHandle colorAttachment = m_CreateInfo.device->createTexture(colorDesc);
                LOG_ASSERT(colorAttachment, "Failed to create render target color attachment texture");

                m_ColorAttachments.push_back(colorAttachment);
            }
        }

        m_CurrentBackBufferIndex = backBufferIndex;
        m_BackBufferCount = backBufferCount;

        // create framebuffer
        auto fbDesc = nvrhi::FramebufferDesc();

        // set depth attachment if available
        if (m_DepthAttachment != nullptr)
            fbDesc.setDepthAttachment(m_DepthAttachment);

        // add color attachments
        for (auto &colorAttachment : m_ColorAttachments)
            fbDesc.addColorAttachment(colorAttachment);

        m_Framebuffers.resize(backBufferCount);

        if (m_Framebuffers[backBufferIndex] == nullptr)
        {
            m_Framebuffers[backBufferIndex] = m_CreateInfo.device->createFramebuffer(fbDesc);
            LOG_ASSERT(m_Framebuffers[backBufferIndex], "Failed to create render target framebuffer");
        }
    }

    void RenderTarget::Resize(uint32_t width, uint32_t height, uint32_t backBufferIndex)
    {
        m_CurrentBackBufferIndex = backBufferIndex;

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
        // destroy color attachments
        m_ColorAttachments.clear();

        // create color attachments
        for (auto &colorDesc : colorDescs)
        {
            nvrhi::TextureHandle colorAttachment = m_CreateInfo.device->createTexture(colorDesc);
            LOG_ASSERT(colorAttachment, "Failed to create render target color attachment texture");

            m_ColorAttachments.push_back(colorAttachment);
        }

        // create framebuffer
        auto fbDesc = nvrhi::FramebufferDesc();

        // set depth attachment if available
        if (m_DepthAttachment != nullptr)
            fbDesc.setDepthAttachment(m_DepthAttachment);

        // add color attachments
        for (auto &colorAttachment : m_ColorAttachments)
            fbDesc.addColorAttachment(colorAttachment);

        m_Framebuffers.resize(m_BackBufferCount);

        if (m_Framebuffers[backBufferIndex] == nullptr)
        {
            m_Framebuffers[backBufferIndex] = m_CreateInfo.device->createFramebuffer(fbDesc);
            LOG_ASSERT(m_Framebuffers[backBufferIndex], "Failed to create render target framebuffer");
        }
    }

    nvrhi::TextureHandle RenderTarget::GetDepthAttachment()
    {
        return m_DepthAttachment;
    }

    nvrhi::FramebufferHandle RenderTarget::GetCurrentFramebuffer()
    {
        if (m_IsSingleFramebuffer)
            m_CurrentBackBufferIndex = 0;

        if (m_CurrentBackBufferIndex <= m_Framebuffers.size())
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

    void RenderTarget::ClearColorAttachment(nvrhi::CommandListHandle commandList, uint32_t index, const glm::vec3 &clearColor)
    {
        if (index >= m_ColorAttachments.size())
        {
            index = glm::max((int)m_ColorAttachments.size() - 1, 0);
            LOG_ASSERT(false, "[Render target] Color attachments index out of bound!");
        }

        i32 backBufferIndex = m_CurrentBackBufferIndex;
        if (m_IsSingleFramebuffer)
            backBufferIndex = 0;

        nvrhi::utils::ClearColorAttachment(commandList, m_Framebuffers[backBufferIndex], index, nvrhi::Color(clearColor.x, clearColor.y, clearColor.z, 1.0f));
    }

    void RenderTarget::ClearDepthAttachment(nvrhi::CommandListHandle commandList, float depth, uint32_t stencil)
    {
        i32 backBufferIndex = m_CurrentBackBufferIndex;
        if (m_IsSingleFramebuffer)
            backBufferIndex = 0;

        nvrhi::utils::ClearDepthStencilAttachment(commandList, m_Framebuffers[backBufferIndex], depth, stencil);
    }

}