#include <ignite/entry_point.hpp>
#include <ignite/core/application.hpp>
#include "editor_layer.hpp"

class EditorApp final : public ignite::Application
{
public:
    explicit EditorApp(const ignite::ApplicationCreateInfo &createInfo)
        : Application(createInfo)
    {
        PushLayer(new ignite::EditorLayer("Ignite Editor Layer"));
    }
};

namespace ignite
{
    Application *CreateApplication(const ApplicationCommandLineArgs args)
    {
        ApplicationCreateInfo createInfo;
        createInfo.cmdLineArgs = args;
        createInfo.width = 1280;
        createInfo.height = 720;
        createInfo.useGui = true;
        createInfo.graphicsApi = nvrhi::GraphicsAPI::D3D12;
        return new EditorApp(createInfo);
    }
}

