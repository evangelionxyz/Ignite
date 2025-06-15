#pragma once



#include "ignite/graphics/mesh.hpp"
#include "ignite/core/types.hpp"

namespace ignite {
    
    class Model;

    class AnimationSystem
    {
    public:
        static void PlayAnimation(std::vector<SkeletalAnimation> &animations, int animIndex = 0);
        static void ApplySkeletonToEntities(Scene *scene, Skeleton &skeleton); 
        static bool UpdateSkeleton(Skeleton &skeleton, SkeletalAnimation &animation, float timeInSeconds);
        static void UpdateGlobalTransforms(Skeleton &skeleton);
        static std::vector<glm::mat4> GetFinalJointTransforms(const Skeleton &skeleton);
    };
}