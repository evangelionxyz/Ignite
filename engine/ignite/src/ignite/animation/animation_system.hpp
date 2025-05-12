#pragma once

#include "ignite/graphics/mesh.hpp"

namespace ignite {
    
    class AnimationSystem
    {
    public:
        static void UpdateSkeleton(Skeleton &skeleton, SkeletalAnimation &animation, float timeInSeconds);
        static void UpdateGlobalTransforms(Skeleton &skeleton);
        static std::vector<glm::mat4> GetFinalJointTransforms(const Skeleton &skeleton);
    };
}