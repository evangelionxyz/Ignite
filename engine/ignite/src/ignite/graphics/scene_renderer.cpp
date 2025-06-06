#include "scene_renderer.hpp"

#include "mesh.hpp"
#include "renderer.hpp"
#include "renderer_2d.hpp"
#include "environment.hpp"

#include "ignite/scene/scene.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/scene/entity.hpp"
#include "ignite/scene/component.hpp"

#include "ignite/core/application.hpp"

#include <ranges>

namespace ignite
{
    void SceneRenderer::Init()
    {
        GraphicsPipelineParams params;
        params.enableBlend = true;
        params.depthWrite = true;
        params.depthTest = true;
        params.fillMode = nvrhi::RasterFillMode::Solid;
        params.cullMode = nvrhi::RasterCullMode::Front;

        // Mesh pipeline
        {
            auto attributes = VertexMesh::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());

            m_MeshPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GBindingLayout::MESH));
            m_MeshPipeline->AddShader("default_mesh.vertex.hlsl", nvrhi::ShaderType::Vertex)
                .AddShader("default_mesh.pixel.hlsl", nvrhi::ShaderType::Pixel)
                .Build();
        }

        // Environment Pipeline
        {
            params.cullMode = nvrhi::RasterCullMode::Front;
            params.comparison = nvrhi::ComparisonFunc::Always;

            auto attribute = Environment::GetAttribute();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = &attribute;
            pci.attributeCount = 1;

            m_EnvironmentPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GBindingLayout::ENVIRONMENT));
            m_EnvironmentPipeline->AddShader("skybox.vertex.hlsl", nvrhi::ShaderType::Vertex)
                .AddShader("skybox.pixel.hlsl", nvrhi::ShaderType::Pixel)
                .Build();
        }

        // Batch Quad 2D
        {
            params.fillMode = nvrhi::RasterFillMode::Solid;
            params.cullMode = nvrhi::RasterCullMode::Front;

            auto attributes = Vertex2DQuad::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());

            auto shaderContext = Renderer::GetShaderLibrary().Get("batch_2d_quad");

            m_BatchQuadPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GBindingLayout::QUAD2D));
            m_BatchQuadPipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
                .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
                .Build();
        }

        // Batch Line 2D
        {
            params.enableBlend = false;
            params.depthWrite = true;
            params.depthTest = true;
            params.fillMode = nvrhi::RasterFillMode::Wireframe;
            params.cullMode = nvrhi::RasterCullMode::None;
            params.primitiveType = nvrhi::PrimitiveType::LineList;

            auto attributes = Vertex2DLine::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());

            m_BatchLinePipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GBindingLayout::LINE));

            auto shaderContext = Renderer::GetShaderLibrary().Get("batch_2d_line");

            m_BatchLinePipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
                .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
                .Build();
        }
        
        // Create Environment
        CreateEnvironment();
    }

    void SceneRenderer::CreatePipelines(nvrhi::IFramebuffer *framebuffer)
    {
        m_BatchQuadPipeline->CreatePipeline(framebuffer);
        m_BatchLinePipeline->CreatePipeline(framebuffer);
        m_EnvironmentPipeline->CreatePipeline(framebuffer);
        m_MeshPipeline->CreatePipeline(framebuffer);
    }

    void SceneRenderer::Render(Scene *scene, Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer)
    {
        if (scene->sceneRenderer == nullptr)
            scene->sceneRenderer = this;

        {
            // First pass: Main Scene
            m_Environment->Render(commandList, framebuffer, m_EnvironmentPipeline, camera);

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
                    meshGraphicsState.pipeline = m_MeshPipeline->GetHandle();
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

            Renderer2D::Flush(m_BatchQuadPipeline->GetHandle(), m_BatchLinePipeline->GetHandle());
            Renderer2D::End();
        }
    }

    void SceneRenderer::CreateEnvironment()
    {
        // create environment
        nvrhi::IDevice *device = Application::GetRenderDevice();

        m_Environment = Environment::Create();
        m_Environment->LoadTexture("resources/hdr/klippad_sunrise_2_2k.hdr");

        nvrhi::CommandListHandle commandList = device->createCommandList();
        commandList->open();
        m_Environment->WriteBuffer(commandList);
        commandList->close();
        device->executeCommandList(commandList);

        m_Environment->SetSunDirection(50.0f, -27.0f);
    }

}