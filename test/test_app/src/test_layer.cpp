#include "test_layer.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"

#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/scene/camera.hpp"

#include <nvrhi/utils.h>

namespace ignite
{
    TestLayer::TestLayer(const std::string &name)
        : Layer(name)
    {
    }

    void TestLayer::OnAttach()
    {
        m_DeviceManager = Application::GetDeviceManager();
        m_Device = m_DeviceManager->GetDevice();

        m_CommandList = m_Device->createCommandList();
    }

    void TestLayer::OnDetach()
    {
    }

    void TestLayer::OnUpdate(f32 deltaTime)
    {
    }

    void TestLayer::OnEvent(Event& e)
    {
    }

    void TestLayer::OnRender(nvrhi::IFramebuffer *framebuffer)
    {
        Layer::OnRender(framebuffer);
        m_CommandList->open();
        // clear main framebuffer
        nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));
        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void TestLayer::OnGuiRender()
    {
        ImGui::Begin("ImGui Window");
        ImGui::Text("FPS: %3.1f", ImGui::GetIO().Framerate);
        ImGui::End();
    }
}