#pragma once

#include "ignite/core/uuid.hpp"

#include <string>
#include <filesystem>
#include <map>
#include <nvrhi/nvrhi.h>

namespace ignite {

    using AssetHandle = UUID;

    enum class AssetType
    {
        Invalid,
        Texture,
        Audio,
        Model,
        Project,
        Environment,
        Scene
    };

    static std::string AssetTypeToString(AssetType type)
    {
        switch (type)
        {
            case ignite::AssetType::Texture: return "Texture";
            case ignite::AssetType::Audio: return "Audio";
            case ignite::AssetType::Model: return "Model";
            case ignite::AssetType::Project: return "Project";
            case ignite::AssetType::Environment: return "Environment";
            case ignite::AssetType::Scene: return "Scene";
            case ignite::AssetType::Invalid:
            default:
                return "Invalid";
        }
    }

    static std::map<std::string, AssetType> s_AssetExtensionMap =
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

    static AssetType AssetTypeFromString(const std::string &typeStr)
    {
        if (typeStr == "Scene")
            return AssetType::Scene;
        else if (typeStr == "Texture")
            return AssetType::Texture;
        else if (typeStr == "Audio")
            return AssetType::Audio;
        else if (typeStr == "Project")
            return AssetType::Project;
        else if (typeStr == "Model")
            return AssetType::Model;
        else if (typeStr == "Environment")
            return AssetType::Environment;
        else
            return AssetType::Invalid;
    }

    static AssetType GetAssetTypeFromExtension(const std::string &ext)
    {
        if (s_AssetExtensionMap.contains(ext))
            return s_AssetExtensionMap.at(ext);

        return AssetType::Invalid;
    }

    struct AssetMetaData
    {
        AssetType type = AssetType::Invalid;
        std::filesystem::path filepath;
    };

    class Asset
    {
    public:
        virtual ~Asset() = default;

        template<typename T>
        T *As()
        {
            return static_cast<T *>(this);
        }

        virtual AssetType GetType() { return AssetType::Invalid; };

        void SetDirtyFlag(bool dirty) 
        { 
            m_IsDirty = dirty; 
        }

        bool IsDirty() const 
        { 
            return m_IsDirty;
        }

        AssetHandle handle;

    protected:
        bool m_IsDirty = true;
    };
}
