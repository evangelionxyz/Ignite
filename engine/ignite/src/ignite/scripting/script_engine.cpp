#include "script_engine.hpp"
#include "script_glue.hpp"
#include "script_class.hpp"

#include "ignite/scene/component.hpp"
#include "ignite/project/project.hpp"
#include "ignite/core/application.hpp"
#include "ignite/core/string_utils.hpp"
#include "ignite/core/platform_utils.hpp"

#include "FileWatch.hpp"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/debug-helpers.h>

#include <cstdlib>
#include <format>

namespace ignite
{
    static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
    {
        {"System.Boolean", ScriptFieldType::Bool},
        {"System.Single",  ScriptFieldType::Float},
        {"System.Char",    ScriptFieldType::Char},
        {"System.Byte",    ScriptFieldType::Byte},
        {"System.Double",  ScriptFieldType::Double},
        {"System.Int16",   ScriptFieldType::Short},
        {"System.Int32",   ScriptFieldType::Int},
        {"System.Int64",   ScriptFieldType::Long},
        {"System.UInt16",  ScriptFieldType::UShort},
        {"System.UInt32",  ScriptFieldType::UInt},
        {"System.UInt64",  ScriptFieldType::ULong},
        {"System.UInt",    ScriptFieldType::UByte},
        {"Ignite.Vector2", ScriptFieldType::Vector2},
        {"Ignite.Vector3", ScriptFieldType::Vector3},
        {"Ignite.Vector4", ScriptFieldType::Vector4},
        {"Ignite.Entity",  ScriptFieldType::Entity},
    };

    namespace Utils
    {
        static char *ReadBytes(const std::filesystem::path &filepath, uint32_t *out_size)
        {
            std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

            // failed to open file
            if (!stream)
                return nullptr;

            const size_t end = stream.tellg();
            stream.seekg(0, std::ios::beg);

            const u32 size = static_cast<u32>(end - stream.tellg());

            // file is empty
            if (!size)
            {
                return nullptr;
            }

            char *buffer = new char[size];
            stream.read(static_cast<std::istream::char_type *>(reinterpret_cast<void *>(buffer)), size);

            *out_size = size;
            return buffer;
        }

        static MonoAssembly *LoadMonoAssembly(const std::filesystem::path &filepath)
        {
            uint32_t fileSize = 0;
            char *file_data = ReadBytes(filepath, &fileSize);

            MonoImageOpenStatus status;
            MonoImage *image = mono_image_open_from_data_full(file_data, fileSize, 1, &status, 0);

            if (status != MONO_IMAGE_OK)
            {
                const char *error_message = mono_image_strerror(status);
                LOG_ERROR("{}", error_message);
                return nullptr;
            }

            const std::string &assembly_path = filepath.generic_string();
            MonoAssembly *assembly = mono_assembly_load_from_full(image, assembly_path.c_str(), &status, 0);
            mono_image_close(image);

            // don't forget to free the file data
            delete[] file_data;

            return assembly;
        }

        void PrintAssemblyTypes(MonoAssembly *assembly)
        {
            MonoImage *image = mono_assembly_get_image(assembly);
            const MonoTableInfo *type_definition_table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
            const i32 num_types = mono_table_info_get_rows(type_definition_table);

            for (i32 i = 0; i < num_types; i++)
            {
                u32 cols[MONO_TYPEDEF_SIZE];
                mono_metadata_decode_row(type_definition_table, i, cols, MONO_TYPEDEF_SIZE);

                const char *nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
                const char *name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

                LOG_WARN("{0}.{1}", nameSpace, name);
            }
        }

        ScriptFieldType MonoTypeToScriptFieldType(MonoType *mono_type)
        {
            std::string type_name = mono_type_get_name(mono_type);

            const auto it = s_ScriptFieldTypeMap.find(type_name);
            if (it == s_ScriptFieldTypeMap.end())
            {
                LOG_ERROR("Unkown Field Type : {}", type_name);
                return ScriptFieldType::Invalid;
            }
            return it->second;
        }
    }

