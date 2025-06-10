#include "jolt_physics.hpp"
#include "ignite/core/types.hpp"

#include "ignite/scene/scene.hpp"

namespace ignite
{
    static constexpr int cMaxPhysicsJobs = 2048;
    static constexpr unsigned int cNumBodies = 20480;
    static constexpr unsigned int cNumBodyMutexes = 0;
    static constexpr unsigned int cMaxBodyPairs = 64000;
    static constexpr unsigned int cMaxContactConstraints = 20480;

    static JoltPhysics *s_JoltInstance = nullptr;

    void JoltPhysics::Init()
    {
        s_JoltInstance = new JoltPhysics();

        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        s_JoltInstance->tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(32 * 1024 * 1024);
        s_JoltInstance->jobSystem = std::make_unique<JPH::JobSystemThreadPool>(cMaxPhysicsJobs, 8,
            std::thread::hardware_concurrency() - 1);

        LOG_WARN("[Jolt Physics] Initalized");
    }

    void JoltPhysics::Shutdown()
    {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        if (s_JoltInstance)
        {
           delete s_JoltInstance;
        }

        LOG_WARN("[Jolt Physics] Shutdown");
    }

    JoltPhysics *JoltPhysics::GetInstance()
    {
        return s_JoltInstance;
    }

    JoltScene::JoltScene(Scene *scene)
        : m_Scene(scene)
    {
    }

    void JoltScene::SimulationStart()
    {
        m_PhysicsSystem.Init(cNumBodies, cNumBodyMutexes, cMaxBodyPairs,
            cMaxContactConstraints, s_JoltInstance->broadPhaseLayer,
            s_JoltInstance->objectVsBroadPhaseLayerFilter,
            s_JoltInstance->objectLayerPairFilter);

        // m_PhysicsSystem.SetBodyActivationListener(s_JoltInstance->bodyActivationListener.get());
        // m_PhysicsSystem.SetContactListener(s_JoltInstance->contactListener.get());
        m_PhysicsSystem.SetGravity(GlmToJoltVec3(m_Scene->physicsGravity));
        m_PhysicsSystem.OptimizeBroadPhase();

        m_BodyInterface = &m_PhysicsSystem.GetBodyInterface();

        for (entt::entity e : m_Scene->registry->view<Rigibody>())
        {
            InstantiateEntity(Entity{ e, m_Scene });
        }
    }

    void JoltScene::SimulationStop()
    {
        for (entt::entity e : m_Scene->registry->view<Rigibody>())
        {
            DestroyEntity(Entity{ e, m_Scene });
        }
    }

    void JoltScene::Simulate(float deltaTime)
    {
        for (const auto id : m_Scene->registry->view<Rigibody>())
        {
            Entity entity = { id, m_Scene };
            const Rigibody &rb = entity.GetComponent<Rigibody>();
            Transform &tc = entity.GetComponent<Transform>();
            ID &idc = entity.GetComponent<ID>();

            if (!rb.body)
                continue;

            // we don't care about the parent
            tc.localTranslation = JoltToGlmVec3(rb.body->GetPosition());
            tc.localRotation = JoltToGlmQuat(rb.body->GetRotation());
            tc.translation = tc.localTranslation;
            tc.rotation = tc.localRotation;
        }

        m_PhysicsSystem.Update(deltaTime, 1, 
            s_JoltInstance->tempAllocator.get(), 
            s_JoltInstance->jobSystem.get());
    }

    void JoltScene::InstantiateEntity(Entity entity)
    {
        if (entity.HasComponent<Rigibody>())
        {
            auto &rb = entity.GetComponent<Rigibody>();

            if (entity.HasComponent<BoxCollider>())
            {
                CreateBoxCollider(entity);
            }

            if (entity.HasComponent<SphereCollider>())
            {
                CreateSphereCollider(entity);
            }
        }
    }

    void JoltScene::DestroyEntity(Entity entity)
    {
        if (entity.HasComponent<Rigibody>())
        {
            auto &rb = entity.GetComponent<Rigibody>();
            if (rb.body)
            {
                m_BodyInterface->RemoveBody(rb.body->GetID());
                m_BodyInterface->DestroyBody(rb.body->GetID());
                rb.body = nullptr;
            }
        }
    }

