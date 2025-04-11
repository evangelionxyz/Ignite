#include "iscript.hpp"
#include <ignite/core/logger.hpp>

#include <string>

namespace ignite
{
    void IScript::Update(f32 deltaTime)
    {
    }

    void IScript::RegisterScript(const std::string &name, std::function<IScript *()> creator)
    {
        GetRegistry()[name] = creator;
    }

    IScript *IScript::CreateScript(const std::string &name)
    {
        auto it = GetRegistry().find(name);
        if (it != GetRegistry().end())
        {
            return it->second();
        }

        return nullptr;
    }

    std::vector<std::string> IScript::GetAvailableScripts()
    {
        std::vector<std::string> script_names;
        for (const auto &entry : GetRegistry())
        {
            script_names.push_back(entry.first);
        }
        return script_names;
    }

    std::unordered_map<std::string, std::function<IScript *()>> &IScript::GetRegistry()
    {
        static std::unordered_map<std::string, std::function<IScript *()>> registry;
        return registry;
    }
}