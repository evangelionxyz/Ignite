#pragma once

#include "physics_2d_component.hpp"


#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <ignite/core/types.hpp>

namespace ignite
{
    class Scene;

    class Physics2D
    {
    public:
        Physics2D() = default;
        explicit Physics2D(Scene *scene);
        ~Physics2D();

        void SimulationStart();
        void SimulationStop();

        void Instantiate(entt::entity e);
        void DestroyBody(entt::entity e);

        void Simulate(f32 deltaTime);
        void CreateBoxCollider(BoxCollider2D *box, b2BodyId bodyId, b2Vec2 size);
        void ApplyForce(Rigidbody2D *body, const glm::vec2 &force, const glm::vec2 &point, bool wake);

    private:
        Scene *m_Scene;
        b2WorldId m_WorldId{};
    };
}