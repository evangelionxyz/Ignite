#pragma once

#include "skeletal_animation.hpp"

namespace ignite {

    AnimationChannel::AnimationChannel(const aiNodeAnim *animNode)
        : translation(0.0f), scale(1.0f), rotation({ 1.0f, 0.0f, 0.0f, 0.0f })
    {
        for (u32 positionIndex = 0; positionIndex < animNode->mNumPositionKeys; ++positionIndex)
        {
            const aiVector3D aiPos = animNode->mPositionKeys[positionIndex].mValue;
            const f32 anim_time = static_cast<f32>(animNode->mPositionKeys[positionIndex].mTime);
            translationKeys.AddFrame({ Math::AssimpToGlmVec3(aiPos), anim_time });
        }

        for (u32 rotationIndex = 0; rotationIndex < animNode->mNumRotationKeys; ++rotationIndex)
        {
            const aiQuaternion aiQuat = animNode->mRotationKeys[rotationIndex].mValue;
            const f32 anim_time = static_cast<f32>(animNode->mRotationKeys[rotationIndex].mTime);
            rotationKeys.AddFrame({ Math::AssimpToGlmQuat(aiQuat), anim_time });
        }

        for (u32 scaleIndex = 0; scaleIndex < animNode->mNumScalingKeys; ++scaleIndex)
        {
            const aiVector3D aiScale = animNode->mScalingKeys[scaleIndex].mValue;
            const f32 anim_time = static_cast<f32>(animNode->mScalingKeys[scaleIndex].mTime);
            scaleKeys.AddFrame({ Math::AssimpToGlmVec3(aiScale), anim_time });
        }
    }

    // time in seconds * ticks per second
    // S * (T/S)
    glm::mat4 AnimationChannel::CalculateTransform(float timeInTicks)
    {
        translation = translationKeys.InterpolateTranslation(timeInTicks);
        rotation = rotationKeys.InterpolateRotation(timeInTicks);
        scale = scaleKeys.InterpolateScaling(timeInTicks);

        return glm::translate(glm::mat4(1.0f), translation)
            * glm::toMat4(rotation)
            * glm::scale(glm::mat4(1.0f), scale);
    }

    SkeletalAnimation::SkeletalAnimation(aiAnimation *anim)
    {
        name = anim->mName.data;

        modf(static_cast<float>(anim->mDuration), &duration);

        ticksPerSeconds = static_cast<float>(anim->mTicksPerSecond);
        if (anim->mTicksPerSecond == 0)
            ticksPerSeconds = 25.0f;

        for (uint32_t i = 0; i < anim->mNumChannels; ++i)
        {
            aiNodeAnim *animNode = anim->mChannels[i];

            std::string nodeName = animNode->mNodeName.data;
            channels[nodeName] = AnimationChannel(animNode);
        }
    }

}