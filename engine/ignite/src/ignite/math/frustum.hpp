#pragma once

#include <glm/glm.hpp>
#include <array>

namespace ignite {
    class Frustum
    {
    public:
        enum class Plane
        {
            Left = 0,
            Right,
            Bottom,
            Top,
            Near,
            Far
        };

        Frustum() = default;
        Frustum(const glm::mat4 &view_projection);

        void Update(const glm::mat4 &view_projection);
        bool IsPointVisible(const glm::vec3 &point) const;
        bool IsSphereVisible(const glm::vec3 &center, float radius) const;
        bool IsAABBVisible(const glm::vec3 &min, const glm::vec3 &max) const;
        const std::array<glm::vec3, 8> &GetCorners() const { return m_Corners; }
        const std::array<glm::vec4, 6> &GetPlanes() const { return m_Planes; }
        std::vector<std::pair<glm::vec3, glm::vec3>> GetEdges() const;

    private:
        std::array<glm::vec4, 6> m_Planes;
        std::array<glm::vec3, 8> m_Corners;
        glm::mat4 m_ViewProjectionInverse;
    };
}