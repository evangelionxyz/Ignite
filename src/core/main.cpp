#include "application.hpp"
#include "logger.hpp"

// #ifdef PLATFORM_WINDOWS
// int WINAPI WinMAin(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// #else
// #endif
int main(int argc, const char **argv)
{
    Logger::Init();

    nvrhi::GraphicsAPI graphicsApi = nvrhi::GraphicsAPI::D3D12;
    DeviceManager *deviceManager = DeviceManager::Create(graphicsApi);

    DeviceCreationParameters deviceParams;
#ifdef _DEBUG
    deviceParams.enableDebugRuntime = true;
    deviceParams.enableNvrhiValidationLayer= true;
#endif

    const char *windowTitle = "Ignite Editor";
    if (!deviceManager->CreateWindowDeviceAndSwapChain(deviceParams, windowTitle))
    {
        LOG_ASSERT(false, "Cannot initalize a graphics device with the requested parameters");
        return -1;
    }

    Application app(deviceManager);
    if (app.Init())
    {
        deviceManager->AddRenderPassToBack(&app);
        deviceManager->RunMessageLoop();
        deviceManager->RemoveRenderPass(&app);
        app.Shutdown();
    }

    deviceManager->Shutdown();
    
    delete deviceManager;
    Logger::Shutdown();
    return 0;
}