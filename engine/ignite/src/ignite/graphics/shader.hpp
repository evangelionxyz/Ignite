#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/logger.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <ShaderMake/ShaderMake.h>

#include <fstream>

#include <nvrhi/nvrhi.h>

namespace ignite
{
    static std::string GetShaderCacheDirectory();
    static void CreateShaderCachedDirectoryIfNeeded();
    static nvrhi::ShaderType ShaderTypeToNVRHIShaderType(ShaderMake::ShaderType type);

    class  Shader
    {
    public:
        Shader() = default;
        Shader(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile = false);

        static ShaderMake::ShaderBlob CompileOrGetShader(const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile);
        static void SPIRVReflect(ShaderMake::ShaderType type, const ShaderMake::ShaderBlob &blob);
        static Ref<Shader> Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile = false);

        nvrhi::ShaderHandle GetHandle() { return m_Handle; }

    private:
        nvrhi::ShaderHandle m_Handle = nullptr;
    };
}
