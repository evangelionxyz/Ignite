#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/logger.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <fstream>

#include <nvrhi/nvrhi.h>

namespace ignite
{
    enum ShaderStage
    {
        ShaderStage_Vertex,
        ShaderStage_Fragment
    };

    struct ShaderData
    {
        ShaderStage stage;
        std::vector<u32> data;
    };

    static const char *GetShaderCacheDirectory()
    {
        return "resources/shaders/cache/";
    }

    static const char *GetHLSLDirectory()
    {
        return "resources/shaders/hlsl/";
    }

    static const char *GetGLSLDirectory()
    {
        return "resources/shaders/glsl/";
    }

    static void CreateShaderCachedDirectoryIfNeeded()
    {
        static std::string cachedDirectory = GetShaderCacheDirectory();
        static std::string hlslDirectory = GetHLSLDirectory();
        static std::string glslDirectory = GetGLSLDirectory();

        if (!std::filesystem::exists(cachedDirectory))
            std::filesystem::create_directories(cachedDirectory);
        if (!std::filesystem::exists(hlslDirectory))
            std::filesystem::create_directories(hlslDirectory);
        if (!std::filesystem::exists(glslDirectory))
            std::filesystem::create_directories(glslDirectory);
    }

    static const char *ShaderStageToString(ShaderStage stage)
    {
        switch(stage)
        {
            case ShaderStage_Vertex: return "Vertex";
            case ShaderStage_Fragment: return "Fragment";
        }

        LOG_ASSERT(false, "Invalid shader stage");
        return "Invalid shader stage";
    }

    static nvrhi::ShaderType ShaderStageToNVRHIShaderType(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage_Vertex: return nvrhi::ShaderType::Vertex;
            case ShaderStage_Fragment: return nvrhi::ShaderType::Pixel;
        }
        
        LOG_ASSERT(false, "Invalid shader stage");
        return nvrhi::ShaderType::None;
    }

    class  Shader
    {
    public:
        Shader() = default;
        Shader(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile = false);

        static ShaderData CompileOrGetVulkanShader(const std::filesystem::path &filepath, ShaderStage stage, bool recompile);
        static void Reflect(ShaderStage stage, const std::vector<u32> &data);

        static std::string ConvertSpirvToHLSL(const std::vector<u32> &data, const std::string &filepath);

        nvrhi::ShaderHandle GetHandle() { return m_Handle; }

        static Ref<Shader> Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile = false);

    private:
        nvrhi::ShaderHandle m_Handle;
    };
}