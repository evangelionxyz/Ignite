#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/logger.hpp"

#include <filesystem>
#include <string>
#include <vector>
#include <ShaderMake/ShaderMake.h>
#include <fstream>
#include <nvrhi/nvrhi.h>

#include <initializer_list>

namespace ignite
{
    static std::string GetShaderCacheDirectory();
    static void CreateShaderCachedDirectoryIfNeeded();

    static nvrhi::ShaderType GetNVRHIShaderType(ShaderMake::ShaderType type)
    {
        switch (type)
        {
        case ShaderMake::ShaderType::Vertex: return nvrhi::ShaderType::Vertex;
        case ShaderMake::ShaderType::Pixel: return nvrhi::ShaderType::Pixel;
        case ShaderMake::ShaderType::Geometry: return nvrhi::ShaderType::Geometry;
        }

        LOG_ASSERT(false, "Invalid shader stage");
        return nvrhi::ShaderType::None;
    }

    static ShaderMake::ShaderType GetShaderMakeShaderType(nvrhi::ShaderType type)
    {
        switch (type)
        {
        case nvrhi::ShaderType::Vertex: return ShaderMake::ShaderType::Vertex;
        case nvrhi::ShaderType::Pixel: return ShaderMake::ShaderType::Pixel;
        case nvrhi::ShaderType::Geometry: return ShaderMake::ShaderType::Geometry;
        }

        LOG_ASSERT(false, "Invalid shader stage");
        return ShaderMake::ShaderType::Vertex;
    }

    class Shader
    {
    public:
        Shader() = default;
        Shader(const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile = false);

        static ShaderMake::ShaderBlob CompileOrGetShader(const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile);
        static Ref<Shader> Create(const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile = false);
        nvrhi::ShaderHandle GetHandle() { return m_Handle; }

        static void SPIRVReflect(ShaderMake::ShaderType type, const ShaderMake::ShaderBlob &blob);
    private:
        nvrhi::ShaderHandle m_Handle = nullptr;
    };
}
