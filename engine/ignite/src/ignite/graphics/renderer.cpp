#include "renderer.hpp"
#include "renderer_2d.hpp"
#include "texture.hpp"
#include "shader.hpp"

#include "environment.hpp"

#include "ignite/core/device/device_manager.hpp"

#include <ranges>

#include "ignite/core/application.hpp"

namespace ignite
{
    Renderer *s_instance = nullptr;

    void ShaderLibrary::Init(nvrhi::GraphicsAPI api)
    {
        m_ShaderMakeOptions.compilerType = ShaderMake::CompilerType_DXC;
        m_ShaderMakeOptions.optimizationLevel = 3;
        m_ShaderMakeOptions.baseDirectory = "resources/shaders/";
        m_ShaderMakeOptions.outputDir = "bin";

        if (api == nvrhi::GraphicsAPI::VULKAN)
            m_ShaderMakeOptions.platformType = ShaderMake::PlatformType_SPIRV;
        else if (api == nvrhi::GraphicsAPI::D3D12)
            m_ShaderMakeOptions.platformType = ShaderMake::PlatformType_DXIL;

        m_ShaderContext = CreateScope<ShaderMake::Context>(&m_ShaderMakeOptions);
    }

    void ShaderLibrary::Compile()
    {
        nvrhi::IDevice *device = Application::GetRenderDevice();
        
        std::vector<Ref<ShaderMake::ShaderContext>> contexts;
        for (auto &shader : m_Shaders | std::views::values)
        {
            for (auto & [context, handle] : shader | std::views::values)
            {
                contexts.push_back(context);
            }
        }

        // compile at once
        m_ShaderContext->CompileShader(contexts);

        // load to NVRHI Shader handle
        for (auto& shader : m_Shaders | std::views::values)
        {
            for (auto &[type, shader] : shader)
            {
                shader.handle = device->createShader(type,
                shader.context->blob.data.data(),
                shader.context->blob.dataSize());
            }
        }
    }

    void ShaderLibrary::Load(const std::string& name, const std::string& filepath)
    {
        if (!Exists(name))
        {
            std::unordered_map<nvrhi::ShaderType, ShaderHandleContext> shader;
            shader[nvrhi::ShaderType::Vertex] = { CreateRef<ShaderMake::ShaderContext>(filepath + ".vertex.hlsl", ShaderMake::ShaderType::Vertex), nullptr };
            shader[nvrhi::ShaderType::Pixel] = { CreateRef<ShaderMake::ShaderContext>(filepath + ".pixel.hlsl", ShaderMake::ShaderType::Pixel), nullptr };
            m_Shaders[name] = shader;
        }
    }

    bool ShaderLibrary::Exists(const std::string& name) const
    {
        return m_Shaders.contains(name);
    }

    std::unordered_map<nvrhi::ShaderType, ShaderHandleContext> ShaderLibrary::Get(const std::string& name)
    {
        if (Exists(name))
            return m_Shaders[name];
        return {};
    }

    ShaderMake::Context *ShaderLibrary::GetContext() const
    {
        return m_ShaderContext.get();
    }

    Renderer::Renderer(DeviceManager *deviceManager, nvrhi::GraphicsAPI api)
    {
        s_instance = this;
        m_GraphicsAPI = api;
        
        nvrhi::IDevice *device = deviceManager->GetDevice();
        nvrhi::CommandListHandle commandList = device->createCommandList();

        {
            TextureCreateInfo textureCI;
            textureCI.format = nvrhi::Format::RGBA8_UNORM;
            textureCI.dimension = nvrhi::TextureDimension::Texture2D;
            textureCI.samplerMode = nvrhi::SamplerAddressMode::ClampToBorder;
            textureCI.width = 1;
            textureCI.height = 1;
            textureCI.flip = false;

            u32 white = 0xFFFFFFFF;
            m_WhiteTexture = Texture::Create(Buffer(&white, sizeof(u32)), textureCI);

            u32 black = 0x00000000;
            m_BlackTexture = Texture::Create(Buffer(&black, sizeof(u32)), textureCI);
            
            commandList->open();
            m_WhiteTexture->Write(commandList);
            m_BlackTexture->Write(commandList);
            commandList->close();

        }
        device->executeCommandList(commandList);

        // Create shaders
        {
            m_ShaderLibrary.Init(m_GraphicsAPI);
            
            m_ShaderLibrary.Load("batch_2d_quad", "batch_2d_quad");
            m_ShaderLibrary.Load("batch_2d_line", "batch_2d_line");
            m_ShaderLibrary.Load("imgui", "imgui");
            m_ShaderLibrary.Load("skybox", "skybox");

            m_ShaderLibrary.Compile();
        }

        // Create binding layouts
        m_BindingLayouts[GBindingLayout::MESH] = device->createBindingLayout(VertexMesh::GetBindingLayoutDesc());
        m_BindingLayouts[GBindingLayout::QUAD2D] = device->createBindingLayout(Vertex2DQuad::GetBindingLayoutDesc());
        m_BindingLayouts[GBindingLayout::LINE] = device->createBindingLayout(Vertex2DLine::GetBindingLayoutDesc());
        m_BindingLayouts[GBindingLayout::ENVIRONMENT] = device->createBindingLayout(Environment::GetBindingLayoutDesc());

        Renderer2D::Init();
    }

    Renderer::~Renderer()
    {
        m_WhiteTexture.reset();
        Renderer2D::Shutdown();
    }

    nvrhi::GraphicsAPI Renderer::GetGraphicsAPI()
    {
        return s_instance->m_GraphicsAPI;
    }

    nvrhi::BindingLayoutHandle Renderer::GetBindingLayout(GBindingLayout type)
    {
        if (s_instance->m_BindingLayouts.contains(type))
            return s_instance->m_BindingLayouts[type];

        return nullptr;
    }

    ShaderLibrary &Renderer::GetShaderLibrary()
    {
        return s_instance->m_ShaderLibrary;
    }

    Ref<Texture> Renderer::GetWhiteTexture()
    {
        return s_instance->m_WhiteTexture;
    }

    Ref<Texture> Renderer::GetBlackTexture()
    {
        return s_instance->m_BlackTexture;
    }
}
