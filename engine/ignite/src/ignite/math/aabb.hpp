#pragma once

#include <glm/glm.hpp>

namespace ignite
{
    struct AABB
    {
        glm::vec3 min = glm::vec3(0.0f);
        glm::vec3 max = glm::vec3(0.0f);

        AABB() = default;
        AABB(const AABB &) = default;
        AABB(const glm::vec3 &center, const glm::vec3 &size)
            : min(center - size * 0.5f), max(center + size * 0.5f)
        {
        }

        bool RayIntersection(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection)
        {
            glm::vec3 invDir = 1.0f / rayDirection;
            glm::vec3 tMin = (min - rayOrigin) * invDir;
            glm::vec3 tMax = (max - rayOrigin) * invDir;
            glm::vec3 t1 = glm::min(tMin, tMax);
            glm::vec3 t2 = glm::max(tMin, tMax);
            float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
            float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);
            return tNear <= tFar && tFar > 0;
        }
    };
}
