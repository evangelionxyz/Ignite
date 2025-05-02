#include "renderer.hpp"
#include "renderer_2d.hpp"
#include "texture.hpp"

#include "ignite/core/device/device_manager.hpp"

namespace ignite
{
    Renderer *s_instance = nullptr;

    Renderer::Renderer(DeviceManager *deviceManager, nvrhi::GraphicsAPI api)
    {
        s_instance = this;

        m_GraphicsAPI = api;
        
        nvrhi::CommandListHandle commandList = deviceManager->GetDevice()->createCommandList();
        {
            u32 white = 0xFFFFFFFF;
            m_WhiteTexture = Texture::Create(Buffer(&white, sizeof(white)));
            
            commandList->open();
            m_WhiteTexture->Write(commandList);
            commandList->close();
        }
        
        deviceManager->GetDevice()->executeCommandList(commandList);
        
        Renderer2D::Init(deviceManager);
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

    Ref<Texture> Renderer::GetWhiteTexture()
    {
        return s_instance->m_WhiteTexture;
    }
}