#pragma once
#include "ignite/core/types.hpp"

namespace ignite
{
    class DeviceManager;
    class Texture;

    class Renderer
    {
    public:
        static void Init(DeviceManager *deviceManager);
        static void Shutdown();
        static Ref<Texture> whiteTexture;
    };
}