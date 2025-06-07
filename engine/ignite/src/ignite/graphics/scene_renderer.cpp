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

        params.enableDepthStencil = false;

        // Fail = Keep, Pass = Replace
        params.frontFaceStencilDesc.failOp = nvrhi::StencilOp::Keep;      // Stencil test fails
        params.frontFaceStencilDesc.depthFailOp = nvrhi::StencilOp::Keep; // Depth test fails
        params.frontFaceStencilDesc.passOp = nvrhi::StencilOp::Replace;   // Both tests pass - WRite to stencil
        params.frontFaceStencilDesc.stencilFunc = nvrhi::ComparisonFunc::Always; // Always pass stencil test

        params.stencilWriteMask = 0xFF;
        params.stencilReadMask = 0x00;
        params.stencilRefValue = 1; // Write value 1 where object is rendered

        // Mesh pipeline
        {
            auto attributes = VertexMesh::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());

            m_MeshPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GPipeline::MESH));
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

            m_EnvironmentPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GPipeline::ENVIRONMENT));
            m_EnvironmentPipeline->AddShader("skybox.vertex.hlsl", nvrhi::ShaderType::Vertex)
                .AddShader("skybox.pixel.hlsl", nvrhi::ShaderType::Pixel)
                .Build();
        }

        // Batch Quad 2D
        {
            params.fillMode = nvrhi::RasterFillMode::Solid;
            params.cullMode = nvrhi::RasterCullMode::Front;

            params.comparison = nvrhi::ComparisonFunc::LessOrEqual;

            auto attributes = Vertex2DQuad::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());

            auto shaderContext = Renderer::GetShaderLibrary().Get("batch_2d_quad");

            m_BatchQuadPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GPipeline::QUAD2D));
            m_BatchQuadPipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
                .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
                .Build();
        }

        // Batch Line 2D Pipeline
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

            m_BatchLinePipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GPipeline::LINE));

            auto shaderContext = Renderer::GetShaderLibrary().Get("batch_2d_line");

            m_BatchLinePipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
                .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
                .Build();
        }

        // Outline Quad Pipeline (second pass) - only renders where stencil != 1
        {
            params.enableBlend = true;
            params.depthWrite = false; // Don't overwrite depth from the first pass
            params.depthTest = true; // Use existing depth buffer
            params.primitiveType = nvrhi::PrimitiveType::TriangleList;
            params.fillMode = nvrhi::RasterFillMode::Solid;
            params.cullMode = nvrhi::RasterCullMode::None;
            params.comparison = nvrhi::ComparisonFunc::LessOrEqual;

            params.enableDepthStencil = true;

            params.frontFaceStencilDesc.failOp = nvrhi::StencilOp::Keep;
            params.frontFaceStencilDesc.depthFailOp = nvrhi::StencilOp::Keep;
            params.frontFaceStencilDesc.passOp = nvrhi::StencilOp::Replace;       // Don't modify stencil
            params.frontFaceStencilDesc.stencilFunc = nvrhi::ComparisonFunc::Less; // Only render where stencil != 1

            // params.backFaceStencilDesc.failOp = nvrhi::StencilOp::Keep;
            // params.backFaceStencilDesc.depthFailOp = nvrhi::StencilOp::Keep;
            // params.backFaceStencilDesc.passOp = nvrhi::StencilOp::Replace;       // Don't modify stencil
            // params.backFaceStencilDesc.stencilFunc = nvrhi::ComparisonFunc::Less; // Only render where stencil != 1

            params.stencilWriteMask = 0x00; // Don't write to stencil
            params.stencilReadMask = 0xFF; // READ from stencil
            params.stencilRefValue = 1; // Compare against value 1

            auto attributes = Vertex2DQuad::GetAttributes();
            GraphicsPiplineCreateInfo pci;
            pci.attributes = attributes.data();
            pci.attributeCount = static_cast<uint32_t>(attributes.size());

            auto shaderContext = Renderer::GetShaderLibrary().Get("outline");

            m_OutlineQuadPipeline = GraphicsPipeline::Create(params, &pci, Renderer::GetBindingLayout(GPipeline::OUTLINE));
            m_OutlineQuadPipeline->AddShader(shaderContext[nvrhi::ShaderType::Vertex].handle, nvrhi::ShaderType::Vertex)
                .AddShader(shaderContext[nvrhi::ShaderType::Pixel].handle, nvrhi::ShaderType::Pixel)
                .Build();
        }
        
        // Create Environment
        CreateEnvironment();
    }

    void SceneRenderer::CreatePipelines(nvrhi::IFramebuffer *framebuffer) const
    {
        m_BatchQuadPipeline->CreatePipeline(framebuffer);
        m_BatchLinePipeline->CreatePipeline(framebuffer);
        m_EnvironmentPipeline->CreatePipeline(framebuffer);
        m_MeshPipeline->CreatePipeline(framebuffer);

        m_OutlineQuadPipeline->CreatePipeline(framebuffer);
    }

    void SceneRenderer::Render(Scene *scene, Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer)
    {
        if (scene->sceneRenderer == nullptr)
            scene->sceneRenderer = this;

        CameraBuffer cameraBuffer = { camera->GetViewProjectionMatrix(), glm::vec4(camera->position, 1.0f) };
        commandList->writeBuffer(Renderer::GetCameraBufferHandle(), &cameraBuffer, sizeof(cameraBuffer));
        
        {
            // First pass: Main Scene
            m_Environment->Render(commandList, framebuffer, m_EnvironmentPipeline);

            Renderer2D::Begin(commandList, framebuffer);

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
                    {
                        meshGraphicsState.addBindingSet(meshRenderer.mesh->bindingSet);
                    }

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

            Renderer2D::Flush(m_BatchQuadPipeline, m_BatchLinePipeline);
            Renderer2D::End();
        }

        
    }

    void SceneRenderer::RenderOutline(Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer, const std::vector<Entity>& selectedEntities)
    {
        {
            // Second pass: Outlining
            Renderer2D::Begin(commandList, framebuffer);

            for (Entity entity : selectedEntities)
            {
                auto &tr = entity.GetTransform();

                if (!tr.visible)
                    continue;

                float baseThickness = 0.025f;
                float distance = glm::distance(camera->position, tr.translation);
                float thicknessFactor = glm::clamp(baseThickness * distance, 0.025f, 1.8f);

                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), tr.scale + thicknessFactor);
                glm::vec3 outlineOffset = -camera->GetForwardDirection() * 0.001f;
                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), tr.translation + outlineOffset);
                
                if (entity.HasComponent<Sprite2D>())
                {
                    auto &sprite = entity.GetComponent<Sprite2D>();
                    glm::mat4 transform = translationMatrix * glm::mat4(tr.rotation) * scaleMatrix;
                    Renderer2D::DrawQuad(transform, sprite.color, sprite.texture, sprite.tilingFactor, entity);
                }
            }

            Renderer2D::Flush(m_OutlineQuadPipeline, m_BatchLinePipeline, true);
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