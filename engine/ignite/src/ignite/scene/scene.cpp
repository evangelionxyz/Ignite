 #include "scene.hpp"

#include <ranges>

#include <entt/entt.hpp>

#include "ignite/graphics/mesh.hpp"
#include "camera.hpp"
#include "ignite/graphics/renderer.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/physics/2d/physics_2d.hpp"
#include "ignite/math/math.hpp"
#include "scene_manager.hpp"
#include "entity.hpp"

#include "ignite/core/application.hpp"

#include "ignite/animation/animation_system.hpp"

namespace ignite
{
    Scene::Scene(const std::string &_name)
        : name(_name)
    {
        registry = new entt::registry();
        physics2D = CreateScope<Physics2D>(this);

        CreateEnvironment();
    }

    void Scene::CreateEnvironment()
    {
        // create env
        auto pipeline = Renderer::GetPipeline(GPipelines::DEFAULT_3D_ENV);
        nvrhi::IDevice *device = Application::GetRenderDevice();
        
        environment = Environment::Create(device);
        environment->LoadTexture(device, "resources/hdr/klippad_sunrise_2_2k.hdr", pipeline->GetBindingLayout());

        nvrhi::CommandListHandle commandList = device->createCommandList();
        commandList->open();
        environment->WriteBuffer(commandList);
        commandList->close();
        device->executeCommandList(commandList);

        environment->SetSunDirection(50.0f, -27.0f);
    }

    Scene::~Scene()
    {
        if (registry)
            delete registry;
    }

    void Scene::OnStart()
    {
        // reset time
        timeInSeconds = 0.0f;

        m_Playing = true;
        physics2D->SimulationStart();
    }

    void Scene::OnStop()
    {
        timeInSeconds = 0.0f;

        m_Playing = false;
        
        physics2D->SimulationStop();
    }

    void Scene::UpdateTransforms(float deltaTime)
    {
        auto skinnedMeshView = registry->view<SkinnedMesh>();
        for (auto entity : skinnedMeshView)
        {
            SkinnedMesh &skinnedMesh = skinnedMeshView.get<SkinnedMesh>(entity);
            if (!skinnedMesh.animations.empty() && skinnedMesh.animations[skinnedMesh.activeAnimIndex]->isPlaying)
            {
                if (AnimationSystem::UpdateSkeleton(skinnedMesh.skeleton, skinnedMesh.animations[skinnedMesh.activeAnimIndex], timeInSeconds))
                {
                    AnimationSystem::ApplySkeletonToEntities(this, skinnedMesh.skeleton);
                    skinnedMesh.boneTransforms = AnimationSystem::GetFinalJointTransforms(skinnedMesh.skeleton);
                }
            }
        }

        auto view = registry->view<ID, Transform>();
        for (auto ent : view)
        {
            const auto &[id, transform] = view.get<ID, Transform>(ent);
            if (id.parent == 0)
            {
                UpdateTransformRecursive(Entity { ent, this }, glm::mat4(1.0f));
            }
        }
    }

    void Scene::UpdateTransformRecursive(Entity entity, const glm::mat4 &parentWorldTransform)
    {
        Transform &transform = entity.GetTransform();
        ID &id = entity.GetComponent<ID>();

        glm::vec3 skew;
        glm::vec4 perspective;
        
        glm::mat4 worldMatrix = parentWorldTransform * transform.GetLocalMatrix();
        
        glm::decompose(worldMatrix,
            transform.scale,
            transform.rotation,
            transform.translation,
            skew,
            perspective);
        
        // Special logic for MeshRenderer (e.g., skeletal animation)
        if (entity.HasComponent<MeshRenderer>())
        {
            MeshRenderer &meshRenderer = entity.GetComponent<MeshRenderer>();

            meshRenderer.meshBuffer.transformation = worldMatrix;
            glm::decompose(meshRenderer.meshBuffer.transformation, transform.scale, transform.rotation, transform.translation, skew, perspective);

            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(meshRenderer.meshBuffer.transformation)));
            meshRenderer.meshBuffer.normal = glm::mat4(normalMat3);
            if (meshRenderer.root != UUID(0))
            {
                Entity rootNodeEntity = SceneManager::GetEntity(this, meshRenderer.root);
                SkinnedMesh &skinnedMesh = rootNodeEntity.GetComponent<SkinnedMesh>();
                
                const size_t numBones = std::min(skinnedMesh.boneTransforms.size(), static_cast<size_t>(MAX_BONES));
                for (size_t i = 0; i < numBones; ++i)
                {
                    meshRenderer.meshBuffer.boneTransforms[i] = skinnedMesh.boneTransforms[i];
                }
                
                for (size_t i = numBones; i < MAX_BONES; ++i)
                {
                    meshRenderer.meshBuffer.boneTransforms[i] = glm::mat4(1.0f);
                }
            }
            else
            {
                for (size_t i = 0; i < MAX_BONES; ++i)
                {
                    meshRenderer.meshBuffer.boneTransforms[i] = glm::mat4(1.0f);
                }
            }
        }

        transform.dirty = false;

        // Recurse for children
        for (const UUID &childUUID : id.children)
        {
            Entity child = SceneManager::GetEntity(this, childUUID);
            UpdateTransformRecursive(child, worldMatrix);
        }
    }

    void Scene::OnUpdateEdit(f32 deltaTime)
    {
        timeInSeconds += deltaTime;

        UpdateTransforms(deltaTime);
    }

    void Scene::OnUpdateRuntimeSimulate(f32 deltaTime)
    {
        timeInSeconds += deltaTime;

        UpdateTransforms(deltaTime);

        physics2D->Simulate(deltaTime);
    }

    void Scene::OnRenderRuntime(nvrhi::IFramebuffer *framebuffer)
    {
        // TODO: render with camera component
    }

    Ref<Scene> Scene::Create(const std::string &name)
    {
        return CreateRef<Scene>(name);
    }

    void Scene::OnRenderRuntimeSimulate(Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer)
    {
        // First pass:
        const auto &meshPipeline = Renderer::GetPipeline(GPipelines::DEFAULT_3D_MESH);
        environment->Render(commandList, framebuffer, Renderer::GetPipeline(GPipelines::DEFAULT_3D_ENV), camera);

        // Second pass:
        Renderer2D::Begin(camera, commandList, framebuffer);

        for (entt::entity e: entities | std::views::values)
        {
            Entity entity = { e, this };
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

    template<typename T>
    void Scene::OnComponentAdded(Entity entity, T &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<ID>(Entity entity, ID &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<Transform>(Entity entity, Transform &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<Sprite2D>(Entity entity, Sprite2D &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<SkinnedMesh>(Entity entity, SkinnedMesh &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<MeshRenderer>(Entity entity, MeshRenderer &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<Rigidbody2D>(Entity entity, Rigidbody2D &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<BoxCollider2D>(Entity entity, BoxCollider2D &comp)
    {
    }
}

