#pragma once

#include "ignite/asset/asset.hpp"
#include "ignite/core/types.hpp"
#include "ignite/core/buffer.hpp"

#include <nvrhi/nvrhi.h>

#include <filesystem>

namespace ignite
{

    struct MipLevelData
    {
        std::vector<uint8_t> data;
        uint32_t width;
        uint32_t height;
        uint32_t rowPitch;
        uint32_t slicePitch;
    };

    class CPUMipGenerator
    {
    public:
        static std::vector<MipLevelData> GenerateMipChain(const void *baseData, uint32_t baseWidth, uint32_t baseHeight, uint32_t baseRowPitch, nvrhi::Format format, uint32_t mipLevels)
        {
            std::vector<MipLevelData> mipChain;
            mipChain.reserve(mipLevels);

            // Add base mip Level (mip 0)
            MipLevelData baseMip;
            baseMip.width = baseWidth;
            baseMip.height = baseHeight;
            baseMip.rowPitch = baseRowPitch;
            baseMip.slicePitch = baseRowPitch * baseHeight;
            baseMip.data.resize(baseMip.slicePitch);
            memcpy(baseMip.data.data(), baseData, baseMip.slicePitch);
            mipChain.push_back(std::move(baseMip));

            // Generate subsequent mip levels
            for (uint32_t mip = 1; mip < mipLevels; ++mip)
            {
                const MipLevelData &srcMip = mipChain[mip - 1];
                MipLevelData dstMip = GenerateNextMipLevel(srcMip, format);
                mipChain.push_back(std::move(dstMip));
            }

            return mipChain;

        }

        static MipLevelData GenerateNextMipLevel(const MipLevelData &srcMip, nvrhi::Format format)
        {
            MipLevelData dstMip;
            dstMip.width = std::max(1u, srcMip.width / 2);
            dstMip.height = std::max(1u, srcMip.height / 2);

            uint32_t bytesPerPixel = GetBytesPerPixel(format);
            dstMip.rowPitch = dstMip.width * bytesPerPixel;
            dstMip.slicePitch = dstMip.rowPitch * dstMip.height;
            dstMip.data.resize(dstMip.slicePitch);

            switch (format) {
            case nvrhi::Format::RGBA8_UNORM:
            case nvrhi::Format::SRGBA8_UNORM:
            case nvrhi::Format::BGRA8_UNORM:
            case nvrhi::Format::SBGRA8_UNORM:
                DownsampleRGBA8(srcMip, dstMip);
                break;
            case nvrhi::Format::RG8_UNORM:
                DownsampleRG8(srcMip, dstMip);
                break;

            case nvrhi::Format::R8_UNORM:
                DownsampleR8(srcMip, dstMip);
                break;

            case nvrhi::Format::RGBA16_FLOAT:
                DownsampleRGBA16_FLOAT(srcMip, dstMip);
                break;

            case nvrhi::Format::RGBA32_FLOAT:
                DownsampleRGBA32_FLOAT(srcMip, dstMip);
                break;

            default:
                // Fallback - treat as RGBA8
                DownsampleRGBA8(srcMip, dstMip);
                break;
            }

            return dstMip;
        }

        static uint32_t GetBytesPerPixel(nvrhi::Format format)
        {
            switch (format) {
            case nvrhi::Format::RGBA8_UNORM:
            case nvrhi::Format::SRGBA8_UNORM:
            case nvrhi::Format::BGRA8_UNORM:
            case nvrhi::Format::SBGRA8_UNORM:
                return 4;
            case nvrhi::Format::RG8_UNORM:
                return 2;
            case nvrhi::Format::R8_UNORM:
                return 1;
            case nvrhi::Format::RGBA16_FLOAT:
                return 8;
            case nvrhi::Format::RGBA32_FLOAT:
                return 16;
            default:
                return 4; // Default to RGBA8
            }
        }

