#include "frustum.hpp"

namespace ignite {

    Frustum::Frustum(const glm::mat4 &view_projection)
    {
        Update(view_projection);
    }

    void Frustum::Update(const glm::mat4 &view_projection)
    {
        // Extract frustum planes from the view-projection matrix
        for (int i = 0; i < 4; ++i)
        {
            m_Planes[(int)Plane::Left][i] = view_projection[i][3] + view_projection[i][0];
            m_Planes[(int)Plane::Right][i] = view_projection[i][3] - view_projection[i][0];
            m_Planes[(int)Plane::Bottom][i] = view_projection[i][3] + view_projection[i][1];
            m_Planes[(int)Plane::Top][i] = view_projection[i][3] - view_projection[i][1];
            m_Planes[(int)Plane::Near][i] = view_projection[i][3] + view_projection[i][2];
            m_Planes[(int)Plane::Far][i] = view_projection[i][3] - view_projection[i][2];
        }

        // Normalize the planes
        for (auto &plane : m_Planes)
        {
            float length = glm::length(glm::vec3(plane));
            plane /= length;
        }

        m_ViewProjectionInverse = glm::inverse(view_projection);
        const glm::vec4 corners[8] = {
            {-1, -1, -1, 1}, {1, -1, -1, 1}, {1, 1, -1, 1}, {-1, 1, -1, 1},
            {-1, -1,  1, 1}, {1, -1,  1, 1}, {1, 1,  1, 1}, {-1, 1,  1, 1}
        };

        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 worldSpace = m_ViewProjectionInverse * corners[i];
            m_Corners[i] = glm::vec3(worldSpace) / worldSpace.w;
        }

    }

    bool Frustum::IsPointVisible(const glm::vec3 &point) const
    {
        for (const auto &plane : m_Planes)
        {
            if (glm::dot(glm::vec3(plane), point) + plane.w < 0)
            {
                return false;
            }
        }
        return true;
    }

    bool Frustum::IsSphereVisible(const glm::vec3 &center, float radius) const
    {
        for (const auto &plane : m_Planes)
        {
            if (glm::dot(glm::vec3(plane), center) + plane.w < -radius)
            {
                return false;
            }
        }

        return true;
    }

    bool Frustum::IsAABBVisible(const glm::vec3 &min, const glm::vec3 &max) const
    {
        for (const auto &plane : m_Planes)
        {
            glm::vec3 p = min;
            if (plane.x >= 0) p.x = max.x;
            if (plane.y >= 0) p.y = max.y;
            if (plane.z >= 0) p.z = max.z;

            if (glm::dot(glm::vec3(plane), p) + plane.w < 0)
            {
                return false;
            }
        }
        return true;
    }

    std::vector<std::pair<glm::vec3, glm::vec3>> Frustum::GetEdges() const
    {
        return
        {
            {m_Corners[0], m_Corners[1]}, {m_Corners[1], m_Corners[2]}, {m_Corners[2], m_Corners[3]}, {m_Corners[3], m_Corners[0]},
            {m_Corners[4], m_Corners[5]}, {m_Corners[5], m_Corners[6]}, {m_Corners[6], m_Corners[7]}, {m_Corners[7], m_Corners[4]},
            {m_Corners[0], m_Corners[4]}, {m_Corners[1], m_Corners[5]}, {m_Corners[2], m_Corners[6]}, {m_Corners[3], m_Corners[7]}
        };
    }
}