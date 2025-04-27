#pragma once

#include "base.hpp"

#include <ignite/core/types.hpp>

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

namespace ignite
{
    class IGNITE_API IScript
    {
    public:
        virtual ~IScript() = default;
        virtual void Update(f32 deltaTime);

        static void RegisterScript(const std::string &name, std::function<IScript *()> creator);
        static IScript *CreateScript(const std::string &name);
        static std::vector<std::string> GetAvailableScripts();

    private:
        static std::unordered_map<std::string, std::function<IScript *()>> &GetRegistry();
    };
}

#define REGISTER_SCRIPT(ClassName)                                        \
static bool _##ClassName##_registered = []() {                            \
    ignite::IScript::RegisterScript(#ClassName, []() { return new ClassName(); });\
    return true;                                                          \
}();
