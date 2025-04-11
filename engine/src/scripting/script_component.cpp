#include "iscript.hpp"
#include "script_component.hpp"

#include <ignite/core/logger.hpp>

namespace ignite
{
    void ScriptComponent::AddScript(const std::string &scriptName)
    {
        IScript *script = IScript::CreateScript(scriptName);
        LOG_ASSERT(script, "Failed to add script: {}", scriptName);

        _scripts.emplace_back(script);
    }

    void ScriptComponent::Update(f32 deltaTime)
    {
        for (IScript *script : _scripts)
            script->Update(deltaTime);
    }
}