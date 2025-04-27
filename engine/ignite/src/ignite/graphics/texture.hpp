#pragma once

#include "ignite/asset/asset.hpp"
#include "ignite/core/types.hpp"
#include "ignite/core/buffer.hpp"

#include <nvrhi/nvrhi.h>

#include <filesystem>

namespace ignite
{
    class Texture : public Asset
    {
    public:
        Texture() = default;
        Texture(Buffer buffer, i32 width, i32 height);
        Texture(const std::filesystem::path &filepath);

        void Write(nvrhi::ICommandList *commandList);

        static Ref<Texture> Create(Buffer buffer, i32 width = 1, i32 height = 1);
        static Ref<Texture> Create(const std::filesystem::path &filepath);

        nvrhi::TextureHandle GetHandle() { return m_Handle; }
        nvrhi::SamplerHandle GetSampler() { return m_Sampler; }

        unsigned char *GetPixelData() { return m_Buffer.Data; }
        i32 GetWidth() { return m_Width; }
        i32 GetHeight() { return m_Height; }
        i32 GetChannels() { return m_Channels; }

        bool operator ==(const Texture &other) const 
        { 
            return m_Sampler.Get() == other.m_Sampler.Get();
        }

    private:
        i32 m_Width, m_Height, m_Channels;
        Buffer m_Buffer;

        nvrhi::TextureHandle m_Handle;
        nvrhi::SamplerHandle m_Sampler;
    };
}