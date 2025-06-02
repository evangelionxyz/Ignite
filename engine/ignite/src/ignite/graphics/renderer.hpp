#pragma once
#include "ignite/core/types.hpp"
#include "graphics_pipeline.hpp"

#include <nvrhi/nvrhi.h>
#include <ShaderMake/ShaderMake.h>

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

    enum class GPipelines
    {
        RENDERER_2D_QUAD,
        RENDERER_2D_LINE,
        DEFAULT_3D_ENV,
        DEFAULT_3D_MESH
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
        static void CreatePipelines(nvrhi::IFramebuffer *framebuffer);

        static nvrhi::GraphicsAPI GetGraphicsAPI();

        static VPShader *GetDefaultShader(const std::string &shaderName);
        static Ref<GraphicsPipeline> GetPipeline(GPipelines gpipeline);
        
    private:

        void LoadDefaultShaders(nvrhi::IDevice *device);
        void InitPipelines();

        nvrhi::GraphicsAPI m_GraphicsAPI;
        Scope<ShaderMake::Context> m_ShaderContext = nullptr;
        ShaderMake::Options m_ShaderMakeOptions;

        std::unordered_map<std::string, VPShader> m_Shaders;
        std::unordered_map<GPipelines, Ref<GraphicsPipeline>> m_Pipelines;

        Ref<Texture> m_WhiteTexture;
        Ref<Texture> m_BlackTexture;
    };
}