        static void DownsampleRGBA8(const MipLevelData &src, MipLevelData &dst)
        {
            const uint8_t *srcData = src.data.data();
            uint8_t *dstData = dst.data.data();

            for (uint32_t y = 0; y < dst.height; ++y)
            {
                for (uint32_t x = 0; x < dst.width; ++x)
                {
                    uint32_t srcX = x * 2;
                    uint32_t srcY = y * 2;

                    // Sample 2x2 block from source
                    uint32_t r = 0, g = 0, b = 0, a = 0;
                    uint32_t sampleCount = 0;

                    for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                    {
                        for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                        {
                            uint32_t srcOffset = (srcY + dy) * src.rowPitch + (srcX + dx) * 4;
                            r += srcData[srcOffset + 0];
                            g += srcData[srcOffset + 1];
                            b += srcData[srcOffset + 2];
                            a += srcData[srcOffset + 3];
                            sampleCount++;
                        }
                    }

                    // Average the samples
                    uint32_t dstOffset = y * dst.rowPitch + x * 4;
                    dstData[dstOffset + 0] = static_cast<uint8_t>(r / sampleCount);
                    dstData[dstOffset + 1] = static_cast<uint8_t>(g / sampleCount);
                    dstData[dstOffset + 2] = static_cast<uint8_t>(b / sampleCount);
                    dstData[dstOffset + 3] = static_cast<uint8_t>(a / sampleCount);
                }
            }
        }

        static void DownsampleRG8(const MipLevelData &src, MipLevelData &dst)
        {
            const uint8_t *srcData = src.data.data();
            uint8_t *dstData = dst.data.data();

            for (uint32_t y = 0; y < dst.height; ++y)
            {
                for (uint32_t x = 0; x < dst.width; ++x)
                {
                    uint32_t srcX = x * 2;
                    uint32_t srcY = y * 2;

                    uint32_t r = 0, g = 0;
                    uint32_t sampleCount = 0;

                    for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                    {
                        for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                        {
                            uint32_t srcOffset = (srcY + dy) * src.rowPitch + (srcX + dx) * 2;
                            r += srcData[srcOffset + 0];
                            g += srcData[srcOffset + 1];
                            sampleCount++;
                        }
                    }

                    uint32_t dstOffset = y * dst.rowPitch + x * 2;
                    dstData[dstOffset + 0] = static_cast<uint8_t>(r / sampleCount);
                    dstData[dstOffset + 1] = static_cast<uint8_t>(g / sampleCount);
                }
            }
        }

        static void DownsampleR8(const MipLevelData &src, MipLevelData &dst)
        {
            const uint8_t *srcData = src.data.data();
            uint8_t *dstData = dst.data.data();

            for (uint32_t y = 0; y < dst.height; ++y)
            {
                for (uint32_t x = 0; x < dst.width; ++x)
                {
                    uint32_t srcX = x * 2;
                    uint32_t srcY = y * 2;

                    uint32_t r = 0;
                    uint32_t sampleCount = 0;

                    for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                    {
                        for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                        {
                            uint32_t srcOffset = (srcY + dy) * src.rowPitch + (srcX + dx);
                            r += srcData[srcOffset];
                            sampleCount++;
                        }
                    }

                    uint32_t dstOffset = y * dst.rowPitch + x;
                    dstData[dstOffset] = static_cast<uint8_t>(r / sampleCount);
                }
            }
        }

        static void DownsampleRGBA16_FLOAT(const MipLevelData &src, MipLevelData &dst)
        {
            const uint16_t *srcData = reinterpret_cast<const uint16_t *>(src.data.data());
            uint16_t *dstData = reinterpret_cast<uint16_t *>(dst.data.data());
            uint32_t srcRowPitchInPixels = src.rowPitch / 8; // 8 bytes per pixel
            uint32_t dstRowPitchInPixels = dst.rowPitch / 8;

            for (uint32_t y = 0; y < dst.height; ++y)
            {
                for (uint32_t x = 0; x < dst.width; ++x)
                {
                    uint32_t srcX = x * 2;
                    uint32_t srcY = y * 2;

                    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
                    uint32_t sampleCount = 0;

                    for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                    {
                        for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                        {
                            uint32_t srcIdx = (srcY + dy) * srcRowPitchInPixels + (srcX + dx) * 4;
                            r += half_to_float(srcData[srcIdx + 0]);
                            g += half_to_float(srcData[srcIdx + 1]);
                            b += half_to_float(srcData[srcIdx + 2]);
                            a += half_to_float(srcData[srcIdx + 3]);
                            sampleCount++;
                        }
                    }

                    uint32_t dstIdx = y * dstRowPitchInPixels + x * 4;
                    dstData[dstIdx + 0] = float_to_half(r / sampleCount);
                    dstData[dstIdx + 1] = float_to_half(g / sampleCount);
                    dstData[dstIdx + 2] = float_to_half(b / sampleCount);
                    dstData[dstIdx + 3] = float_to_half(a / sampleCount);
                }
            }
        }

