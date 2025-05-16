#include "joystick_event.hpp"

#include "ignite/graphics/window.hpp"

namespace ignite {

    Window *JoystickManager::window = nullptr;

    void JoystickManager::Init(Window *window)
    {
        JoystickManager::window = window;
    }

    void JoystickManager::ConnectJoystick(int id)
    {
        Ref<Joystick> j = CreateRef<Joystick>(id);
        connectedJoystick.push_back(std::move(j));

        JoystickConnectionEvent event(id, joystick::state::CONNECTED);
        window->m_Callback(event);
    }

    void JoystickManager::DisconnectJoystick(int id)
    {
        disconnectedIDs.push_back(id);
        
        JoystickConnectionEvent event(id, joystick::state::DISCONNECTED);
        window->m_Callback(event);
    }

    const std::list<Ref<Joystick>> &JoystickManager::GetConnectedJoystick()
    {
        return connectedJoystick;
    }

    const std::list<int> &JoystickManager::GetDisconnectedJoystickIDs()
    {
        return disconnectedIDs;
    }

    std::list<Ref<Joystick>> JoystickManager::connectedJoystick;
    std::list<int> JoystickManager::disconnectedIDs;

}