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

    static const char *GetShaderCacheDirectory();
    static void CreateShaderCachedDirectoryIfNeeded();
    static const char *ShaderStageToString(ShaderStage stage);
    static nvrhi::ShaderType ShaderStageToNVRHIShaderType(ShaderStage stage);

    class  Shader
    {
    public:
        Shader() = default;
        Shader(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile = false);

        static ShaderData CompileOrGetVulkanShader(const std::filesystem::path &filepath, ShaderStage stage, bool recompile);
        static void Reflect(ShaderStage stage, const std::vector<u32> &data);

        nvrhi::ShaderHandle GetHandle() { return m_Handle; }
        static Ref<Shader> Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile = false);

    private:
        nvrhi::ShaderHandle m_Handle = nullptr;
    };
}
