#include "ignite_editor_app.hpp"
#include "core/logger.hpp"

// #ifdef PLATFORM_WINDOWS
// int WINAPI WinMAin(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// #else
// #endif
int main(int argc, const char **argv)
{
    Logger::Init();
    {
        ApplicationCreateInfo createInfo;
        createInfo.appName = "Ignite Editor";
        createInfo.graphicsApi = nvrhi::GraphicsAPI::D3D12;
        EditorApp app(createInfo);
        app.Run();
    }
    
    Logger::Shutdown();

    return 0;
}