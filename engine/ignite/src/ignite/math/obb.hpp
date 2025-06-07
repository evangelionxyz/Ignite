#pragma once

#include <glm/glm.hpp>
#include <array>

namespace ignite 
{
    struct OBB
    {
        glm::vec3 center;
        glm::vec3 halfExtents;
        glm::mat3 orientation;

        OBB() = default;
        OBB(const OBB &) = default;
        OBB(const glm::vec3 &center, const glm::vec3 &worldScale, const glm::quat &rotation)
            : center(center), halfExtents(worldScale * 0.5f), orientation(glm::mat3_cast(rotation))
        {
        }

        std::array<glm::vec3, 8> GetVertices()
        {
            std::array<glm::vec3, 8> vertices;
            glm::vec3 axes[3] =
            {
                orientation[0] * halfExtents.x,
                orientation[1] * halfExtents.y,
                orientation[2] * halfExtents.z
            };

            vertices[0] = center - axes[0] - axes[1] - axes[2];
            vertices[1] = center + axes[0] - axes[1] - axes[2];
            vertices[2] = center - axes[0] + axes[1] - axes[2];
            vertices[3] = center + axes[0] + axes[1] - axes[2];
            vertices[4] = center - axes[0] - axes[1] + axes[2];
            vertices[5] = center + axes[0] - axes[1] + axes[2];
            vertices[6] = center - axes[0] + axes[1] + axes[2];
            vertices[7] = center + axes[0] + axes[1] + axes[2];

            return vertices;
        }

        bool RayIntersection(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, float &tIntersect)
        {
            glm::vec3 delta = center - rayOrigin;
            float tNear = std::numeric_limits<float>::lowest();  // start with lowest possible value
            float tFar = std::numeric_limits<float>::max();  // start with max possible value

            std::array<glm::vec3, 3> axes = { orientation[0], orientation[1], orientation[2] };

            for (int i = 0; i < axes.size(); ++i)
            {
                float e = glm::dot(axes[i], delta);
                float f = glm::dot(axes[i], rayDirection);

                // ray is not parallel to the slab
                if (fabs(f) > glm::epsilon<float>())
                {
                    float t1 = (e + halfExtents[i]) / f;
                    float t2 = (e - halfExtents[i]) / f;

                    if (t1 > t2) std::swap(t1, t2);  // ensure t1 is the near intersection

                    tNear = t1 > tNear ? t1 : tNear;
                    tFar = t2 < tFar ? t2 : tFar;

                    // check if ray misses the box
                    if (tNear > tFar || tFar < 0.0f) return false;
                }
                else // ray is parallel to the slab
                {
                    if (-e - halfExtents[i] > 0.0f || -e + halfExtents[i] < 0.0f)
                        return false;  // no intersection
                }
            }

            // if tNear > 0, return it, otherwise return tFar
            tIntersect = tNear > 0.0f ? tNear : tFar;
            return true;
        }
    };
}
