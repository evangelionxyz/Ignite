#include "script_glue.hpp"
#include "script_engine.hpp"

#include "ignite/scene/component.hpp"
#include "ignite/scene/icomponent.hpp"
#include "ignite/scene/scene.hpp"
#include "ignite/scene/entity.hpp"
#include "ignite/scene/scene_manager.hpp"

#include "ignite/physics/jolt/jolt_physics.hpp"

#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>
#include <box2d/box2d.h>
#include <box2d/types.h>

#include <string>

#ifdef __linux
#include <cxxabi.h>
#endif

namespace ignite
{
#define SCRIPTING_ADD_INTERNAL_CALLS(method) mono_add_internal_call("Ignite.InternalCalls::"#method, reinterpret_cast<const void*>(method))

    namespace Utils
    {
        std::string MonoStringToString(MonoString *string)
        {
            char *cStr = mono_string_to_utf8(string);
            auto str = std::string(cStr);
            mono_free(cStr);

            return str;
        }
    }

    static std::unordered_map<MonoType *, std::function<bool(Entity)>> s_EntityHasComponentFuncs;
    static std::unordered_map<MonoType *, std::function<void(Entity)>> s_EntityAddComponentFuncs;

    // ==============================================
    // Entity

    static bool Entity_HasComponent(UUID entityID, MonoReflectionType *componentType)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            MonoType *managedType = mono_reflection_type_get_type(componentType);
            LOG_ASSERT(s_EntityHasComponentFuncs.contains(managedType), "[ScriptGlue]: Failed to process Entity_HasComponent");
            return s_EntityHasComponentFuncs.at(managedType)(entity);
        }

        return false;
    }

    static void Entity_AddComponent(UUID entityID, MonoReflectionType *componentType)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            MonoType *managedType = mono_reflection_type_get_type(componentType);
            LOG_ASSERT(s_EntityAddComponentFuncs.find(managedType) != s_EntityAddComponentFuncs.end(), "[ScriptGlue]: Failed to process AddComponent");
            s_EntityAddComponentFuncs.at(managedType)(entity);
        }
    }

    static uint64_t Entity_FindEntityByName(MonoString *stringName)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");

        std::string name = Utils::MonoStringToString(stringName);
        Entity entity = SceneManager::GetEntity(scene, name);

        if (!entity.IsValid())
            return 0;

        return entity.GetUUID();
    }

    static void Entity_SetVisibility(UUID entityID, bool value)
    {
        

        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");

        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (!entity.IsValid())
        {
            entity.GetComponent<Transform>().visible = value;
        }
    }

    static void Entity_GetVisibility(UUID entityID, bool *value)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");

        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (!entity.IsValid())
        {
            *value = entity.GetComponent<Transform>().visible;
        }
    }

    static uint64_t Entity_Instantiate(UUID entityID, glm::vec3 translation)
    {        
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");

        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            Entity copyEntity = SceneManager::DuplicateEntity(scene, entity);
            copyEntity.GetComponent<Transform>().translation = translation;

            scene->physics2D->Instantiate(copyEntity);
            scene->physics->InstantiateEntity(copyEntity);
            return copyEntity.GetUUID();
        }

        return 0;
    }

    static void Entity_Destroy(UUID entityID)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");

        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            scene->physics2D->DestroyBody(entity);
            scene->physics->DestroyEntity(entity);
            SceneManager::DestroyEntity(scene, entityID);
        }
    }

    // ==============================================
    // Component
    static MonoObject *GetScriptInstance(UUID entityID)
    {
        return ScriptEngine::GetManagedInstance(entityID);
    }

    static void TransformComponent_GetTranslation(UUID entityID, glm::vec3 *outTranslation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            *outTranslation = entity.GetComponent<Transform>().translation;
        }
    }

    static void TransformComponent_SetTranslation(UUID entityID, glm::vec3 translation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            entity.GetComponent<Transform>().localTranslation = translation;
        }
    }

    static void TransformComponent_GetRotation(UUID entityID, glm::quat *outRotation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (entity.IsValid())
        {
            *outRotation = entity.GetComponent<Transform>().rotation;
        }
    }

    static void TransformComponent_SetRotation(UUID entityID, glm::quat rotation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (entity.IsValid())
        {
            entity.GetComponent<Transform>().rotation = rotation;
        }
    }

    static void TransformComponent_GetEulerAngles(UUID entityID, glm::vec3 *angle)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (entity.IsValid())
        {
            *angle = glm::eulerAngles(entity.GetComponent<Transform>().rotation);
        }
    }

    static void TransformComponent_SetEulerAngles(UUID entityID, glm::vec3 angle)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (entity.IsValid())
        {
            entity.GetComponent<Transform>().rotation = glm::quat(angle);
        }
    }

    static void TransformComponent_GetScale(UUID entityID, glm::vec3 *outScale)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);

        if (entity.IsValid())
        {
            *outScale = entity.GetComponent<Transform>().scale;
        }
    }

    static void TransformComponent_SetScale(UUID entityID, glm::vec3 scale)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        LOG_ASSERT(scene, "[ScriptGlue] Invalid Scene");
        Entity entity = SceneManager::GetEntity(scene, entityID);
        if (entity.IsValid())
        {
            entity.GetComponent<Transform>().scale = scale;
        }
    }

    template <typename... Component>
    static void RegisterComponent()
    {
        ([]()
            {
                std::string_view typeName = typeid(Component).name();
#ifdef OGN_PLATFORM_LINUX
                int status = 0;
                char *demangledName = abi::__cxa_demangle(typeName.data(), nullptr, nullptr, &status);
                if (status == 0 && demangledName != nullptr)
                {
                    typeName = demangledName;
                }
                else
                {
                    LOG_ERROR("Could not demangle type name: {}", typeName);
                    return;
                }
#endif
                size_t pos = typeName.find_last_of(':');
                std::string_view structName = (pos == std::string_view::npos) ? typeName : typeName.substr(pos + 1);

                // Remove 'Component'
                pos = structName.find("Component");
                structName = structName.substr(0, pos);

                std::string managedTypename = std::format("Ignite.{}", structName);
                MonoType *managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
                if (!managedType)
                {
                    LOG_ERROR("[Script Glue] Could not find component type {}", managedTypename);
                    return;
                }
                s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
                s_EntityAddComponentFuncs[managedType] = [](Entity entity) { entity.AddOrReplaceComponent<Component>(); };
#ifdef OGN_PLATFORM_LINUX
                free(demangledName);
#endif

            }(), ...);
    }

    template <typename... Component>
    static void RegisterComponent(ComponentGroup<Component...>)
    {
        RegisterComponent<Component...>();
    }

    void ScriptGlue::RegisterComponents()
    {
        s_EntityHasComponentFuncs.clear();
        RegisterComponent(AllComponents{});
    }

    void ScriptGlue::RegisterFunctions()
    {
        // Entity
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_FindEntityByName);
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_AddComponent);
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_HasComponent);
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_SetVisibility);
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_GetVisibility);
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_Instantiate);
        SCRIPTING_ADD_INTERNAL_CALLS(Entity_Destroy);

        // Components
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_GetTranslation);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_SetTranslation);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_GetRotation);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_SetRotation);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_GetEulerAngles);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_SetEulerAngles);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_GetScale);
        SCRIPTING_ADD_INTERNAL_CALLS(TransformComponent_SetScale);
    }
}
