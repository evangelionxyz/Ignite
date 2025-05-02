#include "shader.hpp"

#include "ignite/core/application.hpp"

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <shaderc/shaderc.hpp>

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
    const char *GetShaderCacheDirectory()
    {
        return "resources/shaders/bin/";
    }

    void CreateShaderCachedDirectoryIfNeeded()
    {
        static std::string cachedDirectory = GetShaderCacheDirectory();
        if (!std::filesystem::exists(cachedDirectory))
            std::filesystem::create_directories(cachedDirectory);
    }

    const char *ShaderStageToString(ShaderStage stage)
    {
        switch(stage)
        {
            case ShaderStage_Vertex: return "Vertex";
            case ShaderStage_Fragment: return "Fragment";
        }

        LOG_ASSERT(false, "Invalid shader stage");
        return "Invalid shader stage";
    }

    nvrhi::ShaderType ShaderStageToNVRHIShaderType(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage_Vertex: return nvrhi::ShaderType::Vertex;
            case ShaderStage_Fragment: return nvrhi::ShaderType::Pixel;
        }
        
        LOG_ASSERT(false, "Invalid shader stage");
        return nvrhi::ShaderType::None;
    }

    static shaderc_shader_kind ShaderStageToShaderC(ShaderStage stage)
    {
        switch(stage)
        {
            case ShaderStage_Vertex: return shaderc_glsl_vertex_shader;
            case ShaderStage_Fragment: return shaderc_glsl_fragment_shader;
        }
        
        LOG_ASSERT(false, "Invalid shader stage");
        return shaderc_shader_kind(0);
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


    Shader::Shader(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile)
    {
        CreateShaderCachedDirectoryIfNeeded();

        ShaderData shaderData = CompileOrGetVulkanShader(filepath, stage, recompile);

        nvrhi::ShaderDesc shaderDesc;
        shaderDesc.shaderType = ShaderStageToNVRHIShaderType(stage);

        m_Handle = device->createShader(shaderDesc, shaderData.data.data(), shaderData.data.size() * sizeof(u32));
        
        LOG_ASSERT(m_Handle, "Failed to create {} shader: {}", 
            ShaderStageToString(shaderData.stage), 
            filepath.generic_string());
    }

    ShaderData Shader::CompileOrGetVulkanShader(const std::filesystem::path &filepath, ShaderStage stage, bool recompile)
    {
        std::filesystem::path filepathCopy = filepath;

        if (filepathCopy.extension() == ".hlsl")
        {
            filepathCopy.replace_extension(""); // remove extension
        }

        auto api = Application::GetInstance()->GetCreateInfo().graphicsApi;

        ShaderData shaderData;
        shaderData.stage = stage;

        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        options.AddMacroDefinition("VULKAN"); // if needed in the shader

        std::filesystem::path cacheDirectory = GetShaderCacheDirectory();
        std::filesystem::path cachedPath = cacheDirectory / (filepathCopy.filename().generic_string() + GetShaderExtension(api));

        std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
        if (in.is_open() && !recompile)
        {
            LOG_ASSERT(std::filesystem::exists(filepath), "[Shader] File does not exists! {}", cachedPath.generic_string().c_str());

            LOG_INFO("Get {} shader from binary: {}", ShaderStageToString(stage), cachedPath.generic_string());

            // get
            in.seekg(0, std::ios::end);
            auto size = in.tellg(); // get data size in bytes

            in.seekg(0, std::ios::beg);

            auto &data = shaderData.data;
            data.resize(size / sizeof(u32));
            in.read(reinterpret_cast<char *>(data.data()), size);

            in.close();

            LOG_INFO("Shader binary loaded!");
        }
        else
        {
            LOG_ASSERT(std::filesystem::exists(filepath), "[Shader] File does not exists! {}", filepath.generic_string().c_str());

            LOG_INFO("Compiling {} shader to binary: {}", ShaderStageToString(stage), filepath.generic_string());

            // compile
            std::string sourceCode;
            std::stringstream ss;

            std::ifstream sourceIn(filepath, std::ios::in);
            if (sourceIn.is_open())
            {
                ss << sourceIn.rdbuf();
                sourceIn.close();
                sourceCode = ss.str();
            }

            shaderc_shader_kind shaderKind = (shaderc_shader_kind)ShaderStageToShaderC(stage);
    
            shaderc::Compiler compiler;
            shaderc::SpvCompilationResult spvModule = compiler.CompileGlslToSpv(sourceCode, shaderKind, filepath.generic_string().c_str());
    
            bool success = spvModule.GetCompilationStatus() == shaderc_compilation_status_success;
            LOG_ASSERT(success, "[Shader] failed to compile vulkan {}", spvModule.GetErrorMessage());
    
            // store the module to the shader data
            shaderData.data = std::vector<u32>(spvModule.cbegin(), spvModule.cend());
    
            // save it to cache
            std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
            auto &data = shaderData.data;
            out.write(reinterpret_cast<char *>(data.data()), data.size() * sizeof(u32));

            out.flush();
            out.close();

            LOG_INFO("Shader compiled to binary!");
        }

        // print reflect info (only SPIRV file)
        if (api == nvrhi::GraphicsAPI::VULKAN)
        {
            Reflect(stage, shaderData.data);
        }

        return shaderData;
    }

    void Shader::Reflect(ShaderStage stage, const std::vector<u32> &data)
    {
        spirv_cross::Compiler compiler(data);
        spirv_cross::ShaderResources res = compiler.get_shader_resources();

        LOG_WARN("Shader Reflect - {}", ShaderStageToString(stage));

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

        // - storage_buffers
        // - storage_images
        // - subpass_inputs
    }

    Ref<Shader> Shader::Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile)
    {
        Ref<Shader> returnShader = CreateRef<Shader>(device, filepath, stage, recompile);
        if (returnShader->GetHandle() == nullptr)
            return nullptr;

        return returnShader;
    }
}
