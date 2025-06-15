#include "serializer.hpp"
#include "serializer.hpp"

#include "ignite/scripting/script_class.hpp"
#include "ignite/scripting/script_engine.hpp"

#include "ignite/asset/asset_importer.hpp"
#include "ignite/scene/scene.hpp"
#include "ignite/project/project.hpp"
#include "ignite/core/logger.hpp"

#include "ignite/scene/entity.hpp"
#include "ignite/scene/component.hpp"
#include "ignite/scene/scene_manager.hpp"

#include <fstream>

namespace ignite {

    Serializer::Serializer(const std::filesystem::path &filepath)
        : m_Filepath(filepath)
    {
    }

    void Serializer::Serialize() const
    {
        std::ofstream outFile(m_Filepath);
        outFile << m_Emitter.c_str();
        outFile.close();
    }

    void Serializer::Serialize(const std::filesystem::path &filepath)
    {
        m_Filepath = filepath;

        std::ofstream outFile(m_Filepath);
        outFile << m_Emitter.c_str();
        outFile.close();
    }

    void Serializer::BeginMap(const std::string &mapName)
    {
        m_Emitter << YAML::Key << mapName;
        m_Emitter << YAML::BeginMap;
    }

    void Serializer::BeginMap()
    {
        m_Emitter << YAML::BeginMap;
    }

    void Serializer::EndMap()
    {
        m_Emitter << YAML::EndMap;
    }

    void Serializer::BeginSequence()
    {
        m_Emitter << YAML::BeginSeq;
    }

    void Serializer::BeginSequence(const std::string &sequenceName)
    {
        m_Emitter << YAML::Key << sequenceName << YAML::Value << YAML::BeginSeq;
    }

    void Serializer::EndSequence()
    {
        m_Emitter << YAML::EndSeq;
    }

    YAML::Node Serializer::Deserialize(const std::filesystem::path &filepath)
    {
        std::ifstream inFile(filepath);
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();
        return YAML::Load(buffer.str());
    }

    SceneSerializer::SceneSerializer(const Ref<Scene> &scene)
        : m_Scene(scene)
    {
    }

