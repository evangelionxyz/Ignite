#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <ignite/physics/2d/physics_2d.hpp>
#include <nvrhi/nvrhi.h>
#include <unordered_map>
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"
#include "ignite/core/uuid.hpp"
#include "ignite/asset/asset.hpp"
#include "ignite/math/aabb.hpp"

namespace ignite
{
    class Camera;
    class Physics2D;
    class Entity;

    using EntityComponents = std::unordered_map<entt::entity, std::vector<IComponent *>>;
    using StringCounterMap = std::unordered_map<std::string, i32>;

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

        void OnRenderRuntimeSimulate(Camera *camera, nvrhi::ICommandList *commandList, nvrhi::IFramebuffer *framebuffer);
        void OnRenderRuntime(nvrhi::IFramebuffer *framebuffer);

        std::string name;
        entt::registry *registry = nullptr;

        std::unordered_map<UUID, entt::entity> entities; // uuid to entity
        std::unordered_map<std::string, UUID> nameToUUID;
        
        EntityComponents registeredComps;
        StringCounterMap entityNamesMapCounter;
        std::vector<std::string> entityNames;

        Scope<Physics2D> physics2D;

        bool IsPlaying() const { return m_Playing; }

        static Ref<Scene> Create(const std::string &name);

        static AssetType GetStaticType() { return AssetType::Scene; }
        virtual AssetType GetType() override { return GetStaticType(); }

        struct DebugBoundingBox
        {
            AABB aabb;
            glm::mat4 transform;
        };

        float timeInSeconds = 0.0f;
    
    private:
        bool m_Playing = false;
    };
}
