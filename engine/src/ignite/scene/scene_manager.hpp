#pragma once

#include "component_group.hpp"
#include "ignite/core/uuid.hpp"
#include "entity.hpp"

namespace ignite
{
    class SceneManager
    {
    public:
        static Entity CreateEntity(Scene *scene, const std::string &name, EntityType type, UUID uuid = UUID());
        static Entity CreateSprite(Scene *scene, const std::string &name, UUID uuid = UUID());
        static void RenameEntity(Scene *scene, Entity entity, const std::string &newName);
        static void DestroyEntity(Scene *scene, Entity entity);
        static void DestroyEntity(Scene *scene, UUID uuid);
        static Entity GetEntity(Scene *scene, UUID uuid);
        static Entity DuplicateEntity(Scene *scene, Entity entity, bool addToParent = true);

        static void AddChild(Scene *scene, Entity destination, Entity source);
        static bool ChildExists(Scene *scene, Entity destination, Entity source);
        static bool IsParent(Scene *scene, UUID target, UUID source);
        static Entity FindChild(Scene *scene, Entity parent, UUID uuid);
        static void CalculateParentTransform(Scene *scene, Transform &transform, UUID parentUUID);

        static Ref<Scene> Copy(Ref<Scene> &other);

        using EntityMap = std::unordered_map<UUID, entt::entity>;
        using EntityComponents = std::unordered_map<entt::entity, std::vector<IComponent *>>;

        template<typename... Component>
        static void CopyComponent(entt::registry *destRegistry, entt::registry *srcRegistry, const EntityMap &entityMap, EntityComponents &registerComps)
        {
            ([&]()
                {
                    auto view = srcRegistry->view<Component>();
                    for (auto srcEntity : view)
                    {
                        for (auto [uuid, destEntity] : entityMap)
                        {
                            // key (UUID)
                            if (uuid == srcRegistry->get<ID>(srcEntity).uuid)
                            {
                                Component &comp = destRegistry->emplace_or_replace<Component>(destEntity, srcRegistry->get<Component>(srcEntity));

                                if constexpr (std::is_base_of<IComponent, Component>::value)
                                {
                                    registerComps[destEntity].emplace_back(static_cast<IComponent *>(&comp));
                                }
                            }
                        }
                    }
                }(), ...
            );
        }
    
        template<typename... Component>
        static void CopyComponent(ComponentGroup<Component...>, entt::registry *destRegistry, entt::registry *srcRegistry, const EntityMap &entityMap, EntityComponents &registerComps)
        {
            CopyComponent<Component...>(destRegistry, srcRegistry, entityMap, registerComps);
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

