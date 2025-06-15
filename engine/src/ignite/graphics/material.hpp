#pragma once

#include "ignite/core/buffer.hpp"
#include "ignite/core/application.hpp"
#include "texture.hpp"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <unordered_map>
#include <nvrhi/nvrhi.h>
#include <ranges>

namespace ignite {

    struct MaterialData
    {
        glm::vec4 baseColor;
        f32 metallic = 0.0f;
        f32 roughness = 1.0f;
        f32 emissive = 0.0f;
    };

    struct Material
    {
        MaterialData data;

        struct TextureData
        {
            Buffer buffer;
            uint32_t width;
            uint32_t height;
            uint32_t rowPitch = 0;
            nvrhi::TextureHandle handle;
        };

        uint32_t mipLevels = 4;

        std::unordered_map<aiTextureType, TextureData> textures;

        nvrhi::SamplerHandle sampler = nullptr;

        bool IsTransparent() const { return _transparent; }
        bool IsReflective() const { return _reflective; }
        bool ShouldWriteTexture() const { return _shouldWriteTexture; }

        void UploadTextureWithMips(nvrhi::ICommandList *commandList,
            nvrhi::TextureHandle handle,
            const void *baseData,
            uint32_t baseWidth,
            uint32_t baseHeight,
            uint32_t baseRowPitch,
            nvrhi::Format format,
            uint32_t mipLevels)
        {
            // Generate all mip levels on CPU
            auto mipChain = CPUMipGenerator::GenerateMipChain(baseData, baseWidth, baseHeight, baseRowPitch, format, mipLevels);

            // Upload all mip levels
            for (uint32_t mip = 0; mip < mipLevels && mip < mipChain.size(); ++mip)
            {
                const auto &mipData = mipChain[mip];
                commandList->writeTexture(handle, 0, mip, mipData.data.data(), mipData.rowPitch);
            }
        }

        void WriteBuffer(nvrhi::ICommandList *commandList)
        {
            for (const auto& [buffer, width, height, rowPitch, handle] : textures | std::views::values)
            {
                if (buffer.Data)
                {
                    UploadTextureWithMips(commandList, handle, buffer.Data, width, height, rowPitch, nvrhi::Format::RGBA8_UNORM, mipLevels);
                }
            }
        }

    private:
        bool _transparent = false;
        bool _reflective = false;
        bool _shouldWriteTexture = false;

        friend class MeshLoader;
    };
}