#pragma once

#include <ranges>

#include "ignite/core/buffer.hpp"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <unordered_map>
#include <nvrhi/nvrhi.h>

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
            uint32_t rowPitch = 0;
            nvrhi::TextureHandle handle;
        };

        std::unordered_map<aiTextureType, TextureData> textures;

        nvrhi::SamplerHandle sampler = nullptr;

        bool IsTransparent() const { return _transparent; }
        bool IsReflective() const { return _reflective; }
        bool ShouldWriteTexture() const { return _shouldWriteTexture; }

        void WriteBuffer(nvrhi::ICommandList *commandList)
        {
            for (const auto& [buffer, rowPitch, handle] : textures | std::views::values)
            {
                if (buffer.Data)
                {
                    commandList->writeTexture(handle, 0, 0, buffer.Data, rowPitch);
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