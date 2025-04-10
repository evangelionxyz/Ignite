#include "shader_factory.hpp"
#include "ignite/core/logger.hpp"

#include <ShaderMake/ShaderBlob.h>

ShaderFactory::ShaderFactory(nvrhi::DeviceHandle device, Ref<vfs::IFileSystem> fs, const std::filesystem::path &basePath)
    : m_Device(device), m_FS(fs), m_BasePath(basePath)
{
}

ShaderFactory::~ShaderFactory()
{
}

void ShaderFactory::ClearCache()
{
    m_ByteCodeCache.clear();
}

Ref<vfs::IBlob> ShaderFactory::GetByteCode(const char *filename, const char *entryName)
{
    if (!m_FS)
        return nullptr;
        
    if (!entryName)
        entryName = "main";

    std::string adjustedName = filename;
    {
        size_t pos = adjustedName.find(".hlsl");
        if (pos != std::string::npos)
            adjustedName.erase(pos, 5);

        if (entryName && strcmp(entryName, "main"))
            adjustedName += "_" + std::string(entryName);
    }

    std::filesystem::path shaderFilePath = m_BasePath / (adjustedName + ".dxil");
    std::shared_ptr<vfs::IBlob>& data = m_ByteCodeCache[shaderFilePath.generic_string()];

    if (data)
        return data;

    data = m_FS->ReadFile(shaderFilePath);

    if (!data)
    {
        LOG_ERROR("Couldn't read the binary file for shader {} from {}", filename, shaderFilePath.generic_string().c_str());
        return nullptr;
    }

    return data;
}

nvrhi::ShaderHandle ShaderFactory::CreateShader(const char *filename, const char *entryName, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc)
{
    Ref<vfs::IBlob> byteCode = GetByteCode(filename, entryName);
    if (!byteCode)
        return nullptr;
    
    nvrhi::ShaderDesc descCopy = desc;
    descCopy.entryName = entryName;
    if (descCopy.debugName.empty())
        descCopy.debugName = filename;

    return CreateStaticShader(StaticShader{byteCode->Data(), byteCode->Size()}, pDefines, descCopy);
}

nvrhi::ShaderLibraryHandle ShaderFactory::CreateShaderLibrary(const char *filename, const std::vector<ShaderMacro> *pDefines)
{
    Ref<vfs::IBlob> byteCode = GetByteCode(filename, nullptr);
    if (!byteCode)
        return nullptr;
    return CreateStaticShaderLibrary(StaticShader{byteCode->Data(), byteCode->Size()}, pDefines);
}

nvrhi::ShaderHandle ShaderFactory::CreateStaticShader(StaticShader shader, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc)
{
    if (!shader.pByteCode || !shader.size)
        return nullptr;
    
    std::vector<ShaderMake::ShaderConstant> constants;
    if (pDefines)
    {
        for (const ShaderMacro &define : *pDefines)
        {
            constants.push_back(ShaderMake::ShaderConstant{define.name.c_str(), define.definition.c_str()});
        }
    }

    const void *permutationByteCode = nullptr;
    size_t permutationSize = 0;
    if (!ShaderMake::FindPermutationInBlob(shader.pByteCode, shader.size, constants.data(), u32(constants.size()), &permutationByteCode, &permutationSize))
    {
        const std::string message = ShaderMake::FormatShaderNotFoundMessage(shader.pByteCode, shader.size, constants.data(), u32(constants.size()));
        LOG_ASSERT(false, "{}", message);
        return nullptr;
    }

    return m_Device->createShader(desc, permutationByteCode, permutationSize);

}

nvrhi::ShaderHandle ShaderFactory::CreateStaticPlatformShader(StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc)
{
    StaticShader shader;
    switch(m_Device->getGraphicsAPI())
    {
        case nvrhi::GraphicsAPI::D3D11:
            shader = dxbc;
            break;
        case nvrhi::GraphicsAPI::D3D12:
            shader = dxil;
            break;
        case nvrhi::GraphicsAPI::VULKAN:
            shader = spirv;
            break;
    }

    return CreateStaticShader(shader, pDefines, desc);
}

nvrhi::ShaderLibraryHandle ShaderFactory::CreateStaticShaderLibrary(StaticShader shader, const std::vector<ShaderMacro> *pDefines)
{
    if (!shader.pByteCode || !shader.size)
        return nullptr;

    std::vector<ShaderMake::ShaderConstant> constants;
    if (pDefines)
    {
        for (const ShaderMacro& define : *pDefines)
            constants.push_back(ShaderMake::ShaderConstant{ define.name.c_str(), define.definition.c_str() });
    }
    
    const void* permutationBytecode = nullptr;
    size_t permutationSize = 0;
    if (!ShaderMake::FindPermutationInBlob(shader.pByteCode, shader.size, constants.data(), uint32_t(constants.size()), &permutationBytecode, &permutationSize))
    {
        const std::string message = ShaderMake::FormatShaderNotFoundMessage(shader.pByteCode, shader.size, constants.data(), uint32_t(constants.size()));
        LOG_ASSERT(false, "{}", message);
        return nullptr;
    }

    return m_Device->createShaderLibrary(permutationBytecode, permutationSize);
}

nvrhi::ShaderLibraryHandle ShaderFactory::CreateStaticPlatformShaderLibrary(StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines)
{
    StaticShader shader;
    switch(m_Device->getGraphicsAPI())
    {
        case nvrhi::GraphicsAPI::D3D12:
            shader = dxil;
            break;
        case nvrhi::GraphicsAPI::VULKAN:
            shader = spirv;
            break;
        default:
            break;
    }

    return CreateStaticShaderLibrary(shader, pDefines);
}

nvrhi::ShaderHandle ShaderFactory::CreateAutoShader(const char *filename, const char *entryName, StaticShader dxbc, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines, const nvrhi::ShaderDesc &desc)
{
    nvrhi::ShaderDesc descCopy = desc;
    descCopy.entryName = entryName;
    if (descCopy.debugName.empty())
        descCopy.debugName = filename;

    nvrhi::ShaderHandle shader = CreateStaticPlatformShader(dxbc, dxil, spirv, pDefines, descCopy);
    if (shader)
        return shader;
        
    return CreateShader(filename, entryName, pDefines, desc);
}

nvrhi::ShaderLibraryHandle ShaderFactory::CreateAutoShaderLibrary(const char *filename, StaticShader dxil, StaticShader spirv, const std::vector<ShaderMacro> *pDefines)
{
    nvrhi::ShaderLibraryHandle shader = CreateStaticPlatformShaderLibrary(dxil, spirv, pDefines);
    if (shader)
        return shader;

    return CreateShaderLibrary(filename, pDefines);
}

std::pair<const void *, size_t> ShaderFactory::FindShaderFromHash(u64 hash, std::function<u64(std::pair<const void *, size_t>, nvrhi::GraphicsAPI)> hashGenerator)
{
    for (auto& entry : m_ByteCodeCache)
    {
        const void* shaderBytes = entry.second->Data();
        size_t shaderSize = entry.second->Size();
        uint64_t entryHash = hashGenerator(std::make_pair(shaderBytes, shaderSize), m_Device->getGraphicsAPI());
        if (entryHash == hash)
        {
            return std::make_pair(shaderBytes, shaderSize);
        }
    }
    return std::make_pair(nullptr, 0);
}

