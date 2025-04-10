#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/vfs/vfs.hpp"

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

#if IGNITE_WITH_DX11 && IGNITE_WITH_STATIC_SHADERS
#define IGNITE_MAKE_DXBC_SHADER(symbol) StaticShader{symbol,sizeof(symbol)}
#else
#define IGNITE_MAKE_DXBC_SHADER(symbol) StaticShader()
#endif

#if IGNITE_WITH_DX12 && IGNITE_WITH_STATIC_SHADERS
#define IGNITE_MAKE_DXIL_SHADER(symbol) StaticShader{symbol,sizeof(symbol)}
#else
#define IGNITE_MAKE_DXIL_SHADER(symbol) StaticShader()
#endif

#if IGNITE_WITH_VULKAN && IGNITE_WITH_STATIC_SHADERS
#define IGNITE_MAKE_SPIRV_SHADER(symbol) StaticShader{symbol,sizeof(symbol)}
#else
#define IGNITE_MAKE_SPIRV_SHADER(symbol) StaticShader()
#endif

// Macro to use with ShaderFactory::CreateStaticPlatformShader.
// If there are symbols g_MyShader_dxbc, g_MyShader_dxil, g_MyShader_spirv - just use:
//      CreateStaticPlatformShader(IGNITE_MAKE_PLATFORM_SHADER(g_MyShader), defines, shaderDesc);
// and all available platforms will be resolved automatically.
#define IGNITE_MAKE_PLATFORM_SHADER(basename) IGNITE_MAKE_DXBC_SHADER(basename##_dxbc), IGNITE_MAKE_DXIL_SHADER(basename##_dxil), IGNITE_MAKE_SPIRV_SHADER(basename##_spirv)

// Similar to IGNITE_MAKE_PLATFORM_SHADER but for libraries - they are not available on DX11/DXBC.
//      CreateStaticPlatformShaderLibrary(IGNITE_MAKE_PLATFORM_SHADER_LIBRARY(g_MyShaderLibrary), defines);
#define IGNITE_MAKE_PLATFORM_SHADER_LIBRARY(basename) IGNITE_MAKE_DXIL_SHADER(basename##_dxil), IGNITE_MAKE_SPIRV_SHADER(basename##_spirv)

class ShaderFactory
{
public:
    ShaderFactory(nvrhi::DeviceHandle device, Ref<vfs::IFileSystem> fs, const std::filesystem::path &basePath);

    virtual ~ShaderFactory();
    void ClearCache();

    Ref<vfs::IBlob> GetByteCode(const char *filename, const char *entryName);
    nvrhi::ShaderHandle CreateShader(const char *filename, const char *entryName, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);
    nvrhi::ShaderLibraryHandle CreateShaderLibrary(const char *filename, const std::vector<ShaderMacro> *pDefines);
    nvrhi::ShaderHandle CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);
    nvrhi::ShaderHandle CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);
    nvrhi::ShaderLibraryHandle CreateStaticShaderLibrary(StaticShader shader, const std::vector<ShaderMacro> *pDefines);
    nvrhi::ShaderLibraryHandle CreateStaticPlatformShaderLibrary(StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines);

    nvrhi::ShaderHandle CreateAutoShader(const char *filename, const char *entryName, StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc);

    nvrhi::ShaderLibraryHandle CreateAutoShaderLibrary(const char *filename, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines);

    std::pair<const void *, size_t> FindShaderFromHash(u64 hash, std::function<u64(std::pair<const void *, size_t>, nvrhi::GraphicsAPI)> hashGenerator);

private:
    nvrhi::DeviceHandle m_Device;
    std::unordered_map<std::string, Ref<vfs::IBlob>> m_ByteCodeCache;
    Ref<vfs::IFileSystem> m_FS;
    std::filesystem::path m_BasePath;
};