    struct ScriptEngineData
    {
        MonoDomain *rootDomain = nullptr;
        MonoDomain *appDomain = nullptr;

        MonoAssembly *coreAssembly = nullptr;
        MonoImage *coreAssemblyImage = nullptr;

        MonoAssembly *appAssembly = nullptr;
        MonoImage *appAssemblyImage = nullptr;

        ScriptClass entityClass;
        std::vector<std::string> entityScriptStorage;

        std::filesystem::path coreAssemblyFilepath;
        std::filesystem::path appAssemblyFilepath;

        std::unique_ptr<filewatch::FileWatch<std::string>> appAssemblyFilewatcher;
        bool assemblyReloadingPending = false;

        Scene *scene = nullptr;
        std::unordered_map<std::string, std::shared_ptr<ScriptClass>> entityClasses;
        std::unordered_map<UUID, std::shared_ptr<ScriptInstance>> entityInstances;
        std::unordered_map<UUID, ScriptFieldMap> entityScriptFields;
    };

    ScriptEngineData *scriptEngineData = nullptr;

    void ScriptEngine::InitMono()
    {
#ifdef _WIN32
        mono_set_assemblies_path("lib");
#elif __linux__
        mono_set_assemblies_path("/usr/lib");
        mono_set_dirs("/usr/lib", "/etc/mono");
        setenv("LD_LIBRARY_PATH", "/usr/lib", 1);
#endif
        MonoDomain *root_domain = mono_jit_init("IgniteJITRuntime");
        LOG_ASSERT(root_domain, "[Script Engine] Mono Domain is NULL!");

        scriptEngineData->rootDomain = root_domain;
        LOG_WARN("[Script Engine] MONO Initialized");

        const char *mono_version = mono_get_runtime_build_info();
        LOG_WARN("[Script Engine] MONO Version: {}", mono_version);
    }

    void ScriptEngine::ShutdownMono()
    {
        if (!scriptEngineData)
        {
            return;
        }

        mono_domain_set(mono_get_root_domain(), false);

        mono_domain_unload(scriptEngineData->appDomain);
        scriptEngineData->appDomain = nullptr;
        scriptEngineData->coreAssembly = nullptr;

        mono_jit_cleanup(scriptEngineData->rootDomain);
        scriptEngineData->rootDomain = nullptr;

        LOG_WARN("[Script Engine] Mono Shutdown");
    }

    void ScriptEngine::Init()
    {
        const auto appAssemblyPath = Project::GetActiveDirectory() / Project::GetInstance()->GetInfo().scriptModulePath;

        if (scriptEngineData)
        {
            scriptEngineData->appAssemblyFilepath = appAssemblyPath.generic_string();
            ReloadAssembly();
            return;
        }

        scriptEngineData = new ScriptEngineData();

        InitMono();
        ScriptGlue::RegisterFunctions();

        // Script Core Assembly
        LOG_ASSERT(std::filesystem::exists("ScriptCore.dll"), "[Script Engine] Script core assembly not found!");
        LoadAssembly("ScriptCore.dll");
        LoadAppAssembly(appAssemblyPath);
        LoadAssemblyClasses();

        // storing classes name into storage
        for (auto &it : scriptEngineData->entityClasses)
        {
            LOG_INFO("Script '{}' loaded", it.first);
            scriptEngineData->entityScriptStorage.emplace_back(it.first);
        }

        scriptEngineData->entityClass = ScriptClass("Ignite", "Entity", true);
        ScriptGlue::RegisterComponents();

        LOG_WARN("[Script Engine] Initialized");
    }

    void ScriptEngine::Shutdown()
    {
        ShutdownMono();

        if (!scriptEngineData)
        {
            return;
        }

        scriptEngineData->entityClasses.clear();
        scriptEngineData->entityInstances.clear();

        delete scriptEngineData;

        LOG_WARN("[Script Engine] Shutdown");
    }

