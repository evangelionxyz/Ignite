#pragma once
#include "ignite/core/uuid.hpp"

namespace ignite {

    class Scene;

    class TransformSystem
    {
    public:
        static void UpdateTransforms(Scene *scene);
    private:
        static void UpdateChildTransform(Scene *scene, UUID parentId);
    };

}