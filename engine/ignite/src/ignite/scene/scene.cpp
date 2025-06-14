 #include "scene.hpp"
#include <entt/entt.hpp>

#include "ignite/audio/fmod_sound.hpp"
#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/renderer.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/physics/2d/physics_2d.hpp"
#include "ignite/physics/jolt/jolt_physics.hpp"
#include "ignite/math/math.hpp"
#include "scene_manager.hpp"
#include "ignite/scripting/script_engine.hpp"
#include "entity.hpp"
#include "ignite/core/application.hpp"
#include "ignite/animation/animation_system.hpp"

#include "ignite/project/project.hpp"

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
        m_Playing = true;

        ScriptEngine::SetSceneContext(this);

        // reset time
        timeInSeconds = 0.0f;

        // resize
        auto camView = registry->view<Camera>();
        for (entt::entity entity : camView)
        {
            Camera &cam = camView.get<Camera>(entity);
            cam.camera.SetSize(static_cast<float>(viewportWidth), static_cast<float>(viewportHeight));
            cam.camera.UpdateProjectionMatrix();
        }

        // play on start audio
        auto audioView = registry->view<AudioSource>();
        for (entt::entity e : audioView)
        {
            AudioSource &as = audioView.get<AudioSource>(e);
            if (as.playOnStart)
            {
                Ref<FmodSound> sound = Project::GetAsset<FmodSound>(as.handle);
                if (sound)
                {
                    sound->Play();
                    sound->SetVolume(as.volume);
                    sound->SetPitch(as.pitch);
                    sound->SetPan(as.pan);
                }
            }
        }

        registry->view<Script>().each([this](entt::entity e, Script &script)
        {
            Entity entity{ e, this };
            ScriptEngine::OnCreateEntity(entity);
        });

        physics2D->SimulationStart();
        physics->SimulationStart();
    }

    void Scene::OnStop()
    {
        m_Playing = false;

        timeInSeconds = 0.0f;

        // play on start audio
        auto audioView = registry->view<AudioSource>();
        for (entt::entity e : audioView)
        {
            AudioSource &as = audioView.get<AudioSource>(e);
            Ref<FmodSound> sound = Project::GetAsset<FmodSound>(as.handle);
            if (sound)
            {
                sound->Stop();
            }
        }

        ScriptEngine::ClearSceneContext();
        
        physics2D->SimulationStop();
        physics->SimulationStop();
    }

    void Scene::UpdateTransforms(float deltaTime)
    {
        auto skinnedMeshView = registry->view<SkinnedMesh>();
        for (auto entity : skinnedMeshView)
        {
            SkinnedMesh &skinnedMesh = skinnedMeshView.get<SkinnedMesh>(entity);
            if (!skinnedMesh.animations.empty() && skinnedMesh.animations[skinnedMesh.activeAnimIndex].isPlaying)
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

        if (entity.HasComponent<Camera>())
        {
            Camera &cam = entity.GetComponent<Camera>();
            if (cam.primary)
            {
                cam.camera.viewMatrix = glm::translate(glm::mat4(1.0f), transform.translation) * glm::toMat4(transform.rotation);
                cam.camera.viewMatrix = glm::inverse(cam.camera.viewMatrix);
                cam.camera.position = transform.translation;
            }
        }
        
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
                
                if (!rootNodeEntity.IsValid())
                {
                    meshRenderer.root = UUID(0);
                }

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

    void Scene::OnResize(uint32_t width, uint32_t height)
    {
        this->viewportWidth = width; this->viewportHeight = height;
        
        auto camView = registry->view<Camera>();
        for (entt::entity entity : camView)
        {
            Camera &cam = camView.get<Camera>(entity);
            cam.camera.SetSize(static_cast<float>(width), static_cast<float>(height));
            cam.camera.UpdateProjectionMatrix();
        }
    }

    Entity Scene::GetPrimaryCamera()
    {
        auto camView = registry->view<Camera>();
        for (entt::entity entity : camView)
        {
            Camera &cam = camView.get<Camera>(entity);
            if (cam.primary)
                return Entity { entity, this };
        }
        
        return Entity{};
    }

    void Scene::OnUpdateRuntimeSimulate(f32 deltaTime)
    {
        timeInSeconds += deltaTime;

        registry->view<Script>().each([this, deltaTime](entt::entity e, Script &sc)
        {
            Entity entity{ e, this };
            ScriptEngine::OnUpdateEntity(entity, deltaTime);
        });

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

    template<>
    void Scene::OnComponentAdded<AudioSource>(Entity entity, AudioSource &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<Script>(Entity entity, Script &comp)
    {
    }

    template<>
    void Scene::OnComponentAdded<Camera>(Entity entity, Camera &comp)
    {
        comp.camera.projectionType = comp.projectionType;
        switch (comp.projectionType)
        {
            case ICamera::Type::Perspective:
            {
                comp.camera.CreatePerspective(comp.fov, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), comp.nearClip, comp.farClip);
                break;
            }
            case ICamera::Type::Orthographic:
            {
                comp.camera.CreateOrthographic(static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), comp.zoom, comp.nearClip, comp.farClip);
                break;
            }
        }
    }
}
