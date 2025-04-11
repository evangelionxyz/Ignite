#pragma once

#include <nvrhi/nvrhi.h>
#include <ignite/core/types.hpp>

#include <glm/glm.hpp>

namespace ignite
{
    struct RenderTarget
    {
        nvrhi::FramebufferHandle framebuffer;
        nvrhi::TextureHandle texture;
        glm::vec3 clearColor = {0.025f, 0.025f, 0.025f};
        nvrhi::IDevice *device;
        f32 width, height;

        RenderTarget() = default;
        RenderTarget(nvrhi::IDevice *_device, f32 _width, f32 _height);

        void Resize(f32 width, f32 height);
    };
}