    bool SceneSerializer::Serialize(const std::filesystem::path &filepath)
    {
        if (!m_Scene)
            return false;

        Serializer sr(filepath);

        sr.BeginMap(); // START

        sr.BeginMap("Scene"); // scene file header
        sr.AddKeyValue<std::string>("Version", ENGINE_VERSION);
        sr.AddKeyValue<std::string>("Title", m_Scene->name);
        sr.BeginSequence("Entities");

        // entities sequence
        for (auto& e : m_Scene->entities | std::views::values)
        {
            Entity entity = { e, m_Scene.get() };
            const ID &idComp = entity.GetComponent<ID>();

            sr.BeginMap(); // START Entity
            {
                // ID Component
                sr.AddKeyValue("ID", idComp.uuid);
                sr.AddKeyValue("Name", idComp.name);
                sr.AddKeyValue("Type", EntityTypeToString(idComp.type));
                sr.AddKeyValue("Parent", idComp.parent);

                // Transform Component
                if (entity.HasComponent<Transform>())
                {
                    const Transform &comp = entity.GetComponent<Transform>();
                    sr.BeginMap("Transform");
                    {
                        sr.AddKeyValue("WorldTranslation", comp.translation);
                        sr.AddKeyValue("WorldRotation", comp.rotation);
                        sr.AddKeyValue("WorldScale", comp.scale);

                        sr.AddKeyValue("LocalTranslation", comp.localTranslation);
                        sr.AddKeyValue("LocalRotation", comp.localRotation);
                        sr.AddKeyValue("LocalScale", comp.localScale);

                        sr.AddKeyValue("Visible", comp.visible);
                    }
                    sr.EndMap();
                }

                // Camera
                if (entity.HasComponent<Camera>())
                {
                    const Camera &comp = entity.GetComponent<Camera>();
                    sr.BeginMap("Camera");
                    {
                        int projectionType = static_cast<int>(comp.projectionType);
                        sr.AddKeyValue("ProjectionType", projectionType);
                        sr.AddKeyValue("NearClip", comp.nearClip);
                        sr.AddKeyValue("FarClip", comp.farClip);
                        sr.AddKeyValue("Zoom", comp.zoom);
                        sr.AddKeyValue("Fov", comp.fov);
                        sr.AddKeyValue("Primary", comp.primary);
                    }
                    sr.EndMap();
                }

                // Sprite component
                if (entity.HasComponent<Sprite2D>())
                {
                    const Sprite2D &comp = entity.GetComponent<Sprite2D>();
                    sr.BeginMap("Sprite2D");
                    {
                        sr.AddKeyValue("Color", comp.color);
                        sr.AddKeyValue("TilingFactor", comp.tilingFactor);
                    }
                    sr.EndMap();
                }

                // Rigidbody 2D
                if (entity.HasComponent<Rigidbody2D>())
                {
                    const Rigidbody2D &comp = entity.GetComponent<Rigidbody2D>();
                    sr.BeginMap("Rigidbody2D");
                    {
                        sr.AddKeyValue("Type", BodyTypeToString(comp.type));
                        sr.AddKeyValue("LinearVelocity", comp.linearVelocity);
                        sr.AddKeyValue("AngularVelocity", comp.angularVelocity);
                        sr.AddKeyValue("GravityScale", comp.gravityScale);
                        sr.AddKeyValue("LinearDamping", comp.linearDamping);
                        sr.AddKeyValue("AngularDamping", comp.angularDamping);
                        sr.AddKeyValue("FixedRotation", comp.fixedRotation);
                        sr.AddKeyValue("IsAwake", comp.isAwake);
                        sr.AddKeyValue("IsEnabled", comp.isEnabled);
                        sr.AddKeyValue("IsEnableSleep", comp.isEnableSleep);
                    }
                    sr.EndMap();
                }

                // Box collider 2D
                if (entity.HasComponent<BoxCollider2D>())
                {
                    const BoxCollider2D &comp = entity.GetComponent<BoxCollider2D>();
                    sr.BeginMap("BoxCollider2D");
                    {
                        sr.AddKeyValue("Size", comp.size);
                        sr.AddKeyValue("Offset", comp.offset);
                        sr.AddKeyValue("Restitution", comp.restitution);
                        sr.AddKeyValue("Friction", comp.friction);
                        sr.AddKeyValue("Density", comp.density);
                        sr.AddKeyValue("IsSensor", comp.isSensor);
                    }
                    sr.EndMap();
                }

                // skinned mesh
                if (entity.HasComponent<SkinnedMesh>())
                {
                    const SkinnedMesh &comp = entity.GetComponent<SkinnedMesh>();
                    sr.BeginMap("SkinnedMesh");
                    sr.AddKeyValue("Filepath", comp.filepath.generic_string());
                    sr.EndMap();
                }

                // Mesh Renderer
                if (entity.HasComponent<MeshRenderer>())
                {
                    const MeshRenderer &comp = entity.GetComponent<MeshRenderer>();
                    sr.BeginMap("MeshRenderer");
                    {
                        sr.AddKeyValue("Root", static_cast<uint64_t>(comp.root));
                        sr.AddKeyValue("MeshIndex", comp.meshIndex);
                    }
                    sr.EndMap();
                }

                // Audio Source
                if (entity.HasComponent<AudioSource>())
                {
                    const AudioSource &comp = entity.GetComponent<AudioSource>();
                    sr.BeginMap("AudioSource");
                    {
                        sr.AddKeyValue("Handle", static_cast<uint64_t>(comp.handle));
                        sr.AddKeyValue("Volume", comp.volume);
                        sr.AddKeyValue("Pitch", comp.pitch);
                        sr.AddKeyValue("Pan", comp.pan);
                        sr.AddKeyValue("PlayOnStart", comp.playOnStart);
                    }
                    sr.EndMap();
                }

                // Script
                if (entity.HasComponent<Script>())
                {
                    const Script &comp = entity.GetComponent<Script>();
                    sr.BeginMap("Script");
                    {
                        sr.AddKeyValue("ClassName", comp.className);

                        // Fields
                        const Ref<ScriptClass> scriptClass = ScriptEngine::GetEntityClassesByName(comp.className);

                        if (scriptClass)
                        {
                            const auto &classFields = scriptClass->GetFields();

                            if (!classFields.empty())
                            {
                                auto &fields = ScriptEngine::GetScriptFieldMap(entity);

                                sr.BeginSequence("Fields");
                                for (const auto &[fieldName, field] : classFields)
                                {
                                    if (fields.find(fieldName) == fields.end() || field.Type == ScriptFieldType::Invalid)
                                    {
                                        continue;
                                    }

                                    sr.BeginMap();
                                    sr.AddKeyValue("Name", fieldName);
                                    sr.AddKeyValue("Type", Utils::ScriptFieldTypeToString(field.Type));
                                    
                                    ScriptFieldInstance fieldInstance = fields.at(fieldName);
                                    switch (field.Type)
                                    {
                                    case ScriptFieldType::Float: sr.AddKeyValue("Value", fieldInstance.GetValue<float>()); break;
                                    case ScriptFieldType::Double: sr.AddKeyValue("Value", fieldInstance.GetValue<double>()); break;
                                    case ScriptFieldType::Bool: sr.AddKeyValue("Value", fieldInstance.GetValue<bool>()); break;
                                    case ScriptFieldType::Char: sr.AddKeyValue("Value", fieldInstance.GetValue<char>()); break;
                                    case ScriptFieldType::Byte: sr.AddKeyValue("Value", fieldInstance.GetValue<int8_t>()); break;
                                    case ScriptFieldType::Short: sr.AddKeyValue("Value", fieldInstance.GetValue<int16_t>()); break;
                                    case ScriptFieldType::Long: sr.AddKeyValue("Value", fieldInstance.GetValue<int64_t>()); break;
                                    case ScriptFieldType::UByte: sr.AddKeyValue("Value", fieldInstance.GetValue<uint8_t>()); break;
                                    case ScriptFieldType::UShort: sr.AddKeyValue("Value", fieldInstance.GetValue<uint16_t>()); break;
                                    case ScriptFieldType::UInt: sr.AddKeyValue("Value", fieldInstance.GetValue<uint32_t>()); break;
                                    case ScriptFieldType::ULong: sr.AddKeyValue("Value", fieldInstance.GetValue<uint64_t>()); break;
                                    case ScriptFieldType::Int: sr.AddKeyValue("Value", fieldInstance.GetValue<int>()); break;
                                    case ScriptFieldType::Vector2: sr.AddKeyValue("Value", fieldInstance.GetValue<glm::vec2>()); break;
                                    case ScriptFieldType::Vector3: sr.AddKeyValue("Value", fieldInstance.GetValue<glm::vec3>()); break;
                                    case ScriptFieldType::Vector4: sr.AddKeyValue("Value", fieldInstance.GetValue<glm::vec4>()); break;
                                    case ScriptFieldType::Entity: sr.AddKeyValue("Value", fieldInstance.GetValue<uint64_t>()); break;
                                    }

                                    sr.EndMap();
                                }

                                sr.EndSequence();
                            }
                        }

                    }
                    sr.EndMap();
                }

            }
            sr.EndMap(); // END Entity
        }

        sr.EndSequence(); // Entities
        sr.EndMap(); // scene

        sr.EndMap(); // END

        // Example
#if 0
        sr.BeginMap(); // START

        sr.BeginMap("Scene"); // scene file header

        sr.AddKeyValue<std::string>("Title", m_Scene->name);
        sr.AddKeyValue<std::string>("Version", ENGINE_VERSION);

        sr.BeginSequence("Entities");


        // entites sequence
        for (int i = 0; i < 10; ++i)
        {
            sr.BeginMap(); // START Entity
            {
                sr.AddKeyValue<std::string>("ID", "ENTITY_ID");
                sr.AddKeyValue<std::string>("Type", "ENTITY_TYPE");
                sr.AddKeyValue<std::string>("Parent", "PARENT_ID");
                sr.BeginMap("Component A");
                {
                    sr.AddKeyValue("Var A", "value");
                    sr.AddKeyValue("Var B", "value");

                    sr.BeginSequence("List Var");
                    {
                        sr.BeginMap();
                        {
                            sr.AddKeyValue("List Var A", "value");
                            sr.AddKeyValue("List Var B", "value");
                        }
                        sr.EndMap();
                    }
                    sr.EndSequence();
                }
                sr.EndMap();
            }
            sr.EndMap(); // END Entity
        }

        sr.EndSequence(); // Entities
        sr.EndMap(); // scene

        sr.EndMap(); // END

#endif
        sr.Serialize();

        // Scene should be not dirty
        m_Scene->SetDirtyFlag(false);

        return true;
    }

