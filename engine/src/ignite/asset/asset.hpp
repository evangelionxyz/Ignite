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
        Audio,
        Model,
        Project,
        Texture,
        TextureCube,
        SkeletalAnimation,
        Anim2D,
        Skeleton,
        MeshSource, // Mesh Source (contains vertices, indices, etc...)
        Mesh, // Mesh Asset
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
            case ignite::AssetType::TextureCube: return "TextureCube";
            case ignite::AssetType::Scene: return "Scene";
            case ignite::AssetType::SkeletalAnimation: return "SkeletalAnimation";
            case ignite::AssetType::Anim2D: return "Anim2D";
            case ignite::AssetType::MeshSource: return "MeshSource";
            case ignite::AssetType::Mesh: return "Mesh";
            case ignite::AssetType::Skeleton: return "Skeleton";
            case ignite::AssetType::Invalid:
            default: return "Invalid";
        }
    }

    static std::map<std::string, AssetType> s_AssetExtensionMap =
    {
        { ".ixproj", AssetType::Project },
        { ".ixscene", AssetType::Scene },
        { ".jpg", AssetType::Texture },
        { ".png", AssetType::Texture },
        { ".jpeg", AssetType::Texture },
        { ".hdr", AssetType::TextureCube },
        { ".mp3", AssetType::Audio },
        { ".flac", AssetType::Audio },
        { ".wav", AssetType::Audio },
        { ".fbx", AssetType::MeshSource },
        { ".glb", AssetType::MeshSource },
        { ".gltf", AssetType::MeshSource },
    };

    static AssetType AssetTypeFromString(const std::string &typeStr)
    {
        if (typeStr == "Scene") return AssetType::Scene;
        if (typeStr == "Texture") return AssetType::Texture;
        if (typeStr == "TextureCube") return AssetType::TextureCube;
        if (typeStr == "Audio") return AssetType::Audio;
        if (typeStr == "Project") return AssetType::Project;
        if (typeStr == "Model") return AssetType::Model;
        if (typeStr == "SkeletalAnimation") return AssetType::SkeletalAnimation;
        if (typeStr == "Anim2D")  return AssetType::Anim2D;
        if (typeStr == "Mesh")  return AssetType::Mesh;
        if (typeStr == "MeshSource")  return AssetType::MeshSource;
        if (typeStr == "Skeleton")  return AssetType::Skeleton;
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
