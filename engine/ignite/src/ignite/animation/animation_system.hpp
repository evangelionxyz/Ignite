#pragma once

#include "ignite/graphics/mesh.hpp"
#include "ignite/core/types.hpp"

namespace ignite {
    
    class Model;

    class AnimationSystem
    {
    public:
        static void PlayAnimation(const Ref<Model> &model, int animIndex = 0);
        static void UpdateAnimation(const Ref<Model> &model, float timeInSeconds);
        static bool UpdateSkeleton(Skeleton &skeleton, const Ref<SkeletalAnimation> &animation, float timeInSeconds);
        static void UpdateGlobalTransforms(Skeleton &skeleton);
        static std::vector<glm::mat4> GetFinalJointTransforms(const Skeleton &skeleton);
    };
}