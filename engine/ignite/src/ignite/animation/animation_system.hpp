#pragma once

#include "ignite/graphics/mesh.hpp"
#include "ignite/core/types.hpp"

namespace ignite {
    
    class Model;

    class AnimationSystem
    {
    public:
        static void PlayAnimation(std::vector<SkeletalAnimation> &animations, int animIndex = 0);
        static void ApplySkeletonToEntities(Scene *scene, const Ref<Skeleton> &skeleton); 
        static bool UpdateSkeleton(Ref<Skeleton> &skeleton, SkeletalAnimation &animation, float timeInSeconds);
        static void UpdateGlobalTransforms(Ref<Skeleton> &skeleton);
        static std::vector<glm::mat4> GetFinalJointTransforms(const Ref<Skeleton> &skeleton);
    };
}