    Ref<Scene> SceneSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        LOG_ASSERT(std::filesystem::exists(filepath), "[Scene SR] File does not exists!\n{}", filepath.generic_string());

        YAML::Node sceneFileNode = Serializer::Deserialize(filepath);
        YAML::Node sceneNode = sceneFileNode["Scene"];

        LOG_ASSERT(sceneNode, "[Scene SR] Invalid scene file");
        if (!sceneNode)
            return nullptr;

        std::string title = sceneNode["Title"].as<std::string>();
        Ref<Scene> desScene = Scene::Create(title);

        for (YAML::Node entityNode : sceneNode["Entities"])
        {
            UUID uuid = UUID(entityNode["ID"].as<uint64_t>());
            std::string name = entityNode["Name"].as<std::string>();
            EntityType type = EntityTypeFromString(entityNode["Type"].as<std::string>());

            Entity desEntity = SceneManager::CreateEntity(desScene.get(), name, type, uuid);
            UUID parent = UUID(entityNode["Parent"].as<uint64_t>());
            desEntity.GetComponent<ID>().parent = parent;

            // Transform component
            if (YAML::Node node = entityNode["Transform"])
            {
                Transform &comp = desEntity.AddComponent<Transform>();
                comp.translation = node["WorldTranslation"].as<glm::vec3>();
                comp.rotation = node["WorldRotation"].as<glm::quat>();
                comp.scale = node["WorldScale"].as<glm::vec3>();
                comp.localTranslation = node["WorldTranslation"].as<glm::vec3>();
                comp.localRotation = node["WorldRotation"].as<glm::quat>();
                comp.localScale = node["WorldScale"].as<glm::vec3>();
                comp.visible = node["Visible"].as<bool>();
            }

            // Camera component
            if (YAML::Node node = entityNode["Camera"])
            {
                Camera &comp = desEntity.AddComponent<Camera>();
                comp.projectionType = static_cast<ICamera::Type>(node["ProjectionType"].as<int>());
                comp.nearClip = node["NearClip"].as<float>();
                comp.farClip = node["FarClip"].as<float>();
                comp.zoom = node["Zoom"].as<float>();
                comp.fov = node["Fov"].as<float>();
                comp.primary = node["Primary"].as<bool>();
            }

            // Sprite2D component
            if (YAML::Node node = entityNode["Sprite2D"])
            {
                Sprite2D &comp = desEntity.AddComponent<Sprite2D>();
                comp.color = node["Color"].as<glm::vec4>();
                comp.tilingFactor = node["TilingFactor"].as<glm::vec2>();
            }

            // Rigidbody 2D
            if (YAML::Node node = entityNode["Rigidbody2D"])
            {
                Rigidbody2D &comp = desEntity.AddComponent<Rigidbody2D>();
                comp.type = BodyTypeFromString(node["Type"].as<std::string>());
                comp.linearVelocity = node["LinearVelocity"].as<glm::vec2>();
                comp.angularVelocity = node["AngularVelocity"].as<float>();
                comp.gravityScale = node["GravityScale"].as<float>();
                comp.linearDamping = node["LinearDamping"].as<float>();
                comp.angularDamping = node["AngularDamping"].as<float>();
                comp.fixedRotation = node["FixedRotation"].as<bool>();
                comp.isAwake = node["IsAwake"].as<bool>();
                comp.isEnabled = node["IsEnabled"].as<bool>();
                comp.isEnableSleep = node["IsEnableSleep"].as<bool>();
            }

            // BoxCollider 2D
            if (YAML::Node node = entityNode["BoxCollider2D"])
            {
                BoxCollider2D &comp = desEntity.AddComponent<BoxCollider2D>();
                comp.size = node["Size"].as<glm::vec2>();
                comp.offset = node["Offset"].as<glm::vec2>();
                comp.restitution = node["Restitution"].as<float>();
                comp.friction = node["Friction"].as<float>();
                comp.density = node["Density"].as<float>();
                comp.isSensor = node["IsSensor"].as<bool>();
            }

            // Audio Source
            if (YAML::Node node = entityNode["AudioSource"])
            {
                AudioSource &comp = desEntity.AddComponent<AudioSource>();
                comp.handle = AssetHandle(node["Handle"].as<uint64_t>());
                comp.volume = node["Volume"].as<float>();
                comp.pitch= node["Pitch"].as<float>();
                comp.pan = node["Pan"].as<float>();
                comp.playOnStart = node["PlayOnStart"].as<bool>();
            }

            // Script
            if (YAML::Node node = entityNode["Script"])
            {
                Script &sc = desEntity.AddComponent<Script>();
                sc.className = node["ClassName"].as<std::string>();

                if (YAML::Node classFieldsNode = node["Fields"])
                {
                    Ref<ScriptClass> scriptClass = ScriptEngine::GetEntityClassesByName(sc.className);
                    if (scriptClass)
                    {
                        const auto &classFields = scriptClass->GetFields();
                        ScriptFieldMap &fieldMap = ScriptEngine::GetScriptFieldMap(desEntity);

                        for (YAML::Node fieldNode : classFieldsNode)
                        {
                            std::string fieldName = fieldNode["Name"].as<std::string>();
                            ScriptFieldType fieldType = Utils::ScriptFieldTypeFromString(fieldNode["Type"].as<std::string>());

                            ScriptFieldInstance &fieldInstance = fieldMap[fieldName];

                            if (!fieldMap.contains(fieldName))
                                continue;

                            fieldInstance.Field = classFields.at(fieldName);

                            switch (fieldType)
                            {
                            case ScriptFieldType::Float: fieldInstance.SetValue(fieldNode["Value"].as<float>()); break;
                            case ScriptFieldType::Double: fieldInstance.SetValue(fieldNode["Value"].as<double>()); break;
                            case ScriptFieldType::Bool: fieldInstance.SetValue(fieldNode["Value"].as<bool>()); break;
                            case ScriptFieldType::Char: fieldInstance.SetValue(fieldNode["Value"].as<char>()); break;
                            case ScriptFieldType::Byte: fieldInstance.SetValue(fieldNode["Value"].as<int8_t>()); break;
                            case ScriptFieldType::Short: fieldInstance.SetValue(fieldNode["Value"].as<int16_t>()); break;
                            case ScriptFieldType::Long: fieldInstance.SetValue(fieldNode["Value"].as<int64_t>()); break;
                            case ScriptFieldType::UByte: fieldInstance.SetValue(fieldNode["Value"].as<uint8_t>()); break;
                            case ScriptFieldType::UShort: fieldInstance.SetValue(fieldNode["Value"].as<uint16_t>()); break;
                            case ScriptFieldType::UInt: fieldInstance.SetValue(fieldNode["Value"].as<uint32_t>()); break;
                            case ScriptFieldType::ULong: fieldInstance.SetValue(fieldNode["Value"].as<uint64_t>()); break;
                            case ScriptFieldType::Int: fieldInstance.SetValue(fieldNode["Value"].as<int>()); break;
                            case ScriptFieldType::Entity: fieldInstance.SetValue(fieldNode["Value"].as<uint64_t>()); break;
                            case ScriptFieldType::Vector2: fieldInstance.SetValue(fieldNode["Value"].as<glm::vec2>()); break;
                            case ScriptFieldType::Vector3: fieldInstance.SetValue(fieldNode["Value"].as<glm::vec3>()); break;
                            case ScriptFieldType::Vector4: fieldInstance.SetValue(fieldNode["Value"].as<glm::vec4>()); break;
                            }
                        }
                    }
                }
            }
        }
        
