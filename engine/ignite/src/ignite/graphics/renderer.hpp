#pragma once
#include "ignite/core/types.hpp"

#include <ShaderMake/ShaderMake.h>

#include "nvrhi/nvrhi.h"

namespace ignite
{
    class DeviceManager;
    class Texture;

    class Renderer
    {
    public:
        Renderer() = default;
        Renderer(DeviceManager *deviceManager, nvrhi::GraphicsAPI api);

        ~Renderer();

        void CreateShaderContext(ShaderMake::Options *options);
        static ShaderMake::Context *GetShaderContext();

        static Ref<Texture> GetWhiteTexture();
        static nvrhi::GraphicsAPI GetGraphicsAPI();
        
    private:
        nvrhi::GraphicsAPI m_GraphicsAPI;
        Scope<ShaderMake::Context> m_ShaderContext = nullptr;

        Ref<Texture> m_WhiteTexture;
    };
}