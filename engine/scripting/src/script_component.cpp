#include "iscript.hpp"
#include "script_component.hpp"

namespace ignite
{
    void ScriptComponent::AddScript(const std::string &scriptName)
    {
        IScript *script = IScript::CreateScript(scriptName);
        _scripts.emplace_back(script);
    }

    void ScriptComponent::Update(f32 deltaTime)
    {
        for (IScript *script : _scripts)
            script->Update(deltaTime);
    }
}