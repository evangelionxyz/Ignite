#include "renderer.hpp"
#include "renderer_2d.hpp"

void Renderer::Init(DeviceManager *deviceManager)
{
    Renderer2D::Init(deviceManager);
}

void Renderer::Shutdown()
{
    Renderer2D::Shutdown();
}

