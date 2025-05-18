#pragma once

#include "ignite/core/types.hpp"
#include "ignite/core/uuid.hpp"

namespace ignite
{
    enum CompType : u8
    {
        CompType_Invalid = 0,
        CompType_ID = 1,
        CompType_Transform,
        CompType_Sprite2D,
        CompType_SkinnedMeshRenderer,
        CompType_StaticMeshRenderer,
        CompType_BoxCollider2D,
        CompType_Rigidbody2D
    };

    class IComponent
    {
    public:

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