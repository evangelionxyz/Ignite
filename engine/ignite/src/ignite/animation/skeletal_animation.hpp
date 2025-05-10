#pragma once

#include "keyframes.hpp"

#include <string>
#include <assimp/anim.h>

#include <unordered_map>

namespace ignite {
    
    struct Joint
    {

    };

    class AnimationNode
    {
    public:
        AnimationNode() = default;
        AnimationNode(const aiNodeAnim *animNode);

        void CalculateTransform(float timeInTicks);

        Vec3Key translationKeys;
        QuatKey rotationKeys;
        Vec3Key scaleKeys;

        // node's local transform
        glm::mat4 localTransform;

        glm::vec3 translation;
        glm::vec3 scale;
        glm::quat rotation;
        glm::mat4 parentTransform;

    };

    class SkeletalAnimation
    {
    public:
        SkeletalAnimation() = default;
        SkeletalAnimation(aiAnimation *anim);

        void Update(float deltaTime, float speed = 1.0f);

        const std::string &GetName() const { return m_Name; }
        const float GetDuration() const { return m_Duration; }
        const float GetTicksPerSecond() { return m_TicksPerSecond; }
        const float GetTimeInSeconds() { return m_TimeInSeconds; }
        const float GetTimeInTicks() { return m_TimeInTicks; }

    private:

        void ReadChannels();

        std::string m_Name;
        aiAnimation *m_Anim = nullptr;

        float m_Duration = 0;
        float m_TicksPerSecond = 1.0f;
        float m_TimeInSeconds = 0.0f;
        float m_TimeInTicks = 0.0f;

        std::unordered_map<std::string, AnimationNode> m_Channels;
    };
}