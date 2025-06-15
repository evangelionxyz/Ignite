#pragma once

#include "event.hpp"

#include <vector>
#include <sstream>
#include <filesystem>

namespace ignite
{
    class WindowResizeEvent final : public Event
    {
    public:
        WindowResizeEvent(unsigned int width, unsigned int height)
            : m_Width(width), m_Height(height) {}
        u32 GetWidth() const { return m_Width; }
        u32 GetHeight() const { return m_Height; }
        [[nodiscard]] std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);

    private:
        unsigned int m_Width, m_Height;
    };

    class WindowCloseEvent final : public Event
    {
    public:
        WindowCloseEvent() = default;
        [[nodiscard]] std::string ToString() const override
        {
            return "WindowCloseEvent: Window Closed!";
        }

        EVENT_CLASS_TYPE(WindowClose);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    };

    class FramebufferResizeEvent final : public Event
    {
    public:
        FramebufferResizeEvent(int width, int height)
            : m_Width(width), m_Height(height) {}
        [[nodiscard]] i32 GetWidth() const { return m_Width; }
        [[nodiscard]] i32 GetHeight() const { return m_Height; }
        [[nodiscard]] std::string ToString() const override
        {
            std::stringstream ss;
            ss << "FramebufferResizeEvent: " << m_Width << ", " << m_Height;
            return ss.str();
        }
        EVENT_CLASS_TYPE(FramebufferResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    private:
        int m_Width, m_Height;
    };

    class WindowDropEvent final : public Event
    {
    public:
        explicit WindowDropEvent(const std::vector<std::filesystem::path> &paths)
            : m_Paths(paths) {}
        explicit WindowDropEvent(std::vector <std::filesystem::path> &&paths)
            : m_Paths(std::move(paths)) {}

        [[nodiscard]] const std::vector<std::filesystem::path> &GetPaths() const { return m_Paths; }

        EVENT_CLASS_TYPE(WindowDrop);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    private:
        std::vector<std::filesystem::path> m_Paths;
    };

    class WindowMaximizedEvent final : public Event
    {
    public:
        explicit WindowMaximizedEvent(bool maximized)
            : m_Maximized(maximized) {}

        [[nodiscard]] std::string ToString() const override
        {
            return "WindowMaximizedEvent: " + m_Maximized ? "True" : "False";
        }

        [[nodiscard]] bool IsMaximized() const { return m_Maximized; }

        EVENT_CLASS_TYPE(WindowMaximized);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);

    private:
        bool m_Maximized = false;
    };

    class WindowMinimizedEvent final : public Event
    {
    public:
        explicit WindowMinimizedEvent(bool minimized)
            : m_Minimized(minimized) {}

        [[nodiscard]] std::string ToString() const override
        {
            return "WindowMinimizedEvent: " + m_Minimized ? "True" : "False";
        }

        [[nodiscard]] bool IsMinimized() const { return m_Minimized; }

        EVENT_CLASS_TYPE(WindowMinimized);
        EVENT_CLASS_CATEGORY(EventCategoryApplication);
    private:
        bool m_Minimized = false;
    };
}