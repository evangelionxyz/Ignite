#pragma once

#include "ignite/asset/asset.hpp"
#include "ignite/asset/asset_manager.hpp"

#include <string>
#include <filesystem>

namespace ignite
{
    class Scene;

    struct ProjectInfo
    {
        std::string name;
        std::filesystem::path filepath;
        std::filesystem::path assetFilepath = "/Assets";
    };

    class Project : public Asset
    {
    public:
        Project() = default;
        Project(const ProjectInfo &info);
        
        const ProjectInfo &GetInfo() const { return m_Info; }
        const Ref<Scene> &GetActiveScene() { return m_ActiveScene; }

        const AssetManager &GetAssetManager() { return m_AssetManager; }

        static Project *GetInstance();
        static Ref<Project> Create(const ProjectInfo &info);

        static AssetType GetStaticType() { return AssetType::Project; }
        virtual AssetType GetType() override { return GetStaticType(); }

    private:
        void SerializeAssetRegistry();

        Ref<Scene> m_ActiveScene;
        ProjectInfo m_Info;

        AssetManager m_AssetManager;
    };
}