#include "physics_2d.hpp"
#include <ignite/scene/scene.hpp>

#include "ignite/scene/component.hpp"

#include "ignite/scene/scene_manager.hpp"

namespace ignite
{
    Physics2D::Physics2D(Scene *scene)
        : m_Scene(scene)
    {
    }

    Physics2D::~Physics2D()
    {
    }

    void Physics2D::SimulationStart()
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        m_WorldId = b2CreateWorld(&worldDef);

        entt::registry *reg = m_Scene->registry;
        auto view = reg->view<Rigidbody2D>();
        for (entt::entity e : view)
        {
            ID &id          = reg->get<ID>(e);
            Rigidbody2D &rb = reg->get<Rigidbody2D>(e);
            Transform &tr   = reg->get<Transform>(e);

            // first, calculate the transformed matrix from parent
            if (id.parent != 0)
            {
                // SceneManager::CalculateParentTransform(m_Scene, tr, id.parent);
            }

            b2BodyDef bodyDef        = b2DefaultBodyDef();
            bodyDef.type             = GetB2BodyType(rb.type);
            bodyDef.position         = { tr.translation.x, tr.translation.y };
            bodyDef.rotation         = b2MakeRot(eulerAngles(tr.rotation).z);
            bodyDef.angularVelocity  = rb.angularVelocity;
            bodyDef.linearVelocity.x = rb.linearVelocity.x;
            bodyDef.linearVelocity.y = rb.linearVelocity.y;
            bodyDef.gravityScale     = rb.gravityScale;
            bodyDef.angularDamping   = rb.angularDamping;
            bodyDef.linearDamping    = rb.linearDamping;
            bodyDef.isEnabled        = rb.isEnabled;
            bodyDef.isAwake          = rb.isAwake;
            bodyDef.fixedRotation    = rb.fixedRotation;

            rb.bodyId = b2CreateBody(m_WorldId, &bodyDef);
            b2Body_SetUserData(rb.bodyId, static_cast<void *>(&e));

            // create box collider
            if (reg->any_of<BoxCollider2D>(e))
            {
                auto &bc = reg->get<BoxCollider2D>(e);
                CreateBoxCollider(&bc, rb.bodyId, b2Vec2(bc.size.x * tr.scale.x, bc.size.y * tr.scale.y));
                b2Shape_SetUserData(bc.shapeId, static_cast<void *>(&e));
            }
        }
    }

    void Physics2D::SimulationStop()
    {
        b2DestroyWorld(m_WorldId);
    }

    void Physics2D::Instantiate(entt::entity e)
    {
        entt::registry *reg = m_Scene->registry;

        ID &id = reg->get<ID>(e);
        Transform &tr      = reg->get<Transform>(e);

        // first, calculate the transformed matrix from parent
        if (id.parent != 0)
        {
            // SceneManager::CalculateParentTransform(m_Scene, tr, id.parent);
        }

        Rigidbody2D &rb          = reg->get<Rigidbody2D>(e);

        b2BodyDef bodyDef        = b2DefaultBodyDef();
        bodyDef.type             = GetB2BodyType(rb.type);
        bodyDef.position         = { tr.translation.x, tr.translation.y };
        bodyDef.rotation         = b2MakeRot(eulerAngles(tr.rotation).z);
        bodyDef.angularVelocity  = rb.angularVelocity;
        bodyDef.linearVelocity.x = rb.linearVelocity.x;
        bodyDef.linearVelocity.y = rb.linearVelocity.y;
        bodyDef.gravityScale     = rb.gravityScale;
        bodyDef.angularDamping   = rb.angularDamping;
        bodyDef.linearDamping    = rb.linearDamping;
        bodyDef.isEnabled        = rb.isEnabled;
        bodyDef.isAwake          = rb.isAwake;
        bodyDef.fixedRotation    = rb.fixedRotation;

        rb.bodyId = b2CreateBody(m_WorldId, &bodyDef);
        b2Body_SetUserData(rb.bodyId, static_cast<void *>(&e));

        // create box collider
        if (reg->any_of<BoxCollider2D>(e))
        {
            auto &bc = reg->get<BoxCollider2D>(e);
            CreateBoxCollider(&bc, rb.bodyId, b2Vec2(bc.size.x * tr.scale.x, bc.size.y * tr.scale.y));
            b2Shape_SetUserData(bc.shapeId, static_cast<void *>(&e));
        }
    }

    void Physics2D::DestroyBody(entt::entity e)
    {
        entt::registry *reg = m_Scene->registry;
        if (reg->any_of<Rigidbody2D>(e))
        {
            if (reg->any_of<BoxCollider2D>(e))
            {
                auto &bc = reg->get<BoxCollider2D>(e);

                // check b2world is already created
                if (bc.shapeId.world0 != 0)
                {
                    b2DestroyShape(bc.shapeId, false);
                }
                
            }

            Rigidbody2D &rb = reg->get<Rigidbody2D>(e);

            // check b2world is already created
            if (rb.bodyId.world0 != 0)
            {
                b2DestroyBody(rb.bodyId);
            }
        }
    }

    void Physics2D::Simulate(f32 deltaTime)
    {
        constexpr i32 subStepCount = 12;
        b2World_Step(m_WorldId, deltaTime, subStepCount);

        const auto reg = m_Scene->registry;
        for (const auto e : reg->view<Rigidbody2D>())
        {
            ID &id = reg->get<ID>(e);
            Transform &tr = reg->get<Transform>(e);
            Rigidbody2D &rb = reg->get<Rigidbody2D>(e);

            if (reg->any_of<BoxCollider2D>(e))
            {
                BoxCollider2D &bc = reg->get<BoxCollider2D>(e);
                b2Shape_SetFriction(bc.shapeId, bc.friction);
                b2Shape_SetDensity(bc.shapeId, bc.density, true);
                b2Shape_SetRestitution(bc.shapeId, bc.restitution);

                f32 width = glm::abs(bc.size.x * tr.scale.x);
                f32 height = glm::abs(bc.size.y * tr.scale.y);

                width = glm::max(width, glm::epsilon<f32>());
                height = glm::max(height, glm::epsilon<f32>());
                const b2Polygon boxShape = b2MakeBox(width, height);
                b2Shape_SetPolygon(bc.shapeId, &boxShape);
            }

            // first, calculate the local transform
            const auto [x, y] = b2Body_GetPosition(rb.bodyId);
            const b2Rot rotation = b2Body_GetRotation(rb.bodyId);
            tr.localTranslation = { x, y, tr.translation.z };
            tr.localRotation = glm::quat({ 0.0f, 0.0f, b2Rot_GetAngle(rotation) });

            tr.translation = tr.localTranslation;
            tr.rotation = tr.localRotation;
        }
    }

    void Physics2D::CreateBoxCollider(BoxCollider2D *box, b2BodyId bodyId, b2Vec2 size)
    {
        f32 width = glm::abs(size.x);
        f32 height = glm::abs(size.y);
        width = glm::max(width, glm::epsilon<f32>());
        height = glm::max(height, glm::epsilon<f32>());
        
        box->currentSize = { width, height };

        const b2Polygon boxShape = b2MakeBox(width, height);

        b2ShapeDef shapeDef           = b2DefaultShapeDef();
        shapeDef.density              = box->density;
        shapeDef.material.friction    = box->friction;
        shapeDef.material.restitution = box->restitution;
        shapeDef.isSensor             = box->isSensor;
        box->shapeId                  = b2CreatePolygonShape(bodyId, &shapeDef, &boxShape);
    }

    void Physics2D::ApplyForce(Rigidbody2D *body, const glm::vec2 &force, const glm::vec2 &point, bool wake)
    {
        b2Body_ApplyForce(body->bodyId, {force.x, force.y}, {point.x, point.y}, wake);
    }

} // namespace ignite
