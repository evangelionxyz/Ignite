#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "ignite/physics/2d/physics_2d.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"
#include "ignite/core/uuid.hpp"
#include "ignite/asset/asset.hpp"
#include "ignite/math/aabb.hpp"

#include <nvrhi/nvrhi.h>
#include <unordered_map>

namespace ignite
{
    class Camera;
    class Physics2D;
    class JoltScene;
    class Entity;
    class Environment;
    class SceneRenderer;

    using EntityComponents = std::unordered_map<entt::entity, std::vector<IComponent *>>;

    class Scene : public Asset
    {
    public:
        Scene() = default;
        explicit Scene(const std::string &name);

        ~Scene();

        void OnStart();
        void OnStop();

        void UpdateTransforms(float deltaTime);
        void UpdateTransformRecursive(Entity entity, const glm::mat4 &parentWorldTransform);
        
        void OnUpdateRuntimeSimulate(f32 deltaTime);
        void OnUpdateEdit(f32 deltaTime);

        void OnResize(uint32_t width, uint32_t height);

        template<typename T>
        void OnComponentAdded(Entity entity, T &comp);

        Entity GetPrimaryCamera();

        SceneRenderer *sceneRenderer = nullptr;

        std::string name;
        entt::registry *registry = nullptr;

        std::unordered_map<UUID, entt::entity> entities; // uuid to entity
        
        EntityComponents registeredComps;

        Scope<Physics2D> physics2D;
        Scope<JoltScene> physics;

        bool IsPlaying() const { return m_Playing; }

        static Ref<Scene> Create(const std::string &name);

        static AssetType GetStaticType() { return AssetType::Scene; }
        virtual AssetType GetType() override { return GetStaticType(); }

        struct DebugBoundingBox
        {
            AABB aabb;
            glm::mat4 transform;
        };

        glm::vec3 physicsGravity{ 0.0f, -9.8f, 0.0f };
        float timeInSeconds = 0.0f;
        uint32_t viewportWidth = 1280, viewportHeight = 720;
    
    private:
        bool m_Playing = false;
    };
}
