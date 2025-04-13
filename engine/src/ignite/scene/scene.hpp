#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <ignite/physics/2d/physics_2d.hpp>
#include <nvrhi/nvrhi.h>
#include <unordered_map>
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"
#include "ignite/core/uuid.hpp"

namespace ignite
{
    class Camera;
    class Physics2D;
    class Entity;

    using EntityMap = std::unordered_map<UUID, entt::entity>;
    using EntityComponents = std::unordered_map<entt::entity, std::vector<IComponent *>>;
    using StringCounterMap = std::unordered_map<std::string, i32>;

    class Scene
    {
    public:
        Scene() = default;
        explicit Scene(const std::string &name);

        ~Scene();

        void OnStart();
        void OnStop();
        
        void OnUpdate(f32 deltaTime);
        void OnRender(Camera *camera, nvrhi::IFramebuffer *framebuffer);

        static Ref<Scene> Copy(Ref<Scene> other);

        std::string name;
        entt::registry *registry = nullptr;

        EntityMap entities;
        EntityComponents registeredComps;
        StringCounterMap entityNamesMapCounter;
        std::vector<std::string> entityNames;

        Scope<Physics2D> physics2D;

        bool IsPlaying() const { return m_Playing; }
    
    private:
        bool m_Playing = false;
    };
}
