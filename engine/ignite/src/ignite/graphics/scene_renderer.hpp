#pragma once

#include "environment.hpp"
#include "graphics_pipeline.hpp"

#include "ignite/scene/entity.hpp"

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

namespace ignite
{
    class Scene;
    class ICamera;

    class SceneRenderer
    {
    public:
        void Init();

        void CreateRenderTarget();
        void ResizeRenderTarget(uint32_t width, uint32_t height);

        void CreatePipelines(nvrhi::IFramebuffer *framebuffer) const;
        void Render(Scene *scene, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer, bool renderEnvironment = true);

        void SetFillMode(nvrhi::RasterFillMode mode) const;

        Ref<GraphicsPipeline> &GetBatchQuadPipeline() { return m_BatchQuadPipeline; }
        Ref<GraphicsPipeline> &GetBatchLinePipeline() { return m_BatchLinePipeline; }
        Ref<GraphicsPipeline> &GetEnvironmentPipeline() { return m_EnvironmentPipeline; }
        Ref<GraphicsPipeline> &GetGeometryPipeline() { return m_GeometryPipeline; }

        Ref<Environment> &GetEnvironment() { return m_Environment; }

    private:
        void CreateEnvironment();

        Ref<Environment> m_Environment;

        Ref<GraphicsPipeline> m_BatchQuadPipeline;
        Ref<GraphicsPipeline> m_BatchLinePipeline;
        Ref<GraphicsPipeline> m_EnvironmentPipeline;

        Ref<GraphicsPipeline> m_GeometryPipeline;
    };
}
