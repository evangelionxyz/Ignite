 #include "scene.hpp"
#include <entt/entt.hpp>

#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/renderer.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/physics/2d/physics_2d.hpp"
#include "ignite/physics/jolt/jolt_physics.hpp"
#include "ignite/math/math.hpp"
#include "scene_manager.hpp"
#include "entity.hpp"
#include "ignite/core/application.hpp"
#include "ignite/animation/animation_system.hpp"

#include <ranges>

namespace ignite
{
    Scene::Scene(const std::string &_name)
        : name(_name)
    {
        registry = new entt::registry();

        physics2D = CreateScope<Physics2D>(this);
        physics = CreateScope<JoltScene>(this);
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
        physics->SimulationStart();
    }

    void Scene::OnStop()
    {
        timeInSeconds = 0.0f;

        m_Playing = false;
        
        physics2D->SimulationStop();
        physics->SimulationStop();
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
        physics->Simulate(deltaTime);
    }

    Ref<Scene> Scene::Create(const std::string &name)
    {
        return CreateRef<Scene>(name);
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

    template<>
    void Scene::OnComponentAdded<Rigibody>(Entity entity, Rigibody &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<BoxCollider>(Entity entity, BoxCollider &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<SphereCollider>(Entity entity, SphereCollider &comp)
    {
    }
}

