#include "texture.hpp"
#include "ignite/core/logger.hpp"

#include <stb_image.h>

#include "ignite/core/application.hpp"

namespace ignite
{
    Texture::Texture(Buffer buffer, const TextureCreateInfo &createInfo)
        : m_CreateInfo(createInfo), m_Data(buffer.Data)
    {
        nvrhi::IDevice *device = Application::GetRenderDevice();
        
        LOG_ASSERT(m_Data && buffer.Data, "[Texture] Pixel data is null");

        const auto &textureDesc = nvrhi::TextureDesc()
            .setDimension(m_CreateInfo.dimension)
            .setWidth(m_CreateInfo.width)
            .setHeight(m_CreateInfo.height)
            .setFormat(m_CreateInfo.format)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setMipLevels(m_CreateInfo.mipLevels)
            .setDebugName("Geometry Texture");
        
        m_Handle = device->createTexture(textureDesc);
        LOG_ASSERT(m_Handle, "Failed to create texture");

        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);

        m_Sampler = device->createSampler(samplerDesc);
        LOG_ASSERT(m_Sampler, "Failed to create texture sampler");

        nvrhi::CommandListHandle commandList = device->createCommandList();
        commandList->open();
        Write(commandList);
        commandList->close();
        device->executeCommandList(commandList);
    }

    Texture::Texture(const std::filesystem::path &filepath, const TextureCreateInfo &createInfo)
        : m_CreateInfo(createInfo), m_Filepath(filepath)
    {
        m_WithSTBI = true;

        LOG_ASSERT(std::filesystem::exists(filepath), "File does not found!");

        // always use RGBA
        i32 channels = 4;

        stbi_set_flip_vertically_on_load(m_CreateInfo.flip ? 1 : 0);

        switch (m_CreateInfo.format)
        {
            case nvrhi::Format::RGBA8_UNORM:
            {
                m_Data = stbi_load(filepath.generic_string().c_str(), &m_CreateInfo.width, &m_CreateInfo.height, &channels, 4);
                LOG_ASSERT(m_Data, "Failed to load texture data");
                break;
            }
            case nvrhi::Format::RGBA32_FLOAT:
            {
                m_Data = stbi_loadf(filepath.generic_string().c_str(), &m_CreateInfo.width, &m_CreateInfo.height, &channels, 4);
                LOG_ASSERT(m_Data, "Failed to load texture data");
                break;
            }
            default:
            {
                LOG_ASSERT(false, "[Texture] Please specify format explicitly!");
                return;
            }
        }

        nvrhi::IDevice *device = Application::GetRenderDevice();

        const auto &textureDesc = nvrhi::TextureDesc()
            .setDimension(m_CreateInfo.dimension)
            .setWidth(m_CreateInfo.width)
            .setHeight(m_CreateInfo.height)
            .setFormat(m_CreateInfo.format)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setMipLevels(m_CreateInfo.mipLevels)
            .setDebugName(filepath.generic_string());

        m_Handle = device->createTexture(textureDesc);
        LOG_ASSERT(m_Handle, "Failed to create texture");

        const auto samplerDesc = nvrhi::SamplerDesc()
            .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat)
            .setAllFilters(true);

        m_Sampler = device->createSampler(samplerDesc);
        LOG_ASSERT(m_Sampler, "Failed to create texture sampler");

        nvrhi::CommandListHandle commandList = device->createCommandList();
        commandList->open();
        Write(commandList);
        commandList->close();
        device->executeCommandList(commandList);
    }

    Texture::~Texture()
    {
    }

    void Texture::Write(nvrhi::ICommandList *commandList)
    {
        LOG_ASSERT(m_Data, "[Texture] Pixel data is null");

        // Row Pitch   = width * channels
        // Depth pitch = Row Pitch * height (for 3D TEXTURE)

        const int channels = 4; // always use RGBA
        int rowPitch = m_CreateInfo.width * channels;
        int depthPitch = rowPitch * m_CreateInfo.height;

        if (m_CreateInfo.format == nvrhi::Format::RGBA8_UNORM)
        {
            // char = 1 byte, 8 bit
            uint8_t *byteData = static_cast<uint8_t *>(m_Data);

            for (uint32_t mip = 0; mip < m_CreateInfo.mipLevels; ++mip)
            {
                commandList->writeTexture(m_Handle, 0, mip, byteData, rowPitch);
            }
        }
        else if (m_CreateInfo.format == nvrhi::Format::RGBA32_FLOAT)
        {
            // float = 4 bytes, 32 bit, we need to multiply sizeof(float)
            float *floatData = static_cast<float *>(m_Data);
            for (uint32_t mip = 0; mip < m_CreateInfo.mipLevels; ++mip)
            {
                commandList->writeTexture(m_Handle, 0, mip, floatData, rowPitch * sizeof(float), depthPitch * sizeof(float));
            }
        }

        if (m_Data && m_WithSTBI)
        {
            // free if loaded with stbi
            stbi_image_free(m_Data);
            m_Data = nullptr;
        }
    }

    Ref<Texture> Texture::Create(Buffer buffer, const TextureCreateInfo &createInfo)
    {
        return CreateRef<Texture>(buffer, createInfo);
    }

    Ref<Texture> Texture::Create(const std::filesystem::path &filepath, const TextureCreateInfo &createInfo)
    {
        return CreateRef<Texture>(filepath, createInfo);
    }
}
