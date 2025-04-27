#pragma once
#include "ignite/core/types.hpp"

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

        void Destroy();

        static Ref<Texture> GetWhiteTexture();
        static nvrhi::GraphicsAPI GetGraphicsAPI();

    private:
        nvrhi::GraphicsAPI m_GraphicsAPI;
        Ref<Texture> m_WhiteTexture;
    };
}