        static void DownsampleRGBA32_FLOAT(const MipLevelData &src, MipLevelData &dst)
        {
            const float *srcData = reinterpret_cast<const float *>(src.data.data());
            float *dstData = reinterpret_cast<float *>(dst.data.data());
            uint32_t srcRowPitchInPixels = src.rowPitch / 16; // 16 bytes per pixel
            uint32_t dstRowPitchInPixels = dst.rowPitch / 16;

            for (uint32_t y = 0; y < dst.height; ++y)
            {
                for (uint32_t x = 0; x < dst.width; ++x) {
                    uint32_t srcX = x * 2;
                    uint32_t srcY = y * 2;

                    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
                    uint32_t sampleCount = 0;

                    for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                    {
                        for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx) {
                            uint32_t srcIdx = (srcY + dy) * srcRowPitchInPixels + (srcX + dx) * 4;
                            r += srcData[srcIdx + 0];
                            g += srcData[srcIdx + 1];
                            b += srcData[srcIdx + 2];
                            a += srcData[srcIdx + 3];
                            sampleCount++;
                        }
                    }

                    uint32_t dstIdx = y * dstRowPitchInPixels + x * 4;
                    dstData[dstIdx + 0] = r / sampleCount;
                    dstData[dstIdx + 1] = g / sampleCount;
                    dstData[dstIdx + 2] = b / sampleCount;
                    dstData[dstIdx + 3] = a / sampleCount;
                }
            }
        }

        static float half_to_float(uint16_t h)
        {
            uint32_t sign = (h & 0x8000) << 16;
            uint32_t exponent = (h & 0x7C00) >> 10;
            uint32_t mantissa = h & 0x03FF;

            if (exponent == 0) {
                if (mantissa == 0) return 0.0f; // Zero
                // Denormalized
                exponent = 127 - 14;
                while ((mantissa & 0x400) == 0) {
                    mantissa <<= 1;
                    exponent--;
                }
                mantissa &= 0x3FF;
            }
            else if (exponent == 31) {
                exponent = 255; // Infinity or NaN
            }
            else {
                exponent += 127 - 15; // Bias adjustment
            }

            uint32_t result = sign | (exponent << 23) | (mantissa << 13);
            return *reinterpret_cast<float *>(&result);
        }

        static uint16_t float_to_half(float f)
        {
            uint32_t bits = *reinterpret_cast<uint32_t *>(&f);
            uint32_t sign = (bits & 0x80000000) >> 16;
            uint32_t exponent = (bits & 0x7F800000) >> 23;
            uint32_t mantissa = bits & 0x007FFFFF;

            if (exponent == 0) return static_cast<uint16_t>(sign); // Zero
            if (exponent == 255) return static_cast<uint16_t>(sign | 0x7C00 | (mantissa ? 0x200 : 0)); // Inf/NaN

            exponent -= 127 - 15; // Bias adjustment
            if (exponent <= 0) return static_cast<uint16_t>(sign); // Underflow to zero
            if (exponent >= 31) return static_cast<uint16_t>(sign | 0x7C00); // Overflow to infinity

            return static_cast<uint16_t>(sign | (exponent << 10) | (mantissa >> 13));
        }
    };

    struct TextureCreateInfo
    {
        i32 width = 1;
        i32 height = 1;
        uint32_t mipLevels = 1;
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
        Texture(const std::filesystem::path &filepath, const TextureCreateInfo &createInfo);

        ~Texture();

        void Write(nvrhi::ICommandList *commandList);

        static Ref<Texture> Create(Buffer buffer, const TextureCreateInfo &createInfo);
        static Ref<Texture> Create(const std::filesystem::path &filepath, const TextureCreateInfo &createInfo);

        nvrhi::TextureHandle GetHandle() { return m_Handle; }
        nvrhi::SamplerHandle GetSampler() { return m_Sampler; }

        i32 GetWidth() const { return m_CreateInfo.width; }
        i32 GetHeight() const { return m_CreateInfo.height; }
        i32 GetChannels() const { return 4; }

        const std::filesystem::path &GetFilepath() { return m_Filepath; }

        bool operator ==(const Texture &other) const 
        { 
            return m_Sampler.Get() == other.m_Sampler.Get() && m_Handle.Get() == other.m_Handle.Get();
        }

        static AssetType GetStaticType() { return AssetType::Texture; }
        virtual AssetType GetType() override { return GetStaticType(); }

    private:
        void *m_Data = nullptr;
        bool m_WithSTBI = false;

        TextureCreateInfo m_CreateInfo;

        std::filesystem::path m_Filepath;
        nvrhi::TextureHandle m_Handle;
        nvrhi::SamplerHandle m_Sampler;
    };

}
