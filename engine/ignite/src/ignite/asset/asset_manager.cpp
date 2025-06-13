#include "asset_manager.hpp"
#include "asset_importer.hpp"

#include "ignite/core/logger.hpp"
#include <cstdint>

namespace ignite {

    static AssetMetaData s_NullMetaData;

    AssetHandle AssetManager::ImportAsset(const std::filesystem::path &filepath)
    {
        bool foundInAssetRegistry = false;

        AssetHandle handle = AssetHandle(0);
        
        std::string fileExtension = filepath.extension().generic_string();

        // create metadata
        AssetMetaData metadata;

        // ixasset means it is part of the project
        // so we need to check from the asset registry
        if (fileExtension == ".ixasset")
        {
            metadata = GetMetaData(filepath, handle);
            foundInAssetRegistry = metadata.type != AssetType::Invalid && handle != AssetHandle(0);
        }

        if (!foundInAssetRegistry)
        {
            metadata.filepath = filepath;
            metadata.type = GetAssetTypeFromExtension(fileExtension);
        }

        // Invalid 
        if (metadata.type == AssetType::Invalid)
        {
            LOG_ERROR("[Asset Manager] Invalid asset type '{}'", filepath.generic_string());
            return AssetHandle(0);
        }

        // Find in registered asset first
        if (!foundInAssetRegistry)
        {
            for (const auto &[assetHandle, assetMetaData] : m_AssetRegistry)
            {
                if (metadata.filepath == assetMetaData.filepath)
                {
                    // found it
                    handle = assetHandle;
                    metadata = assetMetaData;

                    foundInAssetRegistry = true;
                    break;
                }
            }
        }

        // generate handle for new asset
        if (!foundInAssetRegistry)
        {
            handle = AssetHandle();
            Import(handle, metadata);
            m_AssetRegistry[handle] = metadata;
        }
        else
        {
            // get the asset
            Ref<Asset> asset = GetAsset(handle);
            if (!asset)
            {
                m_AssetRegistry[handle] = metadata;
                m_LoadedAssets[handle] = asset;
            }
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

        const AssetMetaData &metadata = GetMetaData(handle);
        return Import(handle, metadata);
    }

    AssetType AssetManager::GetAssetType(AssetHandle handle)
    {
        return GetMetaData(handle).type;
    }

    const AssetMetaData &AssetManager::GetMetaData(const std::filesystem::path &filepath, AssetHandle &outHandle)
    {
        for (const auto &[handle, metadata] : m_AssetRegistry)
        {
            if (metadata.filepath == filepath)
            {
                // found it
                outHandle = handle;
                return metadata;
            }
        }
        return s_NullMetaData;
    }

    const AssetMetaData &AssetManager::GetMetaData(AssetHandle handle) const
    {
        if (m_AssetRegistry.contains(handle))
        {
            return m_AssetRegistry.at(handle);
        }

        return s_NullMetaData;
    }

    AssetHandle AssetManager::GetAssetHandle(const std::filesystem::path& filepath)
    {
        for (const auto &[handle, metadata] : m_AssetRegistry)
        {
            if (metadata.filepath == filepath)
                return handle;
        }
        return AssetHandle(0);
    }

    const std::filesystem::path &AssetManager::GetFilepath(AssetHandle handle)
    {
        return GetMetaData(handle).filepath;
    }

    bool AssetManager::IsAssetHandleValid(AssetHandle handle)
    {
        return (uint64_t)handle != 0 && m_AssetRegistry.contains(handle);
    }

    Ref<Asset> AssetManager::Import(AssetHandle handle, const AssetMetaData &metadata)
    {
        if (m_LoadedAssets.contains(handle))
        {
            return m_LoadedAssets[handle];
        }

        Ref<Asset> asset;
        switch (metadata.type)
        {
        case AssetType::Invalid:
        {
            LOG_ERROR("[Asset Manager] Invalid asset type!");
            return nullptr;
        }

        case AssetType::Scene:
        case AssetType::Texture:
        {
            asset = AssetImporter::Import(handle, metadata);
            m_LoadedAssets[handle] = asset;
            break;
        }
        }
        return asset;
    }

}