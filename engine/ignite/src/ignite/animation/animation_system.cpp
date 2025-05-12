#include "animation_system.hpp"

namespace ignite {

    void AnimationSystem::UpdateSkeleton(Skeleton &skeleton, SkeletalAnimation &animation, float timeInSeconds)
    {
        // Find animation key frames
        float animationTime = fmod(timeInSeconds * animation.ticksPerSeconds, animation.duration);

        for (auto &[nodeName, channel] : animation.channels)
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