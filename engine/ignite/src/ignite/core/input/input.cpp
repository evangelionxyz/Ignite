#include "input.hpp"

#include <GLFW/glfw3.h>
#include "ignite/core/types.hpp"

namespace ignite
{
    struct InputData
    {
        Input *input = nullptr;
        GLFWwindow *window;
    };

    static InputData s_r2d;

    Input::Input(void *window)
    {
        s_r2d.input = this;
        s_r2d.window = static_cast<GLFWwindow *>(window);
    }

    bool Input::IsKeyPressed(KeyCode keycode)
    {
        return glfwGetKey(s_r2d.window, keycode) == GLFW_PRESS;
    }

    bool Input::IsMouseButtonPressed(MouseCode button)
    {
        return glfwGetMouseButton(s_r2d.window, button) == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition()
    {
        f64 x, y;
        glfwGetCursorPos(s_r2d.window, &x, &y);
        return glm::vec2(x, y);
    }
}
