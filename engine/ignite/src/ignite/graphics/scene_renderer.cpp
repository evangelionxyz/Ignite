#include "scene_renderer.hpp"

#include "mesh.hpp"
#include "renderer.hpp"
#include "renderer_2d.hpp"
#include "environment.hpp"

#include "ignite/scene/scene.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/scene/entity.hpp"
#include "ignite/scene/component.hpp"

#include <ranges>

namespace ignite
{
    void SceneRenderer::Render(Scene *scene, Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer)
    {
        // First pass:
        const auto &meshPipeline = Renderer::GetPipeline(GPipelines::DEFAULT_3D_MESH);
        scene->environment->Render(commandList, framebuffer, Renderer::GetPipeline(GPipelines::DEFAULT_3D_ENV), camera);

        // Second pass:
        Renderer2D::Begin(camera, commandList, framebuffer);

        for (entt::entity e : scene->entities | std::views::values)
        {
            Entity entity = { e, scene };
            auto &tr = entity.GetTransform();

            if (!tr.visible)
                continue;

            if (entity.HasComponent<MeshRenderer>())
            {
                MeshRenderer &meshRenderer = entity.GetComponent<MeshRenderer>();

                // write material constant buffer
                commandList->writeBuffer(meshRenderer.mesh->materialBufferHandle, &meshRenderer.material.data, sizeof(meshRenderer.material.data));
                commandList->writeBuffer(meshRenderer.mesh->objectBufferHandle, &meshRenderer.meshBuffer, sizeof(meshRenderer.meshBuffer));

                // render
                auto meshGraphicsState = nvrhi::GraphicsState();
                meshGraphicsState.pipeline = meshPipeline->GetHandle();
                meshGraphicsState.framebuffer = framebuffer;
                meshGraphicsState.viewport = nvrhi::ViewportState().addViewportAndScissorRect(framebuffer->getFramebufferInfo().getViewport());
                meshGraphicsState.addVertexBuffer({ meshRenderer.mesh->vertexBuffer, 0, 0 });
                meshGraphicsState.indexBuffer = { meshRenderer.mesh->indexBuffer, nvrhi::Format::R32_UINT };

                if (meshRenderer.mesh->bindingSet != nullptr)
                    meshGraphicsState.addBindingSet(meshRenderer.mesh->bindingSet);

                commandList->setGraphicsState(meshGraphicsState);

                nvrhi::DrawArguments args;
                args.setVertexCount(static_cast<uint32_t>(meshRenderer.mesh->indices.size()));
                args.instanceCount = 1;

                commandList->drawIndexed(args);
            }

            if (entity.HasComponent<Sprite2D>())
            {
                auto &sprite = entity.GetComponent<Sprite2D>();
                Renderer2D::DrawQuad(tr.GetWorldMatrix(), sprite.color, sprite.texture, sprite.tilingFactor, static_cast<u32>(e));
            }
        }

        Renderer2D::Flush();
        Renderer2D::End();
    }

}