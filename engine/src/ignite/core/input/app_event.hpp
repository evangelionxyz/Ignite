#pragma once

#include "event.hpp"

#include <vector>
#include <sstream>
#include <filesystem>

class WindowResizeEvent final : public Event {
private:
    unsigned int m_Width, m_Height;
public:
    WindowResizeEvent(unsigned int width, unsigned int height)
        : m_Width(width), m_Height(height) {}

    inline unsigned int GetWidth() const { return m_Width; }
    inline unsigned int GetHeight() const { return m_Height; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
        return ss.str();
    };

    EVENT_CLASS_TYPE(WindowResize);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

class WindowCloseEvent final : public Event
{
public:
    WindowCloseEvent() {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowCloseEvent: Window Closed!";
        return ss.str();
    };

    EVENT_CLASS_TYPE(WindowClose);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

class FramebufferResizeEvent final : public Event
{
private:
    int m_Width, m_Height;
public:
    FramebufferResizeEvent(int width, int height)
        : m_Width(width), m_Height(height) {}

    inline int GetWidth() const { return m_Width; }
    inline int GetHeight() const { return m_Height; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "FramebufferResizeEvent: " << m_Width << ", " << m_Height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(FramebufferResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

class WindowDropEvent : public Event
{
public:
    WindowDropEvent(const std::vector<std::filesystem::path> &paths)
        : m_Paths(paths) {}
    WindowDropEvent(std::vector <std::filesystem::path> &&paths)
        : m_Paths(std::move(paths)) {}

    const std::vector<std::filesystem::path> &GetPaths() const { return m_Paths; }

    EVENT_CLASS_TYPE(WindowDrop);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);

private:
    std::vector<std::filesystem::path> m_Paths;
};