    JPH::BodyCreationSettings JoltScene::CreateBody(JPH::ShapeRefC shape, Rigibody &rb, const glm::vec3 &position, const glm::quat &rotation)
    {
        JPH::BodyCreationSettings bodySettings(shape, GlmToJoltVec3(position), GlmToJoltQuat(rotation),
            rb.isStatic ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
            rb.isStatic ? Layers::NON_MOVING : Layers::MOVING);

        bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::None;
        if (rb.rotateX) bodySettings.mAllowedDOFs |= JPH::EAllowedDOFs::RotationX;
        if (rb.rotateY) bodySettings.mAllowedDOFs |= JPH::EAllowedDOFs::RotationY;
        if (rb.rotateZ) bodySettings.mAllowedDOFs |= JPH::EAllowedDOFs::RotationZ;
        if (rb.moveX) bodySettings.mAllowedDOFs |= JPH::EAllowedDOFs::TranslationX;
        if (rb.moveY) bodySettings.mAllowedDOFs |= JPH::EAllowedDOFs::TranslationY;
        if (rb.moveZ) bodySettings.mAllowedDOFs |= JPH::EAllowedDOFs::TranslationZ;

        switch (rb.MotionQuality)
        {
        case Rigibody::EMotionQuality::Discrete:
            bodySettings.mMotionQuality = JPH::EMotionQuality::Discrete;
            break;
        case Rigibody::EMotionQuality::LinearCast:
            bodySettings.mMotionQuality = JPH::EMotionQuality::LinearCast;
            break;
        }

        return bodySettings;
    }

    JPH::BodyInterface *JoltScene::GetBodyInterface() const
    {
        return m_BodyInterface;
    }

    void JoltScene::CreateBoxCollider(Entity entity)
    {
        auto &tc = entity.GetComponent<Transform>();
        auto &rb = entity.GetComponent<Rigibody>();
        auto &col = entity.GetComponent<BoxCollider>();

        glm::vec3 halfExtents = col.scale * tc.scale;
        JPH::BoxShapeSettings shapeSettings(GlmToJoltVec3(halfExtents));

        JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
        JPH::ShapeRefC shape = shapeResult.Get();

        JPH::BodyCreationSettings bodySettings = CreateBody(shape, rb, tc.translation, tc.rotation);

        JPH::Body *body = m_BodyInterface->CreateBody(bodySettings);
        if (body)
        {
            JPH::BodyID bodyId = body->GetID();
            m_BodyInterface->AddBody(bodyId, JPH::EActivation::Activate);
            m_BodyInterface->SetFriction(bodyId, col.friction);
            m_BodyInterface->SetRestitution(bodyId, col.restitution);
            rb.body = body;
        }

        col.shape = (void *)shape.GetPtr();
    }

    void JoltScene::CreateCapsuleCollider(Entity entity)
    {

    }

    void JoltScene::CreateSphereCollider(Entity entity)
    {
        auto &tc = entity.GetComponent<Transform>();
        auto &rb = entity.GetComponent<Rigibody>();
        auto &col = entity.GetComponent<SphereCollider>();

        JPH::SphereShapeSettings shapeSettings(col.radius * 2.0f);

        JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
        JPH::ShapeRefC shape = shapeResult.Get();

        JPH::BodyCreationSettings bodySettings = CreateBody(shape, rb, tc.translation, tc.rotation);

        JPH::Body *body = m_BodyInterface->CreateBody(bodySettings);
        if (body)
        {
            JPH::BodyID bodyId = body->GetID();
            m_BodyInterface->AddBody(bodyId, JPH::EActivation::Activate);
            m_BodyInterface->SetFriction(bodyId, col.friction);
            m_BodyInterface->SetRestitution(bodyId, col.restitution);
            rb.body = body;
        }

        col.shape = (void *)shape.GetPtr();
    }

    void JoltScene::AddForce(const JPH::Body &body, const glm::vec3 &force)
    {
        m_BodyInterface->AddForce(body.GetID(), GlmToJoltVec3(force));
    }

    void JoltScene::AddTorque(const JPH::Body &body, const glm::vec3 &torque)
    {
        m_BodyInterface->AddTorque(body.GetID(), GlmToJoltVec3(torque));
    }

    void JoltScene::AddForceAndTorque(const JPH::Body &body, const glm::vec3 &force, const glm::vec3 &torque)
    {
        m_BodyInterface->AddForceAndTorque(body.GetID(), GlmToJoltVec3(force), GlmToJoltVec3(torque));
    }

    void JoltScene::AddAngularImpulse(const JPH::Body &body, const glm::vec3 &impulse)
    {
        m_BodyInterface->AddAngularImpulse(body.GetID(), GlmToJoltVec3(impulse));
    }

