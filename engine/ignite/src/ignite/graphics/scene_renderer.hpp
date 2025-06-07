#pragma once

#include "environment.hpp"
#include "graphics_pipeline.hpp"

#include "ignite/scene/entity.hpp"

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

namespace ignite
{
    class Scene;
    class Camera;

    class SceneRenderer
    {
    public:
        void Init();
        void CreatePipelines(nvrhi::IFramebuffer *framebuffer) const;
        void Render(Scene *scene, Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer);
        void RenderOutline(Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer, const std::vector<Entity>& selectedEntities);

        Ref<GraphicsPipeline> &GetBatchQuadPipeline() { return m_BatchQuadPipeline; }
        Ref<GraphicsPipeline> &GetBatchLinePipeline() { return m_BatchLinePipeline; }
        Ref<GraphicsPipeline> &GetEnvironmentPipeline() { return m_EnvironmentPipeline; }
        Ref<GraphicsPipeline> &GetMeshPipeline() { return m_MeshPipeline; }

        Ref<Environment> &GetEnvironment() { return m_Environment; }

    private:
        void CreateEnvironment();

        Ref<Environment> m_Environment;

        Ref<GraphicsPipeline> m_BatchQuadPipeline;
        Ref<GraphicsPipeline> m_BatchLinePipeline;
        Ref<GraphicsPipeline> m_EnvironmentPipeline;
        Ref<GraphicsPipeline> m_MeshPipeline;

        Ref<GraphicsPipeline> m_OutlineQuadPipeline;
    };
}
