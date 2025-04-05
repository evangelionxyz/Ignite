#pragma once

#include "core/types.hpp"
#include "core/vfs/vfs.hpp"

#include <nvrhi/nvrhi.h>
#include <memory>

#include <functional>
#include <filesystem>

struct ShaderMacro
{
    std::string name;
    std::string definition;

    ShaderMacro(const std::string &_name, const std::string &_definition)
        : name(_name), definition(_definition)
    {
    }
};

struct StaticShader
{
    void const *pByteCode = nullptr;
    size_t size = 0;
};

class ShaderFactory
{
public:
    ShaderFactory(nvrhi::DeviceHandle device,
        Ref<vfs::IFileSystem> fs,
        const std::filesystem::path &basePath);

    virtual ~ShaderFactory();
    void ClearCache();

    Ref<vfs::IBlob> GetByteCode(const char *filename, const char *entryName);
    nvrhi::ShaderHandle CreateShader(const char *filename, const char *entryName, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);
    nvrhi::ShaderLibraryHandle CreateShaderLibrary(const char *filename, const std::vector<ShaderMacro> *pDefines);
    nvrhi::ShaderHandle CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);
    nvrhi::ShaderHandle CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);
    nvrhi::ShaderLibraryHandle CreateStaticShaderLibrary(StaticShader shader, const std::vector<ShaderMacro> *pDefines);
    nvrhi::ShaderLibraryHandle CreateStaticPlatformShaderLibrary(StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines);

    nvrhi::ShaderHandle CreateAutoShader(const char *filename, const char *entrynName, StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);

    nvrhi::ShaderLibraryHandle CreateAutoShaderLibrary(const char *filename, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines);

    std::pair<const void *, size_t> FindShaderFromHash(u64 hash, std::function<u64(std::pair<const void *, size_t>, nvrhi::GraphicsAPI)> hashGenerator);

private:
    nvrhi::DeviceHandle m_Device;
    std::unordered_map<std::string, Ref<vfs::IBlob>> m_ByteCodeCache;
    Ref<vfs::IFileSystem> m_FS;
    std::filesystem::path m_BasePath;
};