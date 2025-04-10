#pragma once

#include <string>
#include "ignite/core/types.hpp"
#include "ignite/core/logger.hpp"

#include <imgui_internal.h>
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

    template<typename T>
    static  Ref<T> Create(const char *windowTitle)
    {
        const bool valid = std::is_base_of<IPanel, T>::value;
        LOG_ASSERT(valid, "Not a class of IPanel ");
        return CreateRef<T>(windowTitle);
    }

protected:
    std::string m_WindowTitle;
    bool m_IsOpen = false;
    bool m_IsFocused = false;
    bool m_IsHovered = false;
};
