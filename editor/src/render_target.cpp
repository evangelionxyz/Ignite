#include "render_target.hpp"

#include <ignite/core/logger.hpp>

namespace ignite
{
    RenderTarget::RenderTarget(nvrhi::IDevice *_device, f32 _width, f32 _height)
        : device(_device), width(_width), height(_height)
    {
        // create texture
        const auto textureDesc = nvrhi::TextureDesc()
            .setWidth(width)
            .setHeight(height)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setDebugName("Render target texture")
            .setInitialState(nvrhi::ResourceStates::RenderTarget)
            .setKeepInitialState(true)
            .setIsUAV(false)
            .setIsRenderTarget(true)
            .setIsTypeless(false)
            .setUseClearValue(true)
            .setClearValue(nvrhi::Color({clearColor.r, clearColor.g, clearColor.b, 1.0f}));

        texture = device->createTexture(textureDesc);
        LOG_ASSERT(texture, "Failed to create render target texture");

        // create framebuffer
        const auto fbDesc = nvrhi::FramebufferDesc()
        .addColorAttachment(texture);
        framebuffer = device->createFramebuffer(fbDesc);
        LOG_ASSERT(texture, "Failed to create render target framebuffer");
    }

    void Resize(f32 width, f32 height)
    {
        // TODO: Implement resizing
    }
}