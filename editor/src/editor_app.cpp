#include <ignite/entry_point.hpp>
#include <ignite/core/application.hpp>
#include "editor_layer.hpp"

class EditorApp final : public Application
{
public:
    explicit EditorApp(const ApplicationCreateInfo &createInfo)
        : Application(createInfo)
    {
        PushLayer(new IgniteEditorLayer("Ignite Editor Layer"));
    }
};

Application *CreateApplication(const ApplicationCommandLineArgs args)
{
    ApplicationCreateInfo createInfo;
    createInfo.cmdLineArgs = args;
    createInfo.width = 1280;
    createInfo.height = 720;
    return new EditorApp(createInfo);
}
