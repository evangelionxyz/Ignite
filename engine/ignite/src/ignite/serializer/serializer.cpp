#include "serializer.hpp"

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
        std::ofstream outFile(m_Filepath, std::ios::trunc);
        outFile << m_Emitter.c_str();
        outFile.close();
    }

    void Serializer::Serialize(const std::filesystem::path &filepath)
    {
        m_Filepath = filepath;

        std::ofstream outFile(m_Filepath, std::ios::trunc);
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

        sr.AddKeyValue<std::string>("Title", m_Scene->name);
        sr.AddKeyValue<std::string>("Version", "1.0");

        sr.BeginSequence("Entities");

        // entites sequence
        for (auto &[uuid, e] : m_Scene->entities)
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
        sr.AddKeyValue<std::string>("Version", "1.0");

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

        projectSr.AddKeyValue("Version", "1.0");
        projectSr.AddKeyValue("Name", projectInfo.name);
        projectSr.AddKeyValue("AssetPath", projectInfo.assetFilepath.generic_string());
        projectSr.AddKeyValue("AssetRegistry", projectInfo.assetRegistryFilename.generic_string());
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
            const std::filesystem::path assetRegFilepath = filepath.parent_path() / projectInfo.assetRegistryFilename;
            Serializer assetSr(assetRegFilepath);

            assetSr.BeginMap(); // Start

            assetSr.BeginMap("AssetRegistry");

            assetSr.BeginSequence("Assets"); // Asset sequence
            for (auto &[handle, metadata] : assetRegistry)
            {
                assetSr.BeginMap(); // Begin Metadata

                metadata.filepath = m_Project->GetAssetRelativeFilepath(metadata.filepath);

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
        info.assetFilepath = projectNode["AssetPath"].as<std::string>();
        info.assetRegistryFilename = projectNode["AssetRegistry"].as<std::string>();
        info.defaultSceneHandle = AssetHandle(projectNode["DefaultSceneHandle"].as<uint64_t>());

        Ref<Project> project = Project::Create(info);

        auto &assetManager = project->GetAssetManager();

        // import registry
        if (!info.assetRegistryFilename.empty())
        {
            // project filepath / asset filename (.ixreg)
            std::filesystem::path assetRegFilepath = filepath.parent_path() / info.assetRegistryFilename;
            YAML::Node assetRegFileNode = Serializer::Deserialize(assetRegFilepath);
            YAML::Node assetRegNode = assetRegFileNode["AssetRegistry"];

            for (YAML::Node assetNode : assetRegNode["Assets"])
            {
                AssetHandle handle = AssetHandle(assetNode["Handle"].as<uint64_t>());
                AssetMetaData metadata;
                metadata.type = AssetTypeFromString(assetNode["Type"].as<std::string>());
                metadata.filepath = assetNode["Filepath"].as<std::string>();

                assetManager.InsertMetaData(handle, metadata);

                // assetManager.ImportAsset(metadata.filepath);
            }
        }

        if (info.defaultSceneHandle != AssetHandle(0))
        {
            Ref<Scene> activeScene = Project::GetAsset<Scene>(info.defaultSceneHandle);
            if (activeScene)
                project->SetActiveScene(activeScene);
        }

        return project;
    }


    AnimationSerializer::AnimationSerializer(const Ref<SkeletalAnimation> &animation)
        : m_Animation(animation)
    {
    }

    bool AnimationSerializer::Serialize(const std::filesystem::path &filepath)
    {
        if (!m_Animation)
            return false;

        Serializer sr(filepath);

        sr.BeginMap(); // START

        sr.BeginMap("Animation");
        sr.AddKeyValue("Name", m_Animation->name);
        sr.AddKeyValue("Duration", m_Animation->duration);
        sr.AddKeyValue("TicksPerSeconds", m_Animation->ticksPerSeconds);

        sr.BeginSequence("Channels");

        for (auto &[name, channel] : m_Animation->channels)
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

    Ref<SkeletalAnimation> AnimationSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        Ref<SkeletalAnimation> animation;

        return animation;
    }
}