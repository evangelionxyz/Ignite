#include "serializer.hpp"

#include "ignite/scene/scene.hpp"
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
}