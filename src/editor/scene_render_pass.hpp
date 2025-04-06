#pragma once

#include "core/types.hpp"
#include "core/device_manager.hpp"
#include "core/irender_pass.hpp"

class ShaderFactory;

class SceneRenderPass : public IRenderPass
{
public:
    SceneRenderPass() = default;
    SceneRenderPass(DeviceManager *deviceManager);
    bool Init(Ref<ShaderFactory> shaderFactory);
    void Destroy() override;
    void SetLateWarpOptions() override { }
    void BackBufferResizing() override;
    void Animate(f32 elapsedTimeSeconds) override;
    void Render(nvrhi::IFramebuffer *framebuffer) override;
    void RenderScene(nvrhi::IFramebuffer *framebuffer);
    void RenderSplashScreen(nvrhi::IFramebuffer *framebuffer);

private:
    // Window inputs
    virtual bool MousePosUpdate(double xpos, double ypos) override;

private:
    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;
    nvrhi::GraphicsPipelineHandle m_Pipeline;
    nvrhi::CommandListHandle m_CommandList;
    
};
