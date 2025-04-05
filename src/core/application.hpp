#pragma once

#include "device_manager.hpp"
#include "types.hpp"

#include <filesystem>

class Application : public IRenderPass
{
public:
    Application(DeviceManager *deviceManager);
    bool Init();
    void Run();
    void Shutdown();
    
    virtual void SetLateWarpOptions() override { }
    virtual void BackBufferResizing() override;
    virtual void Animate(f32 elapsedTimeSeconds) override;
    virtual void Render(nvrhi::IFramebuffer *framebuffer) override;

    void RenderScene(nvrhi::IFramebuffer *framebuffer);
    void RenderSplashScreen(nvrhi::IFramebuffer *framebuffer);

private:
    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;
    nvrhi::GraphicsPipelineHandle m_Pipeline;
    nvrhi::CommandListHandle m_CommandList;
    
};
