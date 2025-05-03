#include "ignite/entry_point.hpp"
#include "ignite/core/application.hpp"
#include "test_layer.hpp"

class TestApp final : public ignite::Application
{
public:
    explicit TestApp(const ignite::ApplicationCreateInfo &createInfo)
        : Application(createInfo)
    {
        PushLayer(new ignite::TestLayer("Test Layer"));
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
        // default graphics api
        createInfo.graphicsApi = nvrhi::GraphicsAPI::VULKAN;
        return new TestApp(createInfo);
    }
}