    bool ScriptEngine::LoadAssembly(const std::filesystem::path &filepath)
    {
        char *domain_name = new char[20];
        strcpy(domain_name, "IgniteScriptRuntime");
        scriptEngineData->appDomain = mono_domain_create_appdomain(domain_name, nullptr);
        delete[] domain_name;

        mono_domain_set(scriptEngineData->appDomain, true);

        scriptEngineData->coreAssemblyFilepath = filepath;

        scriptEngineData->coreAssembly = Utils::LoadMonoAssembly(filepath);
        if (scriptEngineData->coreAssembly == nullptr)
        {
            return false;
        }

        scriptEngineData->coreAssemblyImage = mono_assembly_get_image(scriptEngineData->coreAssembly);
        return true;
    }

    static void OnAppAssemblyFileSystemEvent(const std::string &path, const filewatch::Event eventType)
    {
        if (!scriptEngineData->assemblyReloadingPending && eventType == filewatch::Event::modified)
        {
            scriptEngineData->assemblyReloadingPending = true;

            Application::SubmitToMainThread([&]()
            {
                if (scriptEngineData->scene && scriptEngineData->scene->IsPlaying())
                    return false;
                
                scriptEngineData->appAssemblyFilewatcher.reset();
                ScriptEngine::ReloadAssembly();

                return true;
            });
        }
    }

    bool ScriptEngine::LoadAppAssembly(const std::filesystem::path &filepath)
    {
        if (!exists(filepath))
        {
            std::filesystem::path buildScriptPath = Project::GetInstance()->GetActiveDirectory() / "build.bat";
            std::string command = std::format("{}", buildScriptPath.generic_string());
            std::system(command.c_str());

            LOG_ASSERT(exists(filepath), "[Script Engine] Failed to generate project file");
        }

        scriptEngineData->appAssemblyFilepath = filepath;
        scriptEngineData->appAssembly = Utils::LoadMonoAssembly(filepath);
        if (!scriptEngineData->appAssembly)
        {
            LOG_ASSERT(false, "[Script Engine] App Assembly is empty {}", filepath.generic_string());
            return false;
        }

        scriptEngineData->appAssemblyImage = mono_assembly_get_image(scriptEngineData->appAssembly);
        scriptEngineData->appAssemblyFilewatcher = std::make_unique<filewatch::FileWatch<std::string>>(filepath.string(), OnAppAssemblyFileSystemEvent);
        scriptEngineData->assemblyReloadingPending = false;

        return true;
    }

    void ScriptEngine::ReloadAssembly()
    {
        mono_domain_set(mono_get_root_domain(), false);

        mono_domain_unload(scriptEngineData->appDomain);
        LoadAssembly(scriptEngineData->coreAssemblyFilepath);

        if (LoadAppAssembly(scriptEngineData->appAssemblyFilepath))
        {
            LoadAssemblyClasses();

            // storing classes name into storage
            scriptEngineData->entityScriptStorage.clear();

            for (auto &it : scriptEngineData->entityClasses)
            {
                scriptEngineData->entityScriptStorage.emplace_back(it.first);
            }

            // retrieve entity class
            scriptEngineData->entityClass = ScriptClass("Ignite", "Entity", true);
            ScriptGlue::RegisterComponents();

            LOG_WARN("[Script Engine] Assembly Reloaded");
        }
    }

    void ScriptEngine::SetSceneContext(Scene *scene)
    {
        scriptEngineData->scene = scene;
    }

    void ScriptEngine::ClearSceneContext()
    {
        scriptEngineData->scene = nullptr;
        scriptEngineData->entityInstances.clear();
    }

    bool ScriptEngine::EntityClassExists(const std::string &fullClassName)
    {
        if (scriptEngineData)
            return scriptEngineData->entityClasses.contains(fullClassName);
        return false;
    }

