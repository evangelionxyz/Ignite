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
    createInfo.CommandLineArgs = args;
    return new EditorApp(createInfo);
}
