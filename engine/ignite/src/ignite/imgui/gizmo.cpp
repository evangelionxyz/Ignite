#include "gizmo.hpp"

#include <ImGuizmo.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ignite {

    void Gizmo::SetInfo(const GizmoInfo &info)
    {
        m_GizmoInfo = info;

        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(
            m_GizmoInfo.viewRect.min.x,
            m_GizmoInfo.viewRect.min.y,
            m_GizmoInfo.viewRect.max.x,
            m_GizmoInfo.viewRect.max.y);

        ImGuizmo::SetOrthographic(m_GizmoInfo.cameraType == Camera::Type::Orthographic);
    }

    void Gizmo::SetOperation(ImGuizmo::OPERATION op)
    {
        m_Operation = op;
    }

    void Gizmo::SetMode(ImGuizmo::MODE mode)
    {
        m_Mode = mode;
    }

    void Gizmo::Manipulate(glm::mat4 &inOutMatrix)
    {
        // X, Y, Z axes
        f32 snapValues[] = { m_GizmoInfo.snapValue, m_GizmoInfo.snapValue, m_GizmoInfo.snapValue };

        ImGuizmo::Manipulate(glm::value_ptr(m_GizmoInfo.cameraView),
            glm::value_ptr(m_GizmoInfo.cameraProjection),
            m_Operation, m_Mode,
            glm::value_ptr(inOutMatrix), 
            nullptr, 
            m_GizmoInfo.isSnapping ? snapValues : nullptr,
            nullptr, nullptr);
    }

    void Gizmo::DrawGrid(float gridSize, const glm::mat4 &gridMatrix)
    {
        ImGuizmo::DrawGrid(
            glm::value_ptr(m_GizmoInfo.cameraView),
            glm::value_ptr(m_GizmoInfo.cameraProjection),
            glm::value_ptr(gridMatrix),
            10.0f
        );
    }

    bool Gizmo::IsManipulating() const
    {
        return ImGuizmo::IsUsing();
    }

    bool Gizmo::IsHovered() const
    {
        return ImGuizmo::IsOver();
    }

}