        // attach each node to it's parent
        for (auto [uuid, e] : desScene->entities)
        {
            Entity entity{ e, desScene.get() };

            if (entity.GetParentUUID() != UUID(0))
            {
                Entity parent = SceneManager::GetEntity(desScene.get(), entity.GetParentUUID());
                SceneManager::AddChild(desScene.get(), parent, entity);
            }
        }

        return desScene;
    }


    ProjectSerializer::ProjectSerializer(const Ref<Project> &project)
        : m_Project(project)
    {
    }

    bool ProjectSerializer::Serialize(const std::filesystem::path &filepath)
    {
        if (!m_Project)
            return false;

        const auto &projectInfo = m_Project->GetInfo();

        Serializer projectSr(filepath);

        projectSr.BeginMap(); // START

        projectSr.BeginMap("Project");

        projectSr.AddKeyValue("Version", ENGINE_VERSION);
        projectSr.AddKeyValue("Name", projectInfo.name);
        projectSr.AddKeyValue("AssetPath", projectInfo.assetDirectory.generic_string());
        projectSr.AddKeyValue("AssetRegistry", projectInfo.assetRegistryFilepath.generic_string());
        projectSr.AddKeyValue("DefaultSceneHandle", projectInfo.defaultSceneHandle);

        projectSr.EndMap();

        projectSr.EndMap(); // END

        projectSr.Serialize();

        // set dirty flags
        m_Project->SetDirtyFlag(false);

        // Serialize asset manager
        auto &assetManager = m_Project->GetAssetManager();
        auto &assetRegistry = assetManager.GetAssetAssetRegistry();

        {
            const std::filesystem::path assetRegFilepath = filepath.parent_path() / projectInfo.assetRegistryFilepath;
            Serializer assetSr(assetRegFilepath);

            assetSr.BeginMap(); // Start

            assetSr.BeginMap("AssetRegistry");

            assetSr.BeginSequence("Assets"); // Asset sequence
            for (auto &[handle, metadata] : assetRegistry)
            {
                assetSr.BeginMap(); // Begin Metadata

                assetSr.AddKeyValue("Handle", static_cast<uint64_t>(handle));
                assetSr.AddKeyValue("Type", AssetTypeToString(metadata.type));
                assetSr.AddKeyValue("Filepath", metadata.filepath.generic_string());

                assetSr.EndMap();
            }

            assetSr.EndSequence(); // Asset sequence

            assetSr.EndMap(); // End

            assetSr.Serialize();
        }

        return true;
    }

    Ref<Project> ProjectSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        bool exists = std::filesystem::exists(filepath);
        LOG_ASSERT(exists, "[Project Serializer] File does not exists");
        if (!exists)
        {
            return nullptr;
        }

        YAML::Node projectFileNode = Serializer::Deserialize(filepath);
        YAML::Node projectNode = projectFileNode["Project"];

        ProjectInfo info;
        info.name = projectNode["Name"].as<std::string>();
        info.filepath = filepath;
        info.assetDirectory = projectNode["AssetPath"].as<std::string>();
        info.assetRegistryFilepath = projectNode["AssetRegistry"].as<std::string>();
        info.defaultSceneHandle = AssetHandle(projectNode["DefaultSceneHandle"].as<uint64_t>());

        Ref<Project> project = Project::Create(info);

        auto &assetManager = project->GetAssetManager();

        // import registry
        if (!info.assetRegistryFilepath.empty())
        {
            // project filepath / asset filename (.ixreg)
            std::filesystem::path assetRegFilepath = filepath.parent_path() / info.assetRegistryFilepath;
            YAML::Node assetRegFileNode = Serializer::Deserialize(assetRegFilepath);
            YAML::Node assetRegNode = assetRegFileNode["AssetRegistry"];

            for (YAML::Node assetNode : assetRegNode["Assets"])
            {
                AssetHandle handle = AssetHandle(assetNode["Handle"].as<uint64_t>());
                AssetMetaData metadata;
                metadata.type = AssetTypeFromString(assetNode["Type"].as<std::string>());
                metadata.filepath = assetNode["Filepath"].as<std::string>();

                assetManager.InsertMetaData(handle, metadata);
            }
        }

        return project;
    }


    AnimationSerializer::AnimationSerializer(const SkeletalAnimation &animation)
        : m_Animation(animation)
    {
    }

    bool AnimationSerializer::Serialize(const std::filesystem::path &filepath)
    {
        Serializer sr(filepath);

        sr.BeginMap(); // START

        sr.BeginMap("Animation");
        sr.AddKeyValue("Version", ENGINE_VERSION);
        sr.AddKeyValue("Name", m_Animation.name);
        sr.AddKeyValue("Duration", m_Animation.duration);
        sr.AddKeyValue("TicksPerSeconds", m_Animation.ticksPerSeconds);

        sr.BeginSequence("Channels");

        for (auto &[name, channel] : m_Animation.channels)
        {
            sr.BeginMap();

            sr.AddKeyValue("Name", name);

            // Translation
            sr.BeginSequence("TranslationKeys");
            for (auto &f : channel.translationKeys.frames)
            {
                sr.BeginMap();
                sr.AddKeyValue("Timestamp", f.Timestamp);
                sr.AddKeyValue("Value", f.Value);
                sr.EndMap();
            }
            sr.EndSequence();

            // Rotation
            sr.BeginSequence("RotationKeys");
            for (auto &f : channel.rotationKeys.frames)
            {
                sr.BeginMap();
                sr.AddKeyValue("Timestamp", f.Timestamp);
                sr.AddKeyValue("Value", f.Value);
                sr.EndMap();
            }
            sr.EndSequence();

            // Scale
            sr.BeginSequence("ScaleKeys");
            for (auto &f : channel.scaleKeys.frames)
            {
                sr.BeginMap();
                sr.AddKeyValue("Timestamp", f.Timestamp);
                sr.AddKeyValue("Value", f.Value);
                sr.EndMap();
            }
            sr.EndSequence();

            sr.EndMap();
        }

        sr.EndSequence();

        sr.EndMap();

        sr.EndMap(); // END

        sr.Serialize(filepath);

        return true;
    }

    SkeletalAnimation AnimationSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        SkeletalAnimation animation;

        return animation;
    }
}