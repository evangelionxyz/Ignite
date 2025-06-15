#include "entity.hpp"

namespace ignite
{
    Entity::Entity()
        : m_Handle(entt::null)
        , m_Scene(nullptr)
        {}
    
    Entity::Entity(const entt::entity e, Scene *scene)
        : m_Handle(e)
        , m_Scene(scene)
        {}
}
