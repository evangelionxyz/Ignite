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
        void RemoveAsset(AssetHandle handle);

        Ref<Asset> GetAsset(AssetHandle handle);
        AssetType GetAssetType(AssetHandle handle);

        const AssetMetaData &GetMetaData(AssetHandle handle) const;
        const std::filesystem::path &GetFilepath(AssetHandle handle);
        bool IsAssetHandleValid(AssetHandle handle);
    
        AssetRegistry &GetAssetAssetRegistry() { return m_AssetRegistry; }

    private:
        AssetRegistry m_AssetRegistry;
    };

}