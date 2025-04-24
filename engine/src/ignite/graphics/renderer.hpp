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
        static nvrhi::GraphicsAPI GetGraphicsAPI();
        static void Init(DeviceManager *deviceManager, nvrhi::GraphicsAPI api);
        static void Shutdown();
        static Ref<Texture> whiteTexture;

    private:
        static nvrhi::GraphicsAPI m_GraphicsAPI;
    };
}