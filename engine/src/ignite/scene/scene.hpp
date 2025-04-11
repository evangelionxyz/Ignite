#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <entt/entt.hpp>
#include <unordered_map>
#include "component.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"

#include <nvrhi/nvrhi.h>

namespace ignite
{
    class Camera;
    using EntityMap = std::unordered_map<ignite::UUID, entt::entity>;

    class Scene
    {
    public:
        Scene() = default;
        Scene(const std::string &name);

        ~Scene();

        void OnUpdate(f32 deltaTime);
        void OnRender(Camera *camera, nvrhi::IFramebuffer *framebuffer);

        entt::entity CreateEntity(const std::string &name, UUID uuid = UUID());
        entt::entity GetEntity(UUID uuid);
        void DestroyEntity(UUID uuid);
        void DestroyEntity(entt::entity entity);

        template<typename T, typename... Args>
        T &EntityAddComponent(const entt::entity entity, Args &&... args)
        {
            if (EntityHasComponent<T>(entity))
                return EntityGetComponent<T>(entity);

            T &comp = registry->emplace_or_replace<T>(entity, std::forward<Args>(args)...);
            return comp;
        }

        template<typename T, typename... Args>
        T &EntityAddOrReplaceComponent(const entt::entity entity, Args &&... args)
        {
            T &comp = registry->emplace_or_replace<T>(entity, std::forward<Args>(args)...);
            return comp;
        }

        template<typename T>
        T &EntityGetComponent(const entt::entity entity)
        {
            return registry->get<T>(entity);
        }

        template<typename T>
        bool EntityHasComponent(const entt::entity entity) const
        {
            return registry->all_of<T>(entity);
        }

        std::string name;
        entt::registry *registry = nullptr;
        EntityMap entities;
    };
}
