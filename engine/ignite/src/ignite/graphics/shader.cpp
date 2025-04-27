#include "shader.hpp"

#include "ignite/core/application.hpp"

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>

#include <shaderc/shaderc.hpp>

#include <fstream>

namespace ignite
{
    static shaderc_shader_kind ShaderStageToShaderC(ShaderStage stage)
    {
        switch(stage)
        {
            case ShaderStage_Vertex: return shaderc_glsl_vertex_shader;
            case ShaderStage_Fragment: return shaderc_glsl_fragment_shader;
        }
        LOG_ASSERT(false, "Invalid shader stage");
    }

    static const char *GetShaderExtension(nvrhi::GraphicsAPI api)
    {
        switch (api)
        {
            case nvrhi::GraphicsAPI::D3D12: return ".dxil";
            case nvrhi::GraphicsAPI::VULKAN: return ".spirv";
        }
        LOG_ASSERT(false, "Invalid api");
    }


    Shader::Shader(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage)
    {
        LOG_ASSERT(std::filesystem::exists(filepath), "Shader {} file does not exits!: {}", ShaderStageToString(stage), filepath.generic_string());

        CreateShaderCachedDirectoryIfNeeded();

        ShaderData shaderData = CompileOrGetVulkanShader(filepath, stage);

        nvrhi::ShaderDesc shaderDesc;
        shaderDesc.shaderType = ShaderStageToNVRHIShaderType(stage);

        m_Handle = device->createShader(shaderDesc, shaderData.data.data(), shaderData.data.size() * sizeof(u32));
        
        LOG_ASSERT(m_Handle, "Failed to create {} shader: {}", 
            ShaderStageToString(shaderData.stage), 
            filepath.generic_string());
    }

    ShaderData Shader::CompileOrGetVulkanShader(const std::filesystem::path &filepath, ShaderStage stage)
    {
        auto api = Application::GetInstance()->GetCreateInfo().graphicsApi;

        ShaderData shaderData;
        shaderData.stage = stage;

        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        std::filesystem::path cacheDirectory = GetShaderCacheDirectory();
        std::filesystem::path cachedPath = cacheDirectory / (filepath.filename().generic_string() + GetShaderExtension(api));

        std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
        if (in.is_open())
        {
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
        const spirv_cross::Compiler compiler(data);
        auto res = compiler.get_shader_resources();

        LOG_WARN("Shader Reflect - {}", ShaderStageToString(stage));
        LOG_WARN("   {} uniform buffers", res.uniform_buffers.size());
        LOG_WARN("   {} resources", res.sampled_images.size());

        if (!res.uniform_buffers.empty())
        {
            LOG_WARN("Uniform buffers: ");
            for (const auto &[id, typeId, baseTypeId, name] : res.uniform_buffers)
            {
                const auto &bufferType = compiler.get_type(baseTypeId);
                u32 bufferSize = static_cast<u32>(compiler.get_declared_struct_size(bufferType));
                u32 binding = compiler.get_decoration(id, spv::DecorationBinding);
                size_t memberCount = bufferType.member_types.size();

                LOG_WARN("     Name: {}", name);
                LOG_WARN("     Size: {}", bufferSize);
                LOG_WARN("  Binding: {}", binding);
                LOG_WARN("  Members: {}", memberCount);
            }
        }
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

    Ref<Shader> Shader::Create(nvrhi::IDevice *device, const std::filesystem::path &filepath, ShaderStage stage)
    {
        return CreateRef<Shader>(device, filepath, stage);
    }
}