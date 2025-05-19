#include "animation_system.hpp"

#include "ignite/core/logger.hpp"
#include "ignite/graphics/model.hpp"

namespace ignite {

    void AnimationSystem::PlayAnimation(const Ref<Model> &model, int animIndex /*= 0*/)
    {
        LOG_ASSERT(model , "[Animationl] model is null!");
        if (animIndex < model->animations.size())
        {
            model->animations[animIndex]->isPlaying = true;
            model->activeAnimIndex = animIndex;
        }
    }

    void AnimationSystem::PlayAnimation(std::vector<Ref<SkeletalAnimation>> &animations, int animIndex /*= 0*/)
    {
        if (animIndex < animations.size())
        {
            animations[animIndex]->isPlaying = true;
        }
    }

    void AnimationSystem::UpdateAnimation(const Ref<Model> &model, float timeInSeconds)
    {
        if (UpdateSkeleton(model->skeleton, model->GetActiveAnimation(), timeInSeconds))
        {
            model->boneTransforms = GetFinalJointTransforms(model->skeleton);
        }
    }

    bool AnimationSystem::UpdateSkeleton(Skeleton &skeleton, const Ref<SkeletalAnimation> &animation, float timeInSeconds)
    {
        if (!animation)
            return false;

        // Find animation key frames
        float animationTime = fmod(timeInSeconds * animation->ticksPerSeconds, animation->duration);

        for (auto &[nodeName, channel] : animation->channels)
        {
            auto it = skeleton.nameToJointMap.find(nodeName);
            if (it != skeleton.nameToJointMap.end())
            {
                i32 jointIndex = it->second;
                skeleton.joints[jointIndex].localTransform = channel.CalculateTransform(animationTime);
            }
        }

        // Update global transforms
        UpdateGlobalTransforms(skeleton);

        return true;
    }

    void AnimationSystem::UpdateGlobalTransforms(Skeleton &skeleton)
    {
        // Important optiomization: Calculate global transforms in hierarchy order
        for (size_t i = 0; i < skeleton.joints.size(); ++i)
        {
            Joint &joint = skeleton.joints[i];

            if (joint.parentJointId == -1)
            {
                // Root joint
                joint.globalTransform = joint.localTransform;
            }
            else
            {
                // Child joint
                joint.globalTransform = skeleton.joints[joint.parentJointId].globalTransform * joint.localTransform;
            }
        }
    }

    std::vector<glm::mat4> AnimationSystem::GetFinalJointTransforms(const Skeleton &skeleton)
    {
        std::vector<glm::mat4> finalTransforms;
        finalTransforms.reserve(skeleton.joints.size());

        for (const Joint &joint : skeleton.joints)
        {
            // Final transform = globalTransform * inverseBindPose
            finalTransforms.push_back(joint.globalTransform * joint.inverseBindPose);
        }

        return finalTransforms;
    }

}