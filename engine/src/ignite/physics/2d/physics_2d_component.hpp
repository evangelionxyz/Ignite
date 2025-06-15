#pragma once
#include "box2d/types.h"
#include "ignite/scene/icomponent.hpp"

#include <string>
#include <glm/glm.hpp>

namespace ignite
{
    enum Body2DType
    {
        Body2DType_Static = 0, Body2DType_Dynamic, Body2DType_Kinematic
    };

    static b2BodyType GetB2BodyType(Body2DType type)
    {
        switch (type)
        {
        case Body2DType_Static: return b2_staticBody;
        case Body2DType_Dynamic: return b2_dynamicBody;
        case Body2DType_Kinematic: return b2_kinematicBody;
        }
        return b2_staticBody;
    }

    static std::string BodyTypeToString(Body2DType type)
    {
        switch (type)
        {
            case ignite::Body2DType_Static: return "Static";
            case ignite::Body2DType_Dynamic: return "Dynamic";
            case ignite::Body2DType_Kinematic: return "Kinematic";
            default: return "Invalid";
        }
    }

    static Body2DType BodyTypeFromString(const std::string &typeStr)
    {
        if (typeStr == "Static") return Body2DType_Static;
        else if (typeStr == "Dynamic") return Body2DType_Dynamic;
        else if (typeStr == "Kinematic") return Body2DType_Kinematic;
        return Body2DType_Static;
    }

    class Rigidbody2D : public IComponent
    {
    public:
        Body2DType type          = Body2DType_Static;
        glm::vec2 linearVelocity = { 0.0f, 0.0f };
        f32 angularVelocity      = 0.0f;
        f32 gravityScale         = 1.0f;
        f32 linearDamping        = 0.6f;
        f32 angularDamping       = 0.2f;
        bool fixedRotation       = false;
        bool isAwake             = true;
        bool isEnabled           = true;
        bool isEnableSleep       = false;
        b2BodyId bodyId          = {};

        static CompType StaticType() { return CompType_Rigidbody2D; }
        virtual CompType GetType() override { return StaticType(); }
    };

    class BoxCollider2D : public IComponent
    {
    public:
        glm::vec2 size        = {0.5f, 0.5f};
        glm::vec2 offset      = {0.0f, 0.0f};
        glm::vec2 currentSize = {0.5f, 0.5f};
        f32 restitution       = 0.1f;
        f32 friction          = 0.5f;
        f32 density           = 1.0f;
        bool isSensor         = false;

        b2ShapeId shapeId{};

        static CompType StaticType() { return CompType_BoxCollider2D; }
        virtual CompType GetType() override { return StaticType(); }
    };
}