    void JoltScene::ActivateBody(const JPH::Body &body)
    {
        m_BodyInterface->ActivateBody(body.GetID());
    }

    void JoltScene::DeactivateBody(const JPH::Body &body)
    {
        m_BodyInterface->DeactivateBody(body.GetID());
    }

    void JoltScene::DestroyBody(const JPH::Body &body)
    {
        m_BodyInterface->DestroyBody(body.GetID());
    }

    bool JoltScene::IsActive(const JPH::Body &body)
    {
        return false; m_BodyInterface->IsActive(body.GetID());
    }

    void JoltScene::MoveKinematic(const JPH::Body &body, const glm::vec3 &targetPosition, const glm::vec3 &targetRotation, float deltaTime)
    {
        m_BodyInterface->MoveKinematic(body.GetID(), GlmToJoltVec3(targetPosition), GlmToJoltQuat(targetRotation), deltaTime);
    }

    void JoltScene::AddImpulse(const JPH::Body &body, const glm::vec3 &impulse)
    {
        m_BodyInterface->AddImpulse(body.GetID(), GlmToJoltVec3(impulse));
    }

    void JoltScene::AddLinearVelocity(const JPH::Body &body, const glm::vec3 &velocity)
    {
        m_BodyInterface->AddLinearVelocity(body.GetID(), GlmToJoltVec3(velocity));
    }

    void JoltScene::SetPosition(const JPH::Body &body, const glm::vec3 &position, bool activate)
    {
        m_BodyInterface->SetPosition(body.GetID(), GlmToJoltVec3(position), activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }

    void JoltScene::SetEulerAngleRotation(const JPH::Body &body, const glm::vec3 &rotation, bool activate)
    {
        m_BodyInterface->SetRotation(body.GetID(), GlmToJoltQuat(rotation), activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }

    void JoltScene::SetRotation(const JPH::Body &body, const glm::quat &rotation, bool activate)
    {
        m_BodyInterface->SetRotation(body.GetID(), GlmToJoltQuat(rotation), activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    }

    void JoltScene::SetLinearVelocity(const JPH::Body &body, const glm::vec3 &vel)
    {
        m_BodyInterface->SetLinearVelocity(body.GetID(), GlmToJoltVec3(vel));
    }

    void JoltScene::SetFriction(const JPH::Body &body, float value)
    {
        m_BodyInterface->SetFriction(body.GetID(), value);
    }

    void JoltScene::SetRestitution(const JPH::Body &body, float value)
    {
        m_BodyInterface->SetRestitution(body.GetID(), value);
    }

    void JoltScene::SetGravityFactor(const JPH::Body &body, float value)
    {
        m_BodyInterface->SetGravityFactor(body.GetID(), value);
    }

    float JoltScene::GetRestitution(const JPH::Body &body)
    {
        return m_BodyInterface->GetRestitution(body.GetID());
    }

    float JoltScene::GetFriction(const JPH::Body &body)
    {
        return m_BodyInterface->GetFriction(body.GetID());
    }

    float JoltScene::GetGravityFactor(const JPH::Body &body)
    {
        return m_BodyInterface->GetGravityFactor(body.GetID());
    }

    glm::vec3 JoltScene::GetPosition(const JPH::Body &body)
    {
        return JoltToGlmVec3(m_BodyInterface->GetPosition(body.GetID()));
    }

    glm::vec3 JoltScene::GetEulerAngles(const JPH::Body &body)
    {
        return glm::eulerAngles(JoltToGlmQuat(m_BodyInterface->GetRotation(body.GetID())));
    }

    glm::quat JoltScene::GetRotation(const JPH::Body &body)
    {
        return JoltToGlmQuat(m_BodyInterface->GetRotation(body.GetID()));
    }

    glm::vec3 JoltScene::GetCenterOfMassPosition(const JPH::Body &body)
    {
        return JoltToGlmVec3(m_BodyInterface->GetCenterOfMassPosition(body.GetID()));
    }

    glm::vec3 JoltScene::GetLinearVelocity(const JPH::Body &body)
    {
        return JoltToGlmVec3(m_BodyInterface->GetLinearVelocity(body.GetID()));
    }

    void JoltScene::SetMaxLinearVelocity(JPH::Body &body, float max)
    {
        body.GetMotionProperties()->SetMaxLinearVelocity(max);
    }

    void JoltScene::SetMaxAngularVelocity(JPH::Body &body, float max)
    {
        body.GetMotionProperties()->SetMaxAngularVelocity(max);
    }

}