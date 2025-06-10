#pragma once
#include "component.hpp"
#include "ignite/physics/2d/physics_2d_component.hpp"

namespace ignite
{
    template<typename... Component>
    struct ComponentGroup
    {
    };

    using AllComponents = ComponentGroup<
        // ID, // do not copy ID component
        Transform, 
        Sprite2D,
        Rigidbody2D,
        BoxCollider2D,
        SkinnedMesh,
        MeshRenderer,
        Rigibody,
        BoxCollider,
        SphereCollider
    >; 
}