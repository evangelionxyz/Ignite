#include "asset_manager.hpp"
#include "asset_importer.hpp"

#include "ignite/core/logger.hpp"
#include <cstdint>

namespace ignite {

    static std::map<const char *, AssetType> s_AssetExtensionMap = 
    {
        { ".ixproj", AssetType::Project },
        { ".ixs", AssetType::Scene },
        { ".jpg", AssetType::Texture },
        { ".hdr", AssetType::Texture },
        { ".png", AssetType::Texture },
        { ".jpeg", AssetType::Texture },
        { ".mp3", AssetType::Audio },
        { ".wav", AssetType::Audio },
        { ".fbx", AssetType::Audio },
        { ".glb", AssetType::Audio },
        { ".gltf", AssetType::Audio },
    };

    static AssetType GetAssetTypeFromExtension(const char *ext)
    {
        return s_AssetExtensionMap.at(ext);
    }

    AssetHandle AssetManager::ImportAsset(const std::filesystem::path &filepath)
    {
        AssetHandle handle;
        
        // create metadata
        AssetMetaData metadata;
        metadata.filepath = filepath;
        metadata.type = GetAssetTypeFromExtension(metadata.filepath.extension().generic_string().c_str());

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

    void AssetManager::RemoveAsset(AssetHandle handle)
    {

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