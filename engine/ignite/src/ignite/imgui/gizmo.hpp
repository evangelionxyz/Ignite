#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

#include "ignite/scene/camera.hpp"

#include "ignite/math/math.hpp"

namespace ignite {
    
    struct GizmoInfo
    {
        glm::mat4 cameraView;
        glm::mat4 cameraProjection;
        Camera::Type cameraType;

        Rect viewRect;
        float snapValue = 0.25f;
        bool isSnapping = false;
    };

    class Gizmo
    {
    public:
        Gizmo() = default;

        void SetInfo(const GizmoInfo &info);

        void SetOperation(ImGuizmo::OPERATION op);
        void SetMode(ImGuizmo::MODE mode);

        void Manipulate(glm::mat4 &inOutMatrix);
        void DrawGrid(float gridSize = 10.0f, const glm::mat4 &gridMatrix = glm::mat4(1.0f));

        bool IsManipulating() const;
        bool IsHovered() const;

    private:
        GizmoInfo m_GizmoInfo;
        ImGuizmo::MODE m_Mode = ImGuizmo::MODE::LOCAL;
        ImGuizmo::OPERATION m_Operation = ImGuizmo::OPERATION::NONE;
    };
}