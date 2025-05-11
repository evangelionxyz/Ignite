#pragma once

#include "ignite/asset/asset.hpp"
#include "ignite/core/types.hpp"
#include "ignite/core/buffer.hpp"

#include <nvrhi/nvrhi.h>

#include <filesystem>

namespace ignite
{
    struct TextureCreateInfo
    {
        nvrhi::IDevice *device;
        
        i32 width = 1;
        i32 height = 1;

        bool flip = false;
        nvrhi::Format format;
        nvrhi::TextureDimension dimension = nvrhi::TextureDimension::Texture2D;
        nvrhi::SamplerAddressMode samplerMode = nvrhi::SamplerAddressMode::ClampToEdge;
    };

    class Texture : public Asset
    {
    public:
        Texture() = default;

        Texture(Buffer buffer, const TextureCreateInfo &createInfo);
        Texture(const std::string &filepath, const TextureCreateInfo &createInfo);

        ~Texture();

        void Write(nvrhi::ICommandList *commandList);

        static Ref<Texture> Create(Buffer buffer, const TextureCreateInfo &createInfo);
        static Ref<Texture> Create(const std::string &filepath, const TextureCreateInfo &createInfo);

        nvrhi::TextureHandle GetHandle() { return m_Handle; }
        nvrhi::SamplerHandle GetSampler() { return m_Sampler; }

        i32 GetWidth() const { return m_CreateInfo.width; }
        i32 GetHeight() const { return m_CreateInfo.height; }
        i32 GetChannels() const { return 4; }

        bool operator ==(const Texture &other) const 
        { 
            return m_Sampler.Get() == other.m_Sampler.Get() && m_Handle.Get() == other.m_Handle.Get();
        }

    private:
        void *m_Data = nullptr;
        bool m_WithSTBI = false;

        TextureCreateInfo m_CreateInfo;

        nvrhi::TextureHandle m_Handle;
        nvrhi::SamplerHandle m_Sampler;
    };
}
