#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <entt/entt.hpp>
#include <unordered_map>
#include "component.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"

#include <ignite/physics/2d/physics_2d.hpp>
#include <nvrhi/nvrhi.h>

namespace ignite
{
    class Camera;
    class Physics2D;

    using EntityMap = std::unordered_map<UUID, entt::entity>;

    class Scene
    {
    public:
        Scene() = default;
        Scene(const std::string &name);

        ~Scene();

        void OnStart();
        void OnStop();
        
        void OnUpdate(f32 deltaTime);
        void OnRender(Camera *camera, nvrhi::IFramebuffer *framebuffer);

        entt::entity CreateEntity(const std::string &name, UUID uuid = UUID());
        entt::entity GetEntity(UUID uuid);
        void DestroyEntity(UUID uuid);
        void DestroyEntity(entt::entity entity);

        template<typename T, typename... Args>
        T &AddComponent(const entt::entity entity, Args &&... args)
        {
            if (HasComponent<T>(entity))
                return GetComponent<T>(entity);

            T &comp = registry->emplace_or_replace<T>(entity, std::forward<Args>(args)...);
            return comp;
        }

        template<typename T, typename... Args>
        T &AddOrReplaceComponent(const entt::entity entity, Args &&... args)
        {
            T &comp = registry->emplace_or_replace<T>(entity, std::forward<Args>(args)...);
            return comp;
        }

        template<typename T>
        T &GetComponent(const entt::entity entity)
        {
            return registry->get<T>(entity);
        }

        template<typename T>
        bool HasComponent(const entt::entity entity) const
        {
            return registry->all_of<T>(entity);
        }

        std::string name;
        entt::registry *registry = nullptr;
        EntityMap entities;
        Scope<Physics2D> physics2D;
    };
}
