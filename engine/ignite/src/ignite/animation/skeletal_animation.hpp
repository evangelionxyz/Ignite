#pragma once

#include "keyframes.hpp"

#include <string>
#include <assimp/anim.h>

#include <unordered_map>

namespace ignite {
    
    class AnimationChannel
    {
    public:
        AnimationChannel() = default;
        AnimationChannel(const aiNodeAnim *animNode);

        // time in seconds * ticks per second
        // S * (T/S)
        glm::mat4 CalculateTransform(float timeInTicks);

        Vec3Key translationKeys;
        QuatKey rotationKeys;
        Vec3Key scaleKeys;

        glm::vec3 translation;
        glm::vec3 scale;
        glm::quat rotation;
    };

    class SkeletalAnimation
    {
    public:
        SkeletalAnimation() = default;
        SkeletalAnimation(aiAnimation *anim);

        std::string name;
        float duration = 0;
        float ticksPerSeconds = 1.0f;
        float timeInSeconds = 0.0f;
        bool isPlaying = false;

        std::unordered_map<std::string, AnimationChannel> channels;
    };
}