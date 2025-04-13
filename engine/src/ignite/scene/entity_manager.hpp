#pragma once

#include "component.hpp"
#include "ignite/core/uuid.hpp"

namespace ignite
{
    class Entity;
    class Scene;

    class EntityManager
    {
    public:
        static Entity CreateEntity(Scene *scene, const std::string &name, EntityType type, UUID uuid = UUID());
        static Entity CreateSprite(Scene *scene, const std::string &name, UUID uuid = UUID());
        static void RenameEntity(Scene *scene, Entity entity, const std::string &newName);
        static void DestroyEntity(Scene *scene, Entity entity);
        static void DestroyEntity(Scene *scene, UUID uuid);
        static Entity GetEntity(Scene *scene, UUID uuid);
    };
}

