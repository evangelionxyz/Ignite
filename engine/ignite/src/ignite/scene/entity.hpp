#pragma once

#include "scene.hpp"
#include "component.hpp"
#include "ignite/core/types.hpp"
#include <entt/entt.hpp>

namespace ignite
{
    class Entity
    {
    public:
        Entity();
        Entity(entt::entity e, Scene *scene);
        Entity(const Entity &other) = default;

        ~Entity()
        {
            m_Handle = {entt::null};
            m_Scene = nullptr;
        }

        template<typename T, typename... Args>
        T &AddComponent(Args &&... args)
        {
            T &comp = m_Scene->registry->get_or_emplace<T>(m_Handle, std::forward<Args>(args)...);
            if constexpr (std::is_base_of<IComponent, T>::value)
            {
                m_Scene->registeredComps[m_Handle].emplace_back(static_cast<IComponent*>(&comp));
                m_Scene->OnComponentAdded<T>(*this, comp);
            }

            return comp;
        }

        template<typename T, typename... Args>
        T &AddOrReplaceComponent(Args &&... args)
        {
            T &comp = m_Scene->registry->emplace_or_replace<T>(m_Handle, std::forward<Args>(args)...);
            if constexpr (std::is_base_of<IComponent, T>::value)
            {
                m_Scene->registeredComps[m_Handle].emplace_back(static_cast<IComponent*>(&comp));
                m_Scene->OnComponentAdded<T>(*this, comp);
            }
            return comp;
        }

        template<typename T>
        T &GetComponent()
        {
            return m_Scene->registry->get<T>(m_Handle);
        }

        template<typename T>
        bool HasComponent() const
        {
            return m_Scene->registry->all_of<T>(m_Handle);
        }

        template<typename T>
        void RemoveComponent()
        {
            T &comp = m_Scene->registry->get<T>(m_Handle);
            m_Scene->registry->remove<T>(m_Handle);

            std::vector<IComponent *> &regComps = m_Scene->registeredComps[m_Handle];

            regComps.erase(std::remove_if(regComps.begin(), regComps.end(), [&](IComponent *regComp)
            {
                return ((IComponent *) &comp) == regComp;
            }), regComps.end());
        }

        bool IsValid() const
        {
            if (!m_Scene)
                return false;

            return m_Scene->registry->valid(m_Handle);
        }

        operator entt::entity() const { return m_Handle; }
        operator i32() const { return static_cast<i32>(m_Handle); }
        operator u32() const { return static_cast<u32>(m_Handle); }
        operator u64() const { return static_cast<u64>(m_Handle); }

        operator bool() const { return IsValid(); }

        bool operator==(const Entity &other) const { return other.m_Handle == m_Handle && other.m_Scene == m_Scene; }
        bool operator!=(const Entity &other) const { return !(*this == other); }

        UUID GetUUID() { return GetComponent<ID>().uuid; }
        UUID GetParentUUID() { return GetComponent<ID>().parent; }
        Transform &GetTransform() { return GetComponent<Transform>(); }
        
        const std::string &GetName() { return GetComponent<ID>().name; }

    private:
        entt::entity m_Handle;
        Scene *m_Scene;
    };
}