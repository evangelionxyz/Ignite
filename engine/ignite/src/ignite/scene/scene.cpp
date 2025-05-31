 #include "scene.hpp"

#include <ranges>

#include <entt/entt.hpp>

#include "ignite/graphics/mesh.hpp"
#include "camera.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/physics/2d/physics_2d.hpp"
#include "ignite/math/math.hpp"
#include "entity.hpp"
#include "scene_manager.hpp"

#include "ignite/animation/animation_system.hpp"

namespace ignite
{
    Scene::Scene(const std::string &_name)
        : name(_name)
    {
        registry = new entt::registry();
        physics2D = CreateScope<Physics2D>(this);
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
                    skinnedMesh.boneTransforms = AnimationSystem::GetFinalJointTransforms(skinnedMesh.skeleton);
                }
            }
        }

        auto view = registry->view<ID, Transform>();
        for (auto ent : view)
        {
            const auto &[id, transform] = view.get<ID, Transform>(ent);
            if (id.parent == 0) // root node
            {
                UpdateTransformRecursive(Entity { ent, this }, glm::mat4(1.0f));
            }
        }

#if 0
        auto view = registry->view<ID, Transform>();
        for (auto eHandle : view)
        {
            Entity entity { eHandle, this };
            Transform &transform = entity.GetComponent<Transform>();
            ID &id = entity.GetComponent<ID>();

            glm::vec3 skew;
            glm::vec4 perspective;
\
            if (id.parent != 0)
            {
                if (entity.HasComponent<MeshRenderer>())
                {
                    MeshRenderer &meshRenderer = entity.GetComponent<MeshRenderer>();

                    Entity rootNodeEntity = SceneManager::GetEntity(this, meshRenderer.root);
                    SkinnedMesh &skinnedMesh = rootNodeEntity.GetComponent<SkinnedMesh>();

                    Entity parentNodeEntity = SceneManager::GetEntity(this, meshRenderer.parentNode);
                    Transform &parentNodeTr = parentNodeEntity.GetComponent<Transform>();
                    const std::string &name = parentNodeEntity.GetName();

                    glm::mat4 transformMatrix = parentNodeTr.GetWorldMatrix();

                    if (skinnedMesh.activeAnimIndex >= 0 && skinnedMesh.animations[skinnedMesh.activeAnimIndex]->isPlaying && id.parent != UUID(0))
                    {
                        auto it = skinnedMesh.skeleton.nameToJointMap.find(name);
                        if (it != skinnedMesh.skeleton.nameToJointMap.end())
                        {
                            Joint &joint = skinnedMesh.skeleton.joints[it->second];
                            transformMatrix = joint.globalTransform * joint.inverseBindPose * parentNodeTr.GetWorldMatrix();
                        }
                    }

                    meshRenderer.meshBuffer.transformation = transformMatrix;
                    
                    glm::decompose(meshRenderer.meshBuffer.transformation * transform.GetLocalMatrix(), transform.scale, transform.rotation, transform.translation, skew, perspective);

                    const size_t numBones = std::min(skinnedMesh.boneTransforms.size(), static_cast<size_t>(MAX_BONES));
                    for (size_t i = 0; i < numBones; ++i)
                    {
                        meshRenderer.meshBuffer.boneTransforms[i] = skinnedMesh.boneTransforms[i];
                    }

                    // Set remaining transforms to identity
                    for (size_t i = numBones; i < MAX_BONES; ++i)
                    {
                        meshRenderer.meshBuffer.boneTransforms[i] = glm::mat4(1.0f);
                    }

                    glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(meshRenderer.meshBuffer.transformation)));
                    meshRenderer.meshBuffer.normal = glm::mat4(normalMat3);
                }
                else
                {
                    Entity parent = SceneManager::GetEntity(this, id.parent);
                    Transform &parentTransform = parent.GetComponent<Transform>();

                    glm::mat4 transformMatrix = parentTransform.GetWorldMatrix() * transform.GetLocalMatrix();

                    glm::decompose(transformMatrix, transform.scale, transform.rotation, transform.translation, skew, perspective);
                }
            }
            else
            {
                glm::decompose(transform.GetLocalMatrix(), transform.scale, transform.rotation, transform.translation, skew, perspective);
            }
        }
#endif
    }

    void Scene::UpdateTransformRecursive(Entity parentEntity, const glm::mat4 &parentWorldTransform)
    {
        Transform &transform = parentEntity.GetComponent<Transform>();
        ID &id = parentEntity.GetComponent<ID>();

        glm::vec3 skew;
        glm::vec4 perspective;

        glm::mat4 worldMatrix = parentWorldTransform * transform.GetLocalMatrix();


        // Special logic for MeshRenderer (e.g., skeletal animation)
        if (parentEntity.HasComponent<MeshRenderer>())
        {
            MeshRenderer &meshRenderer = parentEntity.GetComponent<MeshRenderer>();
            Entity rootNodeEntity = SceneManager::GetEntity(this, meshRenderer.root);
            SkinnedMesh &skinnedMesh = rootNodeEntity.GetComponent<SkinnedMesh>();

            Entity parentNodeEntity = SceneManager::GetEntity(this, meshRenderer.parentNode);
            Transform &parentNodeTr = parentNodeEntity.GetComponent<Transform>();
            const std::string &name = parentNodeEntity.GetName();

            glm::mat4 transformMatrix = worldMatrix;

            if (skinnedMesh.activeAnimIndex >= 0 && skinnedMesh.animations[skinnedMesh.activeAnimIndex]->isPlaying && id.parent != UUID(0))
            {
                auto it = skinnedMesh.skeleton.nameToJointMap.find(name);
                if (it != skinnedMesh.skeleton.nameToJointMap.end())
                {
                    Joint &joint = skinnedMesh.skeleton.joints[it->second];
                    transformMatrix = joint.globalTransform * joint.inverseBindPose * worldMatrix;
                }
            }

            meshRenderer.meshBuffer.transformation = transformMatrix;

            glm::decompose(meshRenderer.meshBuffer.transformation, transform.scale, transform.rotation, transform.translation, skew, perspective);

            const size_t numBones = std::min(skinnedMesh.boneTransforms.size(), static_cast<size_t>(MAX_BONES));
            for (size_t i = 0; i < numBones; ++i)
                meshRenderer.meshBuffer.boneTransforms[i] = skinnedMesh.boneTransforms[i];
            for (size_t i = numBones; i < MAX_BONES; ++i)
                meshRenderer.meshBuffer.boneTransforms[i] = glm::mat4(1.0f);

            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(meshRenderer.meshBuffer.transformation)));
            meshRenderer.meshBuffer.normal = glm::mat4(normalMat3);
        }
        else
        {
            // Standard transform decompose
            glm::decompose(worldMatrix, transform.scale, transform.rotation, transform.translation, skew, perspective);
        }

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

    void Scene::OnRenderRuntimeSimulate(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        Renderer2D::Begin(camera, framebuffer);

        for (entt::entity e: entities | std::views::values)
        {
            Entity entity = { e, this };
            auto &tr = entity.GetComponent<Transform>();
            
            if (!tr.visible)
                continue;

            if (entity.HasComponent<Sprite2D>())
            {
                auto &sprite = entity.GetComponent<Sprite2D>();
                Renderer2D::DrawQuad(tr.GetWorldMatrix(), sprite.color, sprite.texture);
            }
        }

        Renderer2D::Flush();
        Renderer2D::End();
    }
}

