#pragma once

#include "ignite/asset/asset.hpp"
#include "ignite/core/types.hpp"

#include <nvrhi/nvrhi.h>

#include <filesystem>

class Texture : public Asset
{
public:
    Texture() = default;
    Texture(const std::filesystem::path &filepath);

    void Write(nvrhi::ICommandList *commandList);

    static Ref<Texture> Create(const std::filesystem::path &filepath);

    nvrhi::TextureHandle GetHandle() { return m_Handle; }
    nvrhi::SamplerHandle GetSampler() { return m_Sampler; }

    unsigned char *GetPixelData() { return m_PixelData; }
    i32 GetRowPitchSize() { return m_Width * 4; }
    i32 GetWidth() { return m_Width; }
    i32 GetHeight() { return m_Height; }
    i32 GetChannels() { return m_Channels; }

private:
    i32 m_Width, m_Height, m_Channels;
    unsigned char *m_PixelData;

    nvrhi::TextureHandle m_Handle;
    nvrhi::SamplerHandle m_Sampler;
};  