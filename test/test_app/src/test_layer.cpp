#include "test_layer.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/device/device_manager.hpp"

#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/scene/camera.hpp"

#include <nvrhi/utils.h>

namespace ignite
{
    int colorAttachmentIndex = 0;

    TestLayer::TestLayer(const std::string &name)
        : Layer(name)
    {
    }

    void TestLayer::OnAttach()
    {
        m_DeviceManager = Application::GetDeviceManager();
        m_Device = m_DeviceManager->GetDevice();

        m_CommandList = m_Device->createCommandList();
        Renderer2D::InitQuadData(m_DeviceManager->GetDevice(), m_CommandList);

        RenderTargetCreateInfo createInfo = {};
        createInfo.device = m_Device;
        createInfo.depthRead = true;
        createInfo.attachments = {
            //FramebufferAttachments{ nvrhi::Format::D32S8},
            FramebufferAttachments{ nvrhi::Format::RGBA8_UNORM },
        };

        m_RenderTarget = CreateRef<RenderTarget>(createInfo);

        m_ViewportCamera = CreateScope<Camera>("Editor Camera");
        //m_ViewportCamera->CreateOrthographic(Application::GetInstance()->GetCreateInfo().width, Application::GetInstance()->GetCreateInfo().height, 8.0f, 0.1f, 350.0f);
        m_ViewportCamera->CreatePerspective(45.0f, Application::GetInstance()->GetCreateInfo().width, Application::GetInstance()->GetCreateInfo().height, 0.1f, 350.0f);
        m_ViewportCamera->position = { 3.0f, 2.0f, 3.0f };
        m_ViewportCamera->yaw = -0.729f;
        m_ViewportCamera->pitch = 0.410f;

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

        uint32_t backBufferIndex = m_DeviceManager->GetCurrentBackBufferIndex();
        static uint32_t backBufferCount = m_DeviceManager->GetBackBufferCount();
        uint32_t width = (uint32_t)framebuffer->getFramebufferInfo().getViewport().width();
        uint32_t height = (uint32_t)framebuffer->getFramebufferInfo().getViewport().height();

        m_RenderTarget->CreateFramebuffers(backBufferCount, backBufferIndex, { width, height });

        m_CommandList->open();

        // clear main framebuffer
        nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

        // clear render target framebuffer
        m_RenderTarget->ClearColorAttachment(m_CommandList, colorAttachmentIndex, colorAttachmentIndex == 0 ? glm::vec3(0.5f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.5f, 0.0f));
        nvrhi::IFramebuffer *fb = m_RenderTarget->GetCurrentFramebuffer();

        Renderer2D::Begin(m_ViewportCamera.get(), fb);
        Renderer2D::DrawQuad(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)), glm::vec4(1.0f));
        Renderer2D::Flush();
        Renderer2D::End();

        m_CommandList->close();
        m_Device->executeCommandList(m_CommandList);
    }

    void TestLayer::OnGuiRender()
    {
        ImGui::Begin("Test Window");
        const ImTextureID imguiTex = reinterpret_cast<ImTextureID>(m_RenderTarget->GetColorAttachment(colorAttachmentIndex).Get());
        ImGui::Image(imguiTex, {480, 256 });

        ImGui::SliderInt("Color Attachment Index", &colorAttachmentIndex, 0, 1);

        ImGui::End();
    }
}