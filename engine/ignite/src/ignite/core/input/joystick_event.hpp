#pragma once

#include "event.hpp"
#include "joystick_codes.hpp"

#include "ignite/core/types.hpp"

#include <glm/glm.hpp>
#include <spdlog/fmt/fmt.h>
#include <glfw/glfw3.h>
#include <list>

namespace ignite {

    class Window;

    static void ApplyDeadZone(glm::vec2 &v, const float deadZone = 0.1f)
    {
        v *= glm::max(glm::length(v) - deadZone, 0.f) / (1.f - deadZone);
    }

    class Joystick
    {
    public:
        Joystick() = default;

        Joystick(int joyId)
            : m_JoyId(joyId)
        {
        }

        void Update() const
        {
            if (glfwJoystickIsGamepad(m_JoyId))
            {
                const char *name = glfwGetGamepadName(m_JoyId);

                GLFWgamepadstate gamepadState;
                glfwGetGamepadState(m_JoyId, &gamepadState);

                // Set the axes
                float *axisValues = gamepadState.axes;
                m_LeftAxis = glm::vec2(axisValues[joystick::axis::LEFT_X], axisValues[joystick::axis::LEFT_Y]);
                ApplyDeadZone(m_LeftAxis, 0.1f);

                m_RightAxis = glm::vec2(axisValues[joystick::axis::RIGHT_X], axisValues[joystick::axis::RIGHT_Y]);
                ApplyDeadZone(m_RightAxis, 0.1f);

                m_Triggers = glm::vec2(axisValues[joystick::axis::LEFT_TRIGGER], axisValues[joystick::axis::RIGHT_TRIGGER]);

                // Update all of the buttons
                for (int bt = 0; bt < joystick::button::LAST; ++bt)
                {
                    unsigned char buttonState = gamepadState.buttons[bt];
                    m_Buttons[bt] = buttonState == GLFW_PRESS;

                    if (m_Buttons[bt])
                    {
                        m_ButtonStrStream << " button " << bt << " pressed";
                    }
                }
            }
        }

        std::string ToString() const
        {
            std::string msg = fmt::format("[Joystick] Left {} {} Right {} {} L2 {} R2 {} {}\n",
                m_LeftAxis.x, m_LeftAxis.y,
                m_RightAxis.x, m_RightAxis.y,
                m_Triggers.x, m_Triggers.y,
                m_ButtonStrStream.str());
            m_ButtonStrStream.clear();
            return msg;
        }

        int GetID() const { return m_JoyId; }

        bool IsButtonPressed(joystick::JButton bt)
        {
            return m_Buttons[bt];
        }

        const glm::vec2 &GetLeftAxis() const { return m_LeftAxis; }

        const glm::vec2 &GetRightAxis() const { return m_RightAxis; }

        const glm::vec2 &GetTriggerAxis() const { return m_Triggers; }

    private:
        int m_JoyId = -1;
        mutable bool m_Buttons[joystick::button::LAST] = { false };
        mutable glm::vec2 m_LeftAxis, m_RightAxis, m_Triggers;
        mutable std::stringstream m_ButtonStrStream;

    };

    class JoystickManager
    {
    public:
        static void Init(Window *window);
        static void ConnectJoystick(int id);
        static void DisconnectJoystick(int id);

        static const std::list<Ref<Joystick>> &GetConnectedJoystick();
        static const std::list<int> &GetDisconnectedJoystickIDs();

    private:
        static std::list<Ref<Joystick>> connectedJoystick;
        static std::list<int> disconnectedIDs;
        static Window *window;
    };

    class JoystickConnectionEvent : public Event
    {
    public:
        JoystickConnectionEvent(int joyId, joystick::JState state)
            : m_JoyId(joyId), m_State(state)
        {
        }

        std::string ToString() const override
        {
            switch (m_State)
            {
                case joystick::state::CONNECTED:
                {
                    return fmt::format("Joystick {} connected!", m_JoyId);
                }
                case joystick::state::DISCONNECTED:
                {
                    return fmt::format("Joystick {} disconnected!", m_JoyId);
                }
                default:
                {
                    return fmt::format("No Joystick is connected!");
                }
            }
        }

        joystick::JState GetState() const { return m_State; }

        EVENT_CLASS_TYPE(Joystick);
        EVENT_CLASS_CATEGORY(EventCategoryJoystick);

    private:
        int m_JoyId = -1;
        joystick::JState m_State = joystick::state::DISCONNECTED;
    };


}