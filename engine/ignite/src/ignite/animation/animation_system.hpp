#pragma once

#include "ignite/graphics/mesh.hpp"
#include "ignite/core/types.hpp"

namespace ignite {
    
    class Model;

    class AnimationSystem
    {
    public:
        static void PlayAnimation(std::vector<Ref<SkeletalAnimation>> &animations, int animIndex = 0);
        static void PlayAnimation(Ref<Model> &model, int animIndex = 0);
        static void UpdateAnimation(Ref<Model> &model, float timeInSeconds);
        static void ApplySkeletonToEntities(Scene *scene, Ref<Skeleton> &skeleton); 
        static bool UpdateSkeleton(Ref<Skeleton> &skeleton, const Ref<SkeletalAnimation> &animation, float timeInSeconds);
        static void UpdateGlobalTransforms(Ref<Skeleton> &skeleton);
        static std::vector<glm::mat4> GetFinalJointTransforms(const Ref<Skeleton> &skeleton);
    };
}