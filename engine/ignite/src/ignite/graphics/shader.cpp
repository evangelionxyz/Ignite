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
        return "resources/shaders/cache/";
    }

    const char *GetHLSLDirectory()
    {
        return "resources/shaders/hlsl/";
    }

    const char *GetGLSLDirectory()
    {
        return "resources/shaders/glsl/";
    }

    void CreateShaderCachedDirectoryIfNeeded()
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
        LOG_ASSERT(std::filesystem::exists(filepath), "{} shader file does not exits!: {}", ShaderStageToString(stage), filepath.generic_string());

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
        auto api = Application::GetInstance()->GetCreateInfo().graphicsApi;

        ShaderData shaderData;
        shaderData.stage = stage;

        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        options.AddMacroDefinition("VULKAN"); // if needed in the shader

        std::filesystem::path cacheDirectory = GetShaderCacheDirectory();
        std::filesystem::path cachedPath = cacheDirectory / (filepath.filename().generic_string() + GetShaderExtension(api));

        std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
        if (in.is_open() && !recompile)
        {
            LOG_INFO("Get {} shader from binary: {}", ShaderStageToString(stage), filepath.generic_string());

            // get

            in.seekg(0, std::ios::end);
            auto size = in.tellg(); // get data size in bytes

            in.seekg(0, std::ios::beg);

            auto &data = shaderData.data;
            data.resize(size / sizeof(u32));
            in.read(reinterpret_cast<char *>(data.data()), size);

            in.close();
        }
        else
        {
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

            // create HLSL shader
            std::filesystem::path hlslPath = GetHLSLDirectory() + filepath.filename().generic_string();
            ConvertSpirvToHLSL(data, hlslPath.generic_string());
        }

        // print reflect info
        Reflect(stage, shaderData.data);

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

            LOG_WARN("  [UBO] Name: {}, Set: {}, Binding: {}, Size: {}, Members: {}", ubo.name, set, binding, size, memberCount);
        }

        // --- Sampled Images (combined or separate textures) ---
        LOG_WARN("   {} sampled images", res.sampled_images.size());
        for (const auto &image : res.sampled_images)
        {
            u32 binding = compiler.get_decoration(image.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);

            LOG_WARN("  [Texture] Name: {}, Set: {}, Binding: {}", image.name, set, binding);
        }

        // --- Separate Samplers ---
        LOG_WARN("   {} separate samplers", res.separate_samplers.size());
        for (const auto &sampler : res.separate_samplers)
        {
            u32 binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);

            LOG_WARN("  [Sampler] Name: {}, Set: {}, Binding: {}", sampler.name, set, binding);
        }

        // --- Separate Images (non-combined, i.e., texture2D) ---
        LOG_WARN("   {} separate images", res.separate_images.size());
        for (const auto &image : res.separate_images)
        {
            u32 binding = compiler.get_decoration(image.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);

            LOG_WARN("  [Separate Image] Name: {}, Set: {}, Binding: {}", image.name, set, binding);
        }

        // --- Push Constants ---
        LOG_WARN("   {} push constants", res.push_constant_buffers.size());
        for (const auto &pcb : res.push_constant_buffers)
        {
            const auto &type = compiler.get_type(pcb.base_type_id);
            u32 size = static_cast<u32>(compiler.get_declared_struct_size(type));

            LOG_WARN("  [PushConstant] Name: {}, Size: {}", pcb.name, size);
        }

        // - storage_buffers
        // - storage_images
        // - subpass_inputs
    }


    std::string Shader::ConvertSpirvToHLSL(const std::vector<u32> &data, const std::string &filepath)
    {
        spirv_cross::CompilerHLSL compiler(data);
        spirv_cross::CompilerHLSL::Options options;
        options.shader_model = 60; // shader model 6.0
        compiler.set_hlsl_options(options);

        std::string shaderSource = compiler.compile();
        std::ofstream outfile(filepath + ".hlsl");
        outfile << shaderSource;

        return shaderSource;
    }

    Ref<Shader> Shader::Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage, bool recompile)
    {
        return CreateRef<Shader>(device, filepath, stage, recompile);
    }
}