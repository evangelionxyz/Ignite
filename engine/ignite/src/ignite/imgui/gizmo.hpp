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
        Gizmo(const GizmoInfo &info);

        void SetOperation(ImGuizmo::OPERATION op);
        void SetMode(ImGuizmo::MODE mode);

        void Manipulate(glm::mat4 &inOutMatrix);

        bool IsManipulating() const;
        bool IsHovered() const;

    private:
        GizmoInfo m_GizmoInfo;
        ImGuizmo::MODE m_Mode = ImGuizmo::MODE::LOCAL;
        ImGuizmo::OPERATION m_Operation = ImGuizmo::OPERATION::NONE;
    };
}