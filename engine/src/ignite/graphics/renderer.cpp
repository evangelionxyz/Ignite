#include "renderer.hpp"
#include "renderer_2d.hpp"
#include "texture.hpp"

#include "ignite/core/device/device_manager.hpp"

namespace ignite
{
    Ref<Texture> Renderer::whiteTexture;

    void Renderer::Init(DeviceManager *deviceManager)
    {
        nvrhi::CommandListHandle commandList = deviceManager->GetDevice()->createCommandList();
        u32 white = 0xFFFFFFFF;
        whiteTexture = Texture::Create(Buffer(&white, sizeof(white)));
        
        commandList->open();
        whiteTexture->Write(commandList);
        commandList->close();
        deviceManager->GetDevice()->executeCommandList(commandList);

        Renderer2D::Init(deviceManager);
    }

    void Renderer::Shutdown()
    {
        Renderer2D::Shutdown();
    }
}