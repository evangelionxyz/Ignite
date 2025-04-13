#pragma once

#include "component_group.hpp"
#include "ignite/core/uuid.hpp"
#include "entity.hpp"

namespace ignite
{
    class EntityManager
    {
    public:
        static Entity CreateEntity(Scene *scene, const std::string &name, EntityType type, UUID uuid = UUID());
        static Entity CreateSprite(Scene *scene, const std::string &name, UUID uuid = UUID());
        static void RenameEntity(Scene *scene, Entity entity, const std::string &newName);
        static void DestroyEntity(Scene *scene, Entity entity);
        static void DestroyEntity(Scene *scene, UUID uuid);
        static Entity GetEntity(Scene *scene, UUID uuid);

        static void DestroyEntityWithCommand(Scene *scene, Entity entity);

        using EntityMap = std::unordered_map<UUID, entt::entity>;

        template<typename... Component>
        static void CopyComponent(entt::registry *destRegistry, entt::registry *srcRegistry, EntityMap entityMap)
        {
            ([&]()
                {
                    auto view = srcRegistry->view<Component>();
                    for (auto srcEntity : view)
                    {
                        for (auto e : entityMap)
                        {
                            // key (UUID)
                            if (e.first == srcRegistry->get<ID>(srcEntity).uuid)
                            {
                                entt::entity dstEntity = e.second;
                                destRegistry->emplace_or_replace<Component>(dstEntity, srcRegistry->get<Component>(srcEntity));
                            }
                        }
                    }
                }(), ...
            );
        }
    
        template<typename... Component>
        static void CopyComponent(ComponentGroup<Component...>, entt::registry *destRegistry, entt::registry *srcRegistry, EntityMap entityMap)
        {
            CopyComponent<Component...>(destRegistry, srcRegistry, entityMap);
        }
    
        template <typename... Component>
        static void CopyComponentIfExists(Entity dstEntity, Entity srcEntity)
        {
            ([&]()
            {
                if (srcEntity.HasComponent<Component>())
                {
                    dstEntity.AddOrReplaceComponent<Component>(srcEntity.GetComponent<Component>());
                }
            }(), ...);
        }
    
        template<typename... Component>
        static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dstEntity, Entity srcEntity)
        {
            CopyComponentIfExists<Component...>(dstEntity, srcEntity);
        }
    };
}

