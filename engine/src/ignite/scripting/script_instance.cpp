#include "script_instance.hpp"
#include "script_engine.hpp"
#include "ignite/scene/entity.hpp"
#include "script_class.hpp"

#include <mono/jit/jit.h>

namespace ignite
{
    ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
        : m_ScriptClass(scriptClass)
    {
        m_Instance = scriptClass->Instantiate();

        m_OnConstructor = ScriptEngine::GetEntityClass()->GetMethod(".ctor", 1);
        m_OnCreateMethod = scriptClass->GetMethod("OnCreate");
        m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);

        // Entity Constructor
        {
            UUID entityID = entity.GetUUID();
            void *param = &entityID;
            m_ScriptClass->InvokeMethod(m_Instance, m_OnConstructor, &param);
        }
    }

    void ScriptInstance::InvokeOnCreate()
    {
        if (m_OnCreateMethod)
        {
            m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);
        }
    }

    void ScriptInstance::InvokeOnUpdate(float time)
    {
        if (m_OnUpdateMethod)
        {
            void *param = &time;
            m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
        }
    }

    bool ScriptInstance::GetFieldValueInternal(const std::string &name, void *buffer)
    {
        const auto &fields = m_ScriptClass->GetFields();

        auto it = fields.find(name);
        if (it == fields.end())
        {
            LOG_ERROR("[Script Instance] Failed to Get Internal Value");
            return false;
        }

        const ScriptField &field = it->second;
        if (field.Type == ScriptFieldType::Entity)
        {
            // Get Entity Field from App Class
            MonoObject *fieldValue = nullptr;
            mono_field_get_value(m_Instance, field.ClassField, &fieldValue);
            if (!fieldValue || !fieldValue->vtable)
            {
                LOG_ERROR("[Script Instance] Could not get field '{}' in class", name);
                return false;
            }

            // get the ID from field's class
            MonoClass *klass = mono_object_get_class(fieldValue);
            MonoClassField *idField = mono_class_get_field_from_name(klass, "ID");
            if (!idField)
            {
                LOG_ERROR("[Script Instance] Failed to find field '{}' in Entity class", name);
                return false;
            }

            mono_field_get_value(fieldValue, idField, buffer);
        }
        else
        {
            mono_field_get_value(m_Instance, field.ClassField, buffer);
        }

        return true;
    }

    bool ScriptInstance::SetFieldValueInternal(const std::string &name, const void *value)
    {
        const auto &fields = m_ScriptClass->GetFields();
        auto it = fields.find(name);
        if (it == fields.end())
        {
            LOG_ERROR("[Script Instance] Failed to set field '{}' value", name);
            return false;
        }

        const ScriptField &field = it->second;

        if (field.Type == ScriptFieldType::Entity)
        {
            MonoObject *object = static_cast<MonoObject *>(const_cast<void *>(value));
            mono_field_set_value(m_Instance, field.ClassField, object);
        }
        else
        {
            mono_field_set_value(m_Instance, field.ClassField, (void *)value);
        }

        return true;
    }
}
