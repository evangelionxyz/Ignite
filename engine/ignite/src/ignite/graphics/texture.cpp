#include "texture.hpp"
#include "ignite/core/logger.hpp"

#include <stb_image.h>

namespace ignite
{
    Texture::Texture(nvrhi::IDevice *device, Buffer buffer, i32 width, i32 height)
        : m_Buffer(buffer), m_Width(width), m_Height(height)
    {
        const auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setWidth(m_Width)
            .setHeight(m_Height)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("Texture");
        
        m_Handle = device->createTexture(textureDesc);
        LOG_ASSERT(m_Handle, "Failed to create texture");

        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);

        m_Sampler = device->createSampler(samplerDesc);
        LOG_ASSERT(m_Sampler, "Failed to create texture sampler");
    }

    Texture::Texture(nvrhi::IDevice *device, const std::filesystem::path &filepath)
    {
        LOG_ASSERT(exists(filepath), "File does not found!");

        m_Buffer.Data = stbi_load(filepath.generic_string().c_str(), &m_Width, &m_Height, &m_Channels, 4);
        LOG_ASSERT(m_Buffer.Data, "Failed to load texture data");

        m_Buffer.Size = m_Width * m_Height * 4;

        const auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setWidth(m_Width)
            .setHeight(m_Height)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("Geometry Texture");

        m_Handle = device->createTexture(textureDesc);
        LOG_ASSERT(m_Handle, "Failed to create texture");

        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);

        m_Sampler = device->createSampler(samplerDesc);
        LOG_ASSERT(m_Sampler, "Failed to create texture sampler");
    }

    Texture::~Texture()
    {
    }

    void Texture::Write(nvrhi::ICommandList *commandList)
    {
        commandList->writeTexture(m_Handle, 0, 0, m_Buffer.Data, m_Width * 4);
    }

    Ref<Texture> Texture::Create(nvrhi::IDevice *device, Buffer buffer, i32 width, i32 height)
    {
        return CreateRef<Texture>(device, buffer, width, height);
    }

    Ref<Texture> Texture::Create(nvrhi::IDevice *device, const std::filesystem::path &filepath)
    {
        return CreateRef<Texture>(device, filepath);
    }
}