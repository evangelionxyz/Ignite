#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/uuid.hpp"

namespace ignite
{
    enum CompType : u8
    {
        CompType_Invalid = 0,
        CompType_ID = 1,
        CompType_Camera,
        CompType_Transform,
        CompType_Sprite2D,
        CompType_SkinnedMesh,
        CompType_StaticMesh,
        CompType_MeshRenderer,
        CompType_BoxCollider2D,
        CompType_Rigidbody2D,
        CompType_Rigidbody,
        CompType_BoxCollider,
        CompType_SphereCollider,
    };

    class IComponent
    {
    public:
        virtual ~IComponent() = default;

        template<typename T>
        T *As()
        {
            return static_cast<T *>(this);
        }

        UUID GetCompID() { return m_UUID; }

        bool dirty = true;

        virtual CompType GetType() { return CompType_Invalid; };
    private:
        UUID m_UUID;
    };
}