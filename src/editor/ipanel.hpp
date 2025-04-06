#pragma once

#include <imgui.h>
#include <string>

class IPanel
{
public:
    IPanel() = default;
    IPanel(const char *windowTitle)
        : m_WindowTitle(windowTitle)
    {
    }

    virtual bool IsOpen() { return m_IsOpen; }
    virtual bool IsFocused() { return m_IsFocused; }
    virtual bool IsHovered() { return m_IsHovered; }

    virtual void RenderGui() { }
    virtual void Destroy() { }

    std::string &GetTitle() { return m_WindowTitle; }

protected:
    std::string m_WindowTitle;
    bool m_IsOpen = false;
    bool m_IsFocused = false;
    bool m_IsHovered = false;
};
