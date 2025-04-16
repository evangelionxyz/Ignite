#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include "ignite/core/logger.hpp"
#include "ignite/core/types.hpp"

#include "states.hpp"

namespace ignite
{
    class IPanel
    {
    public:
        virtual ~IPanel() = default;

        IPanel() = default;
        explicit IPanel(const char *windowTitle)
            : m_WindowTitle(windowTitle)
        {
        }

        // from Layer class
        virtual void OnGuiRender() { }

        // to child class
        virtual bool IsOpen() { return m_IsOpen; }
        virtual bool IsFocused() { return m_IsFocused; }
        virtual bool IsHovered() { return m_IsHovered; }
        virtual void OnUpdate(f32 deltaTime) { }

        std::string &GetTitle() { return m_WindowTitle; }

    protected:
        std::string m_WindowTitle;
        bool m_IsOpen = false;
        bool m_IsFocused = false;
        bool m_IsHovered = false;
    };
}


