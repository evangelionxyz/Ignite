#pragma once

#include "script_field.hpp"

#include <string>
#include <unordered_map>

extern "C"
{
    typedef struct _MonoClass MonoClass;
    typedef struct _MonoMethod MonoMethod;
    typedef struct _MonoObject MonoObject;
}

namespace ignite
{
    class ScriptInstance;

    class ScriptClass
    {
    public:
        ScriptClass() = default;
        ScriptClass(const std::string &classNamespace, const std::string &className, bool core = false);

        MonoObject *Instantiate();
        MonoMethod *GetMethod(const std::string &name, int parameterCount = 0);
        MonoObject *InvokeMethod(MonoObject *instance, MonoMethod *method, void **params = nullptr);
        void HandleException(MonoObject *exception);
        std::unordered_map<std::string, ScriptField> GetFields() const { return m_Fields; }

    private:
        MonoClass *m_MonoClass = nullptr;
        std::string m_ClassName;
        std::string m_ClassNamespace;
        std::unordered_map<std::string, ScriptField> m_Fields;

        friend class ScriptEngine;
    };
}
