#pragma once

#include "base.hpp"

#include <string>
#include <vector>

namespace ignite
{
    class IScript;

    class IGNITE_API ScriptComponent
    {
    public:
        ScriptComponent() = default;
        void AddScript(const std::string &scriptName);
        void Update(f32 deltaTime);

    protected:
        std::vector<IScript *> _scripts;
    };
}