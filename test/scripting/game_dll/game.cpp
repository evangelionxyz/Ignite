#include <iostream>

#include <scripting/iscript.hpp>

class SimpleScript : public ignite::IScript
{
public:
    void Update(f32 deltaTime) override
    {
        std::cout << "SimpleScript is updating: " << deltaTime << '\n';
    }
};

class PlayerMovement : public ignite::IScript
{
public:
    void Update(f32 deltaTime) override
    {
        std::cout << "PlayerMovement script is updating: " << deltaTime << '\n';
    }
};

REGISTER_SCRIPT(SimpleScript)
REGISTER_SCRIPT(PlayerMovement)
