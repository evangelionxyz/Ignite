#pragma once

#include <glm/glm.hpp>

namespace ignite
{
    struct AABB
    {
        glm::vec3 Min = glm::vec3(0.0f);
        glm::vec3 Max = glm::vec3(0.0f);

        AABB() = default;
        AABB(const AABB &) = default;
        AABB(const glm::vec3 &center, const glm::vec3 &size)
            : Min(center - size * 0.5f), Max(center + size * 0.5f)
        {
        }

        bool RayIntersection(const glm::vec3 &ray_origin, const glm::vec3 &ray_direction)
        {
            glm::vec3 invDir = 1.0f / ray_direction;
            glm::vec3 tMin = (Min - ray_origin) * invDir;
            glm::vec3 tMax = (Max - ray_origin) * invDir;
            glm::vec3 t1 = glm::min(tMin, tMax);
            glm::vec3 t2 = glm::max(tMin, tMax);
            float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
            float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);
            return tNear <= tFar && tFar > 0;
        }
    };
}
