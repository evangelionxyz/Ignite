#pragma once

#include "ignite/core/logger.hpp"
#include "ignite/scene/entity.hpp"
#include "ignite/scene/component.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace ignite {

    static JPH::Vec3 GlmToJoltVec3(const glm::vec3 &v)
    {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    static glm::vec3 JoltToGlmVec3(const JPH::Vec3 &v)
    {
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    static JPH::Quat GlmToJoltQuat(const glm::quat &q)
    {
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    static glm::quat JoltToGlmQuat(const JPH::Quat &q)
    {
        return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }

    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }

    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr uint32_t NUM_LAYERS(2);
    }

    class JoltObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    class JoltBroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        JoltBroadPhaseLayerInterfaceImpl()
        {
            m_ObjectToBP[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            m_ObjectToBP[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint32_t GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            LOG_ASSERT(inLayer < Layers::NUM_LAYERS, "[Jolt] Invalid Layer");
            return m_ObjectToBP[inLayer];
        }

    private:
        JPH::BroadPhaseLayer m_ObjectToBP[Layers::NUM_LAYERS];
    };

    class JoltObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                LOG_ASSERT(false, "");
                return false;
            }
        }
    };

    class JoltPhysics
    {
    public:
        static void Init();
        static void Shutdown();

        static JoltPhysics *GetInstance();

        Scope<JPH::TempAllocator> tempAllocator;
        Scope<JPH::JobSystem> jobSystem;
        Scope<JPH::BodyActivationListener> bodyActivationListener;
        Scope<JPH::ContactListener> contactListener;

        JoltBroadPhaseLayerInterfaceImpl broadPhaseLayer;
        JoltObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
        JoltObjectLayerPairFilterImpl objectLayerPairFilter;
    };

    class Scene;

    class JoltScene
    {
    public:
        JoltScene(Scene *scene);

        void SimulationStart();
        void SimulationStop();

        void Simulate(float deltaTime);

        void InstantiateEntity(Entity entity);
        void DestroyEntity(Entity entity);

        JPH::BodyCreationSettings CreateBody(JPH::ShapeRefC shape, Rigibody &rb, const glm::vec3 &position, const glm::quat &rotation);

        void CreateBoxCollider(Entity entity);
        void CreateCapsuleCollider(Entity entity);
        void CreateSphereCollider(Entity entity);

        void AddForce(const JPH::Body &body, const glm::vec3 &force);
        void AddTorque(const JPH::Body &body, const glm::vec3 &torque);
        void AddForceAndTorque(const JPH::Body &body, const glm::vec3 &force, const glm::vec3 &torque);
        void AddAngularImpulse(const JPH::Body &body, const glm::vec3 &impulse);
        void ActivateBody(const JPH::Body &body);
        void DeactivateBody(const JPH::Body &body);
        void DestroyBody(const JPH::Body &body);
        bool IsActive(const JPH::Body &body);
        void MoveKinematic(const JPH::Body &body, const glm::vec3 &targetPosition, const glm::vec3 &targetRotation, float deltaTime);
        void AddImpulse(const JPH::Body &body, const glm::vec3 &impulse);
        void AddLinearVelocity(const JPH::Body &body, const glm::vec3 &velocity);
        void SetPosition(const JPH::Body &body, const glm::vec3 &position, bool activate);
        void SetEulerAngleRotation(const JPH::Body &body, const glm::vec3 &rotation, bool activate);
        void SetRotation(const JPH::Body &body, const glm::quat &rotation, bool activate);
        void SetLinearVelocity(const JPH::Body &body, const glm::vec3 &vel);
        void SetFriction(const JPH::Body &body, float value);
        void SetRestitution(const JPH::Body &body, float value);
        void SetGravityFactor(const JPH::Body &body, float value);
        float GetRestitution(const JPH::Body &body);
        float GetFriction(const JPH::Body &body);
        float GetGravityFactor(const JPH::Body &body);
        glm::vec3 GetPosition(const JPH::Body &body);
        glm::vec3 GetEulerAngles(const JPH::Body &body);
        glm::quat GetRotation(const JPH::Body &body);
        glm::vec3 GetCenterOfMassPosition(const JPH::Body &body);
        glm::vec3 GetLinearVelocity(const JPH::Body &body);
        void SetMaxLinearVelocity(JPH::Body &body, float max);
        void SetMaxAngularVelocity(JPH::Body &body, float max);

        JPH::BodyInterface *GetBodyInterface() const;

    private:
        Scene *m_Scene;
        JPH::BodyInterface *m_BodyInterface;
        JPH::PhysicsSystem m_PhysicsSystem;

    };
}
