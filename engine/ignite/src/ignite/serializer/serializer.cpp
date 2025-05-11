#include "serializer.hpp"

#include "ignite/scene/scene.hpp"
#include "ignite/core/logger.hpp"

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
        sr.Serialize();

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

        std::string sceneName = sceneNode.as<std::string>();
        Ref<Scene> desScene = Scene::Create(sceneName);

        // TODO: iterate entities

        return desScene;
    }

}