    void ScriptEngine::OnCreateEntity(Entity entity)
    {
        if (const auto &sc = entity.GetComponent<Script>(); EntityClassExists(sc.className))
        {
            const UUID uuid = entity.GetUUID();

            const auto instance = std::make_shared<ScriptInstance>(scriptEngineData->entityClasses[sc.className], entity);
            scriptEngineData->entityInstances[uuid] = instance;

            // Copy Fields Value from Editor to Runtime
            if (scriptEngineData->entityScriptFields.contains(uuid))
            {
                ScriptFieldMap &fieldMap = scriptEngineData->entityScriptFields.at(uuid);
                const auto classFields = instance->GetScriptClass()->GetFields();

                if (fieldMap.size() != classFields.size())
                {
                    // find any private members
                    std::vector<std::string> keysToRemove;
                    for (const auto &[fieldName, field] : fieldMap)
                    {
                        if (!classFields.contains(fieldName))
                        {
                            keysToRemove.push_back(fieldName);
                        }
                    }

                    // remove if they are there
                    for (const auto &key : keysToRemove)
                    {
                        fieldMap.erase(key);
                    }
                }

                for (auto &[name, fieldInstance] : fieldMap)
                {
                    // Check invalid type
                    if (fieldInstance.Field.Type == ScriptFieldType::Invalid)
                    {
                        const ScriptFieldType type = instance->GetScriptClass()->GetFields()[name].Type;
                        fieldInstance.Field.Type = type;
                        LOG_WARN("[Script Engine] Checking invalid type {}", name);
                    }

                    switch (fieldInstance.Field.Type)
                    {
                    case ScriptFieldType::Entity:
                    {
                        uint64_t uuid = *reinterpret_cast<uint64_t *>(fieldInstance.m_Buffer);
                        if (uuid == 0)
                        {
                            LOG_ERROR("[Script Engine] Field '{}' (Entity class) is not assigned yet", name);
                            continue;
                        }

                        MonoMethod *ctorMethod = scriptEngineData->entityClass.GetMethod(".ctor", 1);
                        if (!ctorMethod)
                        {
                            LOG_ERROR("[Script Engine] Failed to find constructor {}", name);
                            continue;
                        }

                        // Create new instance
                        MonoObject *entityInstance = ScriptEngine::InstantiateObject(scriptEngineData->entityClass.m_MonoClass);
                        if (!entityInstance)
                        {
                            LOG_ERROR("[Script Engine] Failed to create Entity instance. {}", name);
                            continue;
                        }

                        // Set the entity ID
                        void *param = &uuid;
                        mono_runtime_invoke(ctorMethod, entityInstance, &param, nullptr);

                        // set the new instance into the app class's field
                        instance->SetFieldValueInternal(name, entityInstance);
                        break;
                    }
                    case ScriptFieldType::Invalid:
                        LOG_ASSERT(false, "[Script Engine] Null Object Field {}", name);
                        return;
                    default:
                        instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
                        break;
                    }
                }
            }

            // C# On Create Function
            instance->InvokeOnCreate();
        }
    }

    void ScriptEngine::OnUpdateEntity(Entity entity, float time)
    {
        const UUID uuid = entity.GetUUID();
        if (const auto &it = scriptEngineData->entityInstances.find(uuid); it == scriptEngineData->entityInstances.end())
        {
            LOG_ERROR("[Script Engine] Entity script instance is not attached! {} {}", entity.GetName(), static_cast<uint64_t>(uuid));
            return;
        }

        const auto instance = scriptEngineData->entityInstances.at(uuid);
        instance->InvokeOnUpdate(time);
    }

    MonoString *ScriptEngine::CreateString(const char *string)
    {
        return mono_string_new(scriptEngineData->appDomain, string);
    }

    ScriptClass *ScriptEngine::GetEntityClass()
    {
        return &scriptEngineData->entityClass;
    }

    std::shared_ptr<ScriptClass> ScriptEngine::GetEntityClassesByName(const std::string &name)
    {
        if (!scriptEngineData)
            return nullptr;

        if (!scriptEngineData->entityClasses.contains(name))
            return nullptr;

        return scriptEngineData->entityClasses.at(name);
    }

    std::unordered_map<std::string, std::shared_ptr<ScriptClass>> ScriptEngine::GetEntityClasses()
    {
        return scriptEngineData->entityClasses;
    }

