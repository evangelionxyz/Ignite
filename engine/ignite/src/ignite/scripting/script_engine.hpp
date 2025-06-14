#pragma once

#include "ignite/scene/scene.hpp"
#include "ignite/scene/entity.hpp"
#include "script_instance.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

extern "C"
{
    typedef struct _MonoClass MonoClass;
    typedef struct _MonoMethod MonoMethod;
    typedef struct _MonoObject MonoObject;
    typedef struct _MonoAssembly MonoAssembly;
    typedef struct _MonoImage MonoImage;
    typedef struct _MonoClassField MonoClassField;
    typedef struct _MonoString MonoString;
}

namespace ignite
{
    using ScriptFieldMap = std::unordered_map<std::string, ScriptFieldInstance>;
    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();

        static bool LoadAssembly(const std::filesystem::path &filepath);
        static bool LoadAppAssembly(const std::filesystem::path &filepath);
        static void ReloadAssembly();
        static void SetSceneContext(Scene *scene);
        static void ClearSceneContext();

        static bool EntityClassExists(const std::string &fullClassName);

        static void OnCreateEntity(Entity entity);
        static void OnUpdateEntity(Entity entity, float time);

        static MonoString *CreateString(const char *string);

        static ScriptClass *GetEntityClass();
        static std::shared_ptr<ScriptClass> GetEntityClassesByName(const std::string &name);
        static std::unordered_map<std::string, std::shared_ptr<ScriptClass>> GetEntityClasses();
        static ScriptFieldMap &GetScriptFieldMap(Entity entity);

        static std::shared_ptr<ScriptInstance> GetEntityScriptInstance(UUID uuid);
        static std::vector<std::string> GetScriptClassStorage();
        static Scene *GetSceneContext();
        static MonoImage *GetCoreAssemblyImage();
        static MonoImage *GetAppAssemblyImage();
        static MonoObject *GetManagedInstance(UUID uuid);

    private:
        static void InitMono();
        static void ShutdownMono();

        static MonoObject *InstantiateObject(MonoClass *monoClass);
        static void LoadAssemblyClasses();

        friend class ScriptClass;
    };

    namespace Utils
    {
        inline const char *ScriptFieldTypeToString(ScriptFieldType type)
        {
            switch (type)
            {
            case ScriptFieldType::Invalid: return "Invalid";
            case ScriptFieldType::Float:   return "Float";
            case ScriptFieldType::Double:  return "Double";
            case ScriptFieldType::Bool:    return "Boolean";
            case ScriptFieldType::Char:    return "Char";
            case ScriptFieldType::Byte:    return "Byte";
            case ScriptFieldType::Short:   return "Short";
            case ScriptFieldType::Int:     return "Int";
            case ScriptFieldType::Long:    return "Long";
            case ScriptFieldType::UByte:   return "UByte";
            case ScriptFieldType::UShort:  return "UShort";
            case ScriptFieldType::UInt:    return "UInt";
            case ScriptFieldType::ULong:   return "ULong";
            case ScriptFieldType::Vector2: return "Vec2";
            case ScriptFieldType::Vector3: return "Vec3";
            case ScriptFieldType::Vector4: return "Vec4";
            case ScriptFieldType::Entity:  return "Entity";
            }

            LOG_ASSERT(false, "Invalid Script Field Type!");
            return "Invalid";
        }

        inline ScriptFieldType ScriptFieldTypeFromString(std::string_view type)
        {
            if (type == "Invalid") return ScriptFieldType::Invalid;
            if (type == "Float")   return ScriptFieldType::Float;
            if (type == "Double")  return ScriptFieldType::Double;
            if (type == "Boolean") return ScriptFieldType::Bool;
            if (type == "Char")    return ScriptFieldType::Char;
            if (type == "Byte")    return ScriptFieldType::Byte;
            if (type == "Short")   return ScriptFieldType::Short;
            if (type == "Int")     return ScriptFieldType::Int;
            if (type == "Long")    return ScriptFieldType::Long;
            if (type == "UByte")   return ScriptFieldType::UByte;
            if (type == "UShort")  return ScriptFieldType::UShort;
            if (type == "UInt")    return ScriptFieldType::UInt;
            if (type == "ULong")   return ScriptFieldType::ULong;
            if (type == "Vec2")    return ScriptFieldType::Vector2;
            if (type == "Vec3")    return ScriptFieldType::Vector3;
            if (type == "Vec4")    return ScriptFieldType::Vector4;
            if (type == "Entity")  return ScriptFieldType::Entity;

            return ScriptFieldType::Invalid;
        }
    }
}
