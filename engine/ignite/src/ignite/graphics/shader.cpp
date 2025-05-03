#include "shader.hpp"

#include "renderer.hpp"

#include "ignite/core/application.hpp"

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>

#include <fstream>
#include <iterator>
#include <string>

#ifdef PLATFORM_WINDOWS
#   include <dxcapi.h>
#   include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace ignite
{
    std::string GetShaderCacheDirectory()
    {
        return "resources/shaders/bin/";
    }

    void CreateShaderCachedDirectoryIfNeeded()
    {
        static std::string cachedDirectory = GetShaderCacheDirectory();
        if (!std::filesystem::exists(cachedDirectory))
            std::filesystem::create_directories(cachedDirectory);
    }

    const char *ShaderStageToString(ShaderMake::ShaderType type)
    {
        switch (type)
        {
            case ShaderMake::ShaderType::Vertex: return "Vertex";
            case ShaderMake::ShaderType::Pixel: return "Pixel";
        }

        LOG_ASSERT(false, "Invalid shader type");
        return "Invalid shader type";
    }

    nvrhi::ShaderType ShaderTypeToNVRHIShaderType(ShaderMake::ShaderType type)
    {
        switch (type)
        {
            case ShaderMake::ShaderType::Vertex: return nvrhi::ShaderType::Vertex;
            case ShaderMake::ShaderType::Pixel: return nvrhi::ShaderType::Pixel;
        }

        LOG_ASSERT(false, "Invalid shader stage");
        return nvrhi::ShaderType::None;
    }

    static const char *GetShaderExtension(nvrhi::GraphicsAPI api)
    {
        switch (api)
        {
            case nvrhi::GraphicsAPI::D3D12: return ".dxil";
            case nvrhi::GraphicsAPI::VULKAN: return ".spirv";
        }

        LOG_ASSERT(false, "Invalid Graphics API");
        return "Invalid Graphics API";
    }


    Shader::Shader(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile)
    {
        CreateShaderCachedDirectoryIfNeeded();

        ShaderMake::ShaderBlob blob = CompileOrGetShader(filepath, type, recompile);

        LOG_ASSERT(blob.data.data(), "[Shader] Blob data is not valid");

        nvrhi::ShaderDesc shaderDesc;
        shaderDesc.shaderType = ShaderTypeToNVRHIShaderType(type);

        m_Handle = device->createShader(shaderDesc, blob.data.data(), blob.dataSize());

        LOG_ASSERT(m_Handle, "Failed to create {} shader: {}",
            ShaderStageToString(type),
            filepath.generic_string());
    }

    ShaderMake::ShaderBlob Shader::CompileOrGetShader(const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile)
    {
        ShaderMake::ShaderBlob shaderBlob;

        std::filesystem::path filepathCopy = filepath.filename();
        auto api = Application::GetInstance()->GetCreateInfo().graphicsApi;

        LOG_ASSERT(std::filesystem::exists(filepath), "[Shader] File does not exists! {}", filepath.generic_string().c_str());

        {
            // get or compile shader
            ShaderMake::ShaderContextDesc shaderDesc = ShaderMake::ShaderContextDesc();

            // filepath from filepathCopy
            std::shared_ptr<ShaderMake::ShaderContext> shaderContext = std::make_shared<ShaderMake::ShaderContext>(filepathCopy.generic_string(), type, shaderDesc, recompile);
            ShaderMake::CompileStatus status = Renderer::GetShaderContext()->CompileShader({ shaderContext });

            bool success = status == ShaderMake::CompileStatus::Success;
            LOG_ASSERT(success, "[Shader] failed to get or compile shader");

            // copy blob
            shaderBlob = std::move(shaderContext->blob);
        }

        // print reflect info (only SPIRV file)
        if (api == nvrhi::GraphicsAPI::VULKAN)
        {
            SPIRVReflect(type, shaderBlob);
        }

        return shaderBlob;
    }

    void Shader::SPIRVReflect(ShaderMake::ShaderType type, const ShaderMake::ShaderBlob &blob)
    {
        if (blob.data.size() % sizeof(uint32_t) != 0)
            throw std::runtime_error("Shader blob size is not aligned to 4 bytes");

        const uint32_t *ptr = reinterpret_cast<const uint32_t *>(blob.data.data());
        size_t wordCount = blob.data.size() / sizeof(uint32_t);
        std::vector<uint32_t> dataBlob(ptr, ptr + wordCount);

        spirv_cross::Compiler compiler(dataBlob);
        spirv_cross::ShaderResources res = compiler.get_shader_resources();

        LOG_WARN("Shader Reflect - {}", ShaderStageToString(type));

        // --- Uniform Buffers ---
        LOG_WARN("   {} uniform buffers", res.uniform_buffers.size());
        for (const auto &ubo : res.uniform_buffers)
        {
            const auto &type = compiler.get_type(ubo.base_type_id);
            u32 size = static_cast<u32>(compiler.get_declared_struct_size(type));
            u32 binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
            size_t memberCount = type.member_types.size();

            LOG_INFO("  [UBO] Name: {}, Set: {}, Binding: {}, Size: {}, Members: {}", ubo.name, set, binding, size, memberCount);
        }

        // --- Sampled Images (combined or separate textures) ---
        LOG_WARN("   {} sampled images", res.sampled_images.size());
        for (const auto &image : res.sampled_images)
        {
            u32 binding = compiler.get_decoration(image.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);

            LOG_INFO("  [Texture] Name: {}, Set: {}, Binding: {}", image.name, set, binding);
        }

        // --- Separate Samplers ---
        LOG_WARN("   {} separate samplers", res.separate_samplers.size());
        for (const auto &sampler : res.separate_samplers)
        {
            u32 binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);

            LOG_INFO("  [Sampler] Name: {}, Set: {}, Binding: {}", sampler.name, set, binding);
        }

        // --- Separate Images (non-combined, i.e., texture2D) ---
        LOG_WARN("   {} separate images", res.separate_images.size());
        for (const auto &image : res.separate_images)
        {
            u32 binding = compiler.get_decoration(image.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);

            LOG_INFO("  [Separate Image] Name: {}, Set: {}, Binding: {}", image.name, set, binding);
        }

        // --- Push Constants ---
        LOG_WARN("   {} push constants", res.push_constant_buffers.size());
        for (const auto &pcb : res.push_constant_buffers)
        {
            const auto &type = compiler.get_type(pcb.base_type_id);
            u32 size = static_cast<u32>(compiler.get_declared_struct_size(type));

            LOG_INFO("  [PushConstant] Name: {}, Size: {}", pcb.name, size);
        }
    }

    Ref<Shader> Shader::Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderMake::ShaderType type, bool recompile)
    {
        Ref<Shader> returnShader = CreateRef<Shader>(device, filepath, type, recompile);
        if (returnShader->GetHandle() == nullptr)
            return nullptr;

        return returnShader;
    }
}
