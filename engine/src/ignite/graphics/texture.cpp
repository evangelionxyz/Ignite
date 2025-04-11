#include "texture.hpp"

#include "ignite/core/logger.hpp"
#include "ignite/core/application.hpp"

#include <stb_image.h>

Texture::Texture(const std::filesystem::path &filepath)
{
    LOG_ASSERT(exists(filepath), "File does not found!");

    m_PixelData = stbi_load(filepath.generic_string().c_str(), &m_Width, &m_Height, &m_Channels, 4);
    LOG_ASSERT(m_PixelData, "Failed to load texture data");

    nvrhi::IDevice *device = Application::GetDeviceManager()->GetDevice();

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

void Texture::Write(nvrhi::ICommandList *commandList)
{
    commandList->writeTexture(m_Handle, 0, 0, m_PixelData, GetRowPitchSize());
}

Ref<Texture> Texture::Create(const std::filesystem::path &filepath)
{
    return CreateRef<Texture>(filepath);
}