#pragma once

#include "asset.hpp"

#include <map>

namespace ignite {

    using AssetRegistry = std::map<AssetHandle, AssetMetaData>;

    class AssetManager
    {
    public:
        AssetManager() = default;

        AssetHandle ImportAsset(const std::filesystem::path &filepath);
        void InsertMetaData(AssetHandle handle, const AssetMetaData &metadata);
        void RemoveAsset(AssetHandle handle);
        Ref<Asset> GetAsset(AssetHandle handle);
        AssetType GetAssetType(AssetHandle handle);
        const AssetMetaData &GetMetaData(const std::filesystem::path &filepath, AssetHandle &outHandle);
        const AssetMetaData &GetMetaData(AssetHandle handle) const;
        AssetHandle GetAssetHandle(const std::filesystem::path &filepath);
        
        const std::filesystem::path &GetFilepath(AssetHandle handle);
        bool IsAssetHandleValid(AssetHandle handle);
    
        AssetRegistry &GetAssetAssetRegistry() { return m_AssetRegistry; }

    private:
        Ref<Asset> Import(AssetHandle handle, const AssetMetaData &metadata);

        AssetRegistry m_AssetRegistry;
        std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
    };

}