    ScriptFieldMap &ScriptEngine::GetScriptFieldMap(Entity entity)
    {
        LOG_ASSERT(entity.IsValid(), "[Script Engine] Failed to get entity");

        const UUID uuid = entity.GetUUID();
        return scriptEngineData->entityScriptFields[uuid];
    }

    std::vector<std::string> ScriptEngine::GetScriptClassStorage()
    {
        return scriptEngineData->entityScriptStorage;
    }

    std::shared_ptr<ScriptInstance> ScriptEngine::GetEntityScriptInstance(UUID uuid)
    {
        const auto &it = scriptEngineData->entityInstances.find(uuid);
        if (it == scriptEngineData->entityInstances.end())
        {
            LOG_ERROR("[Script Engine] Failed to find {}", static_cast<uint64_t>(uuid));
            return nullptr;
        }

        return it->second;
    }

    Scene *ScriptEngine::GetSceneContext()
    {
        return scriptEngineData->scene;
    }

    MonoImage *ScriptEngine::GetCoreAssemblyImage()
    {
        return scriptEngineData->coreAssemblyImage;
    }

    MonoImage *ScriptEngine::GetAppAssemblyImage()
    {
        return scriptEngineData->appAssemblyImage;
    }

    MonoObject *ScriptEngine::GetManagedInstance(UUID uuid)
    {
        if (!scriptEngineData->entityInstances.contains(uuid))
        {
            LOG_ASSERT(false, "[Script Engine] Invalid Script Instance {}", static_cast<uint64_t>(uuid));
        }

        return scriptEngineData->entityInstances.at(uuid)->GetMonoObject();
    }

    MonoObject *ScriptEngine::InstantiateObject(MonoClass *monoClass)
    {
        MonoObject *instance = mono_object_new(scriptEngineData->appDomain, monoClass);
        if (!instance)
        {
            return nullptr;
        }

        mono_runtime_object_init(instance);
        return instance;
    }

    void ScriptEngine::LoadAssemblyClasses()
    {
        scriptEngineData->entityClasses.clear();

        const MonoTableInfo *typeDefinitionTable = mono_image_get_table_info(scriptEngineData->appAssemblyImage,
            MONO_TABLE_TYPEDEF);

        const i32 num_types = mono_table_info_get_rows(typeDefinitionTable);
        MonoClass *entity_class = mono_class_from_name(scriptEngineData->coreAssemblyImage, "Ignite", "Entity");

        for (i32 i = 0; i < num_types; i++)
        {
            u32 cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(typeDefinitionTable, i, cols, MONO_TYPEDEF_SIZE);

            const char *nameSpace = mono_metadata_string_heap(scriptEngineData->appAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
            const char *className = mono_metadata_string_heap(scriptEngineData->appAssemblyImage, cols[MONO_TYPEDEF_NAME]);

            std::string fullName = className;

            if (strlen(nameSpace) != 0)
            {
                fullName = std::format("{}.{}", nameSpace, className);
            }

            MonoClass *monoClass = mono_class_from_name(scriptEngineData->appAssemblyImage, nameSpace, className);
            if (monoClass == entity_class)
            {
                continue;
            }

            if (const bool is_entity_master_class = mono_class_is_subclass_of(monoClass,
                entity_class, false); !is_entity_master_class)
            {
                continue;
            }

            const auto script_class = std::make_shared<ScriptClass>(nameSpace, className);
            scriptEngineData->entityClasses[fullName] = script_class;

            void *iterator = nullptr;

            while (MonoClassField *field = mono_class_get_fields(monoClass, &iterator))
            {
                const char *fieldName = mono_field_get_name(field);
                if (const uint32_t flags = mono_field_get_flags(field); flags & FIELD_ATTRIBUTE_PUBLIC)
                {
                    MonoType *type = mono_field_get_type(field);
                    const ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
                    script_class->m_Fields[fieldName] = { fieldType, fieldName, field };
                }
            }
        }

        auto &entityClasses = scriptEngineData->entityClasses;
    }
}