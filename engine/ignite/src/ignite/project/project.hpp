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
        std::filesystem::path filepath;
        std::filesystem::path assetFilepath = "Assets";
        std::filesystem::path assetRegistryFilename = "AssetRegistry.ixreg";
        std::filesystem::path scriptModulePath = "bin/GameAssembly.dll";
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
        
        std::vector<std::pair<AssetHandle, AssetMetaData>> ValidateAssetRegistry()
        {
            std::vector<std::pair<AssetHandle, AssetMetaData>> invalidRegistry;
            AssetRegistry &assetRegistry = GetAssetManager().GetAssetAssetRegistry();

            for (auto it = assetRegistry.begin(); it != assetRegistry.end();)
            {
                const std::filesystem::path &filepath = GetAssetFilepath(it->second.filepath);
                if (!std::filesystem::exists(filepath))
                {
                    invalidRegistry.emplace_back(it->first, it->second);
                    it = assetRegistry.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            
            return invalidRegistry;
        }

        template<typename T>
        static Ref<T> GetAsset(AssetHandle handle)
        {
            Ref<Asset> asset = GetInstance()->GetAssetManager().GetAsset(handle);
            return std::static_pointer_cast<T>(asset);
        }


        const Ref<Scene> &GetActiveScene() { return m_ActiveScene; }

        std::filesystem::path GetAssetDirectory() const
        {
            return m_Info.filepath.parent_path() / m_Info.assetFilepath;
        }

        static std::filesystem::path GetActiveAssetDirectory()
        {
            return GetInstance()->GetAssetDirectory();
        }

        static std::filesystem::path GetActiveDirectory()
        {
            return GetInstance()->m_Info.filepath.parent_path();
        }

        AssetManager &GetAssetManager() { return m_AssetManager; }
        ProjectInfo &GetInfo() { return m_Info; }

        static Project *GetInstance();
        static Ref<Project> Create(const ProjectInfo &info);

        static AssetType GetStaticType() { return AssetType::Project; }
        virtual AssetType GetType() override { return GetStaticType(); }

    private:
        void SerializeAssetRegistry();

        Ref<Scene> m_ActiveScene; // current active scene in editor
        ProjectInfo m_Info;

        AssetManager m_AssetManager;
    };

}