#include "input.hpp"

#include <GLFW/glfw3.h>
#include "ignite/core/types.hpp"

struct InputData
{
    Input *input = nullptr;
    GLFWwindow *window;
};

static InputData s_Data;

Input::Input(void *window)
{
    s_Data.input = this;
    s_Data.window = static_cast<GLFWwindow *>(window);
}

bool Input::IsKeyPressed(KeyCode keycode)
{
    return glfwGetKey(s_Data.window, keycode) == GLFW_PRESS;
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    return glfwGetMouseButton(s_Data.window, button) == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition()
{
    f64 x, y;
    glfwGetCursorPos(s_Data.window, &x, &y);
    return glm::vec2(x, y);
}
