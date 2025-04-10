#pragma once

#include <glm/vec2.hpp>

#include "key_codes.hpp"
#include "mouse_codes.hpp"

class Input
{
public:
    Input() = default;
    explicit Input(void *window);

    static bool IsKeyPressed(KeyCode keycode);
    static bool IsMouseButtonPressed(MouseCode button);
    static glm::vec2 GetMousePosition();
};
