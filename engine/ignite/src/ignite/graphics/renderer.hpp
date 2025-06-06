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
    struct ShaderHandleContext
    {
        Ref<ShaderMake::ShaderContext> context;
        nvrhi::ShaderHandle handle;
    };

    enum class GBindingLayout
    {
        MESH, QUAD2D, LINE, ENVIRONMENT
    };

    class ShaderLibrary
    {
    public:
        void Init(nvrhi::GraphicsAPI api);
        void Compile();
        void Load(const std::string &name, const std::string &filepath);
        bool Exists(const std::string &name) const;
        
        std::unordered_map<nvrhi::ShaderType, ShaderHandleContext> Get(const std::string &name);

        ShaderMake::Context *GetContext() const;

    private:
        std::unordered_map<std::string, std::unordered_map<nvrhi::ShaderType, ShaderHandleContext>> m_Shaders;
        Scope<ShaderMake::Context> m_ShaderContext = nullptr;
        ShaderMake::Options m_ShaderMakeOptions;
    };

    class Renderer
    {
    public:
        Renderer() = default;
        Renderer(DeviceManager *deviceManager, nvrhi::GraphicsAPI api);

        ~Renderer();
        
        static Ref<Texture> GetWhiteTexture();
        static Ref<Texture> GetBlackTexture();
        static nvrhi::GraphicsAPI GetGraphicsAPI();
        static nvrhi::BindingLayoutHandle GetBindingLayout(GBindingLayout type);

        static ShaderLibrary &GetShaderLibrary();
        
    private:
        nvrhi::GraphicsAPI m_GraphicsAPI;
        ShaderLibrary m_ShaderLibrary;

        std::unordered_map<GBindingLayout, nvrhi::BindingLayoutHandle> m_BindingLayouts;

        Ref<Texture> m_WhiteTexture;
        Ref<Texture> m_BlackTexture;
    };
}