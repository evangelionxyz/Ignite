#pragma once
#include "key_codes.hpp"
#include "event.hpp"

#include <sstream>

class KeyEvent : public Event
{
public:
    [[nodiscard]] KeyCode GetKeyCode() const { return m_KeyCode; }
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
    explicit KeyEvent(const KeyCode keycode)
        : m_KeyCode(keycode) {}

    KeyCode m_KeyCode;
};

class KeyPressedEvent final : public KeyEvent
{
public:
    KeyPressedEvent(const KeyCode keycode, const uint16_t repeatCount)
        : KeyEvent(keycode), m_RepeatCount(repeatCount) {}

    [[nodiscard]] uint16_t GetRepeatCount() const { return m_RepeatCount; }

    [[nodiscard]] std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)
private:
    uint16_t m_RepeatCount;
};

class KeyReleasedEvent final : public KeyEvent
{
public:
    explicit KeyReleasedEvent(const KeyCode keycode)
        : KeyEvent(keycode) {}

    [[nodiscard]] std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_KeyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent final : public KeyEvent
{
public:
    explicit KeyTypedEvent(const KeyCode keycode)
        : KeyEvent(keycode) {}

    [[nodiscard]] std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyTypedEvent: " << m_KeyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyTyped)
};