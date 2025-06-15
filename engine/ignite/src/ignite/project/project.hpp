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
        AssetHandle defaultSceneHandle = AssetHandle(0);

        std::filesystem::path filepath; // the actual project file (.ixproj)
        std::filesystem::path scriptModuleFilepath;
        std::filesystem::path assetDirectory = "Assets";
        std::filesystem::path scriptsDirectory = "Scripts";
        std::filesystem::path assetRegistryFilepath = "AssetRegistry.ixreg";
        std::filesystem::path premakeFilepath = "premake5.lua";
        std::filesystem::path batchScriptFilepath = "build.bat";
    };

    class Project : public Asset
    {
    public:
        Project() = default;

        Project(const ProjectInfo &info);

        ~Project() override;
        
        std::filesystem::path GetAssetRelativeFilepath(const std::filesystem::path &filepath) const;
        std::filesystem::path GetAssetFilepath(const std::filesystem::path &filepath) const;
        std::filesystem::path GetRelativeFilepath(const std::filesystem::path &filepath) const;
        
        void SetActiveScene(const Ref<Scene> &scene);
        bool BuildSolution();
        std::vector<std::pair<AssetHandle, AssetMetaData>> ValidateAssetRegistry();


        std::filesystem::path GetDirectory() const
        {
            return m_Info.filepath.parent_path();
        }

        std::filesystem::path GetSolutionFilepath() const
        {
            return GetDirectory() / std::string(m_Info.name + ".sln");
        }

        std::filesystem::path GetAssetDirectory() const
        {
            return GetDirectory() / m_Info.assetDirectory;
        }

        std::filesystem::path GetScriptsDirectory() const
        {
            return GetDirectory() / m_Info.scriptsDirectory;
        }

        // Static methods
        static std::filesystem::path GetActiveScriptsDirectory()
        {
            return GetActive()->GetScriptsDirectory();
        }

        static std::filesystem::path GetActiveSolutionFilepath()
        {
            return GetActive()->GetSolutionFilepath();
        }

        static std::filesystem::path GetActiveAssetDirectory()
        {
            return GetActive()->GetAssetDirectory();
        }

        static std::filesystem::path GetActiveProjectDirectory()
        {
            return GetActive()->GetDirectory();
        }

        template<typename T>
        static Ref<T> GetAsset(AssetHandle handle)
        {
            Ref<Asset> asset = GetActive()->GetAssetManager().GetAsset(handle);
            return std::static_pointer_cast<T>(asset);
        }

        AssetManager &GetAssetManager() { return m_AssetManager; }
        ProjectInfo &GetInfo() { return m_Info; }
        const Ref<Scene> &GetActiveScene() { return m_ActiveScene; }

        static Project *GetActive();
        static Ref<Project> Create(const ProjectInfo &info);

        static AssetType GetStaticType() { return AssetType::Project; }
        virtual AssetType GetType() override { return GetStaticType(); }

    private:
        void GenerateProject();
        void SerializeAssetRegistry();

        Ref<Scene> m_ActiveScene; // current active scene in editor
        ProjectInfo m_Info;

        AssetManager m_AssetManager;
    };

}