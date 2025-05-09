#include "renderer.hpp"
#include "renderer_2d.hpp"
#include "texture.hpp"
#include "shader.hpp"

#include "ignite/core/device/device_manager.hpp"

namespace ignite
{
    Renderer *s_instance = nullptr;

    VPShader::VPShader(nvrhi::IDevice *device, const std::string &filename)
    {
        vertexContext = CreateRef<ShaderMake::ShaderContext>(filename + ".vertex.hlsl", ShaderMake::ShaderType::Vertex);
        pixelContext = CreateRef<ShaderMake::ShaderContext>(filename + ".pixel.hlsl", ShaderMake::ShaderType::Pixel);
    }

    Renderer::Renderer(DeviceManager *deviceManager, nvrhi::GraphicsAPI api)
    {
        s_instance = this;
        m_GraphicsAPI = api;
        
        nvrhi::IDevice *device = deviceManager->GetDevice();
        nvrhi::CommandListHandle commandList = device->createCommandList();

        {
            u32 white = 0xFFFFFFFF;
            m_WhiteTexture = Texture::Create(device, Buffer(&white, sizeof(white)));
            
            commandList->open();
            m_WhiteTexture->Write(commandList);
            commandList->close();
        }

        // Create shader make
        {
            m_ShaderMakeOptions.compilerType = ShaderMake::CompilerType_DXC;
            m_ShaderMakeOptions.optimizationLevel = 3;
            m_ShaderMakeOptions.baseDirectory = "resources/shaders/";
            m_ShaderMakeOptions.outputDir = "bin";

            if (m_GraphicsAPI == nvrhi::GraphicsAPI::VULKAN)
                m_ShaderMakeOptions.platformType = ShaderMake::PlatformType_SPIRV;
            else if (m_GraphicsAPI == nvrhi::GraphicsAPI::D3D12)
                m_ShaderMakeOptions.platformType = ShaderMake::PlatformType_DXIL;

            m_ShaderContext = CreateScope<ShaderMake::Context>(&m_ShaderMakeOptions);
        }

        // initialize default shaders
        LoadDefaultShaders(device);
        
        device->executeCommandList(commandList);

        Renderer2D::Init(device);
    }

    Renderer::~Renderer()
    {
        m_WhiteTexture.reset();
        Renderer2D::Shutdown();
    }

    ShaderMake::Context *Renderer::GetShaderContext()
    {
        return s_instance->m_ShaderContext.get();
    }

    nvrhi::GraphicsAPI Renderer::GetGraphicsAPI()
    {
        return s_instance->m_GraphicsAPI;
    }

    VPShader *Renderer::GetDefaultShader(const std::string &shaderName)
    {
        if (s_instance->m_Shaders.contains(shaderName))
            return &s_instance->m_Shaders[shaderName];
        return nullptr;
    }

    void Renderer::LoadDefaultShaders(nvrhi::IDevice *device)
    {
        // create shaders
        m_Shaders["quadBatch2D"] = VPShader(device, "quad_batch_2d");
        m_Shaders["imgui"] = VPShader(device, "imgui");
        m_Shaders["default_mesh"] = VPShader(device, "default_mesh");
        m_Shaders["skybox"] = VPShader(device, "skybox");

        std::vector<Ref<ShaderMake::ShaderContext>> contexts;
        for (auto &shader : m_Shaders)
        {
            contexts.push_back(shader.second.vertexContext);
            contexts.push_back(shader.second.pixelContext);
        }

        // compile at once
        m_ShaderContext->CompileShader(contexts);

        // load to NVRHI Shader handle
        for (auto &[name, shader] : m_Shaders)
        {
            shader.vertex = device->createShader(nvrhi::ShaderType::Vertex,
                shader.vertexContext->blob.data.data(),
                shader.vertexContext->blob.dataSize());

            shader.pixel = device->createShader(nvrhi::ShaderType::Pixel,
                shader.pixelContext->blob.data.data(),
                shader.pixelContext->blob.dataSize());
        }
    }

    Ref<Texture> Renderer::GetWhiteTexture()
    {
        return s_instance->m_WhiteTexture;
    }
}