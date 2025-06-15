#pragma once

#include "ignite/core/uuid.hpp"

#include <unordered_map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace ignite
{
    struct Joint
    {
        std::string name;
        int32_t id; // index in joints array
        int32_t parentJointId; // parent in skeleton hierarchy (-1 for root)
        glm::mat4 inverseBindPose; // inverse bind pose matrix
        glm::mat4 localTransform; // current local transform
        glm::mat4 globalTransform; // current global transform
    };

    struct Skeleton
    {
        std::vector<Joint> joints;
        std::unordered_map<std::string, int32_t> nameToJointMap; // for fast lookup by name
        std::unordered_map<int32_t, UUID> jointEntityMap;
    };
}
