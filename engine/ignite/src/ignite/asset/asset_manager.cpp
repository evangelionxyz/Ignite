#include "asset_manager.hpp"
#include "asset_importer.hpp"

#include "ignite/core/logger.hpp"
#include <cstdint>

namespace ignite {

    AssetHandle AssetManager::ImportAsset(const std::filesystem::path &filepath)
    {
        AssetHandle handle;
        
        // create metadata
        AssetMetaData metadata;
        metadata.filepath = filepath;
        metadata.type = GetAssetTypeFromExtension(metadata.filepath.extension().generic_string());

        // Invalid 
        if (metadata.type == AssetType::Invalid)
        {
            LOG_ERROR("[Asset Manager] Invalid asset type {}", filepath.generic_string());
            return AssetHandle(0);
        }

        // Find in registered asset first
        for (const auto &[assetHandle, assetMetaData] : m_AssetRegistry)
        {
            if (metadata.filepath == assetMetaData.filepath)
            {
                // found it
                handle = assetHandle;
                metadata = assetMetaData;
                break;
            }
        }

        // get the asset
        Ref<Asset> asset = GetAsset(handle);
        if (!asset)
        {
            m_AssetRegistry[handle] = metadata;
        }

        return handle;
    }

    void AssetManager::InsertMetaData(AssetHandle handle, const AssetMetaData &metadata)
    {
        m_AssetRegistry[handle] = metadata;
    }

    void AssetManager::RemoveAsset(AssetHandle handle)
    {
        auto it = m_AssetRegistry.find(handle);
        if (it != m_AssetRegistry.end())
            m_AssetRegistry.erase(it);
    }

    Ref<Asset> AssetManager::GetAsset(AssetHandle handle)
    {
        if (!IsAssetHandleValid(handle))
        {
            return nullptr;
        }

        Ref<Asset> asset;
        const AssetMetaData &metadata = GetMetaData(handle);

        switch (metadata.type)
        {
            case AssetType::Invalid:
            {
                LOG_ERROR("[Asset Manager] Invalid asset type!");
                return nullptr;
            }

            case AssetType::Scene:
            {
                asset = AssetImporter::Import(handle, metadata);
                break;
            }
        }

        return asset;
    }

    AssetType AssetManager::GetAssetType(AssetHandle handle)
    {
        return GetMetaData(handle).type;
    }

    const AssetMetaData &AssetManager::GetMetaData(AssetHandle handle) const
    {
        static AssetMetaData s_NullMetaData;
        
        if (m_AssetRegistry.contains(handle))
        {
            return m_AssetRegistry.at(handle);
        }

        return s_NullMetaData;
    }

    const std::filesystem::path &AssetManager::GetFilepath(AssetHandle handle)
    {
        return GetMetaData(handle).filepath;
    }

    bool AssetManager::IsAssetHandleValid(AssetHandle handle)
    {
        return (uint64_t)handle != 0 && m_AssetRegistry.contains(handle);
    }
}