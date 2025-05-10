#pragma once
#include "ignite/core/types.hpp"
#include <ShaderMake/ShaderMake.h>
#include <nvrhi/nvrhi.h>

#include <string>
#include <unordered_map>

namespace ignite
{
    class DeviceManager;
    class Texture;
    class Shader;

    // vertex/pixel shader
    struct VPShader
    {
        nvrhi::ShaderHandle vertex;
        nvrhi::ShaderHandle pixel;
        Ref<ShaderMake::ShaderContext> vertexContext;
        Ref<ShaderMake::ShaderContext> pixelContext;

        VPShader() = default;
        VPShader(nvrhi::IDevice *device, const std::string &filepath);
    };

    class Renderer
    {
    public:
        Renderer() = default;
        Renderer(DeviceManager *deviceManager, nvrhi::GraphicsAPI api);

        ~Renderer();

        static ShaderMake::Context *GetShaderContext();
        
        static Ref<Texture> GetWhiteTexture();
        static Ref<Texture> GetBlackTexture();

        static nvrhi::GraphicsAPI GetGraphicsAPI();

        static VPShader *GetDefaultShader(const std::string &shaderName);
        
    private:

        void LoadDefaultShaders(nvrhi::IDevice *device);

        nvrhi::GraphicsAPI m_GraphicsAPI;
        Scope<ShaderMake::Context> m_ShaderContext = nullptr;
        ShaderMake::Options m_ShaderMakeOptions;

        std::unordered_map<std::string, VPShader> m_Shaders;

        Ref<Texture> m_WhiteTexture;
        Ref<Texture> m_BlackTexture;
    };
}