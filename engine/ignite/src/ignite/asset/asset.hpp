#pragma once

#include "ignite/core/uuid.hpp"

#include <string>
#include <filesystem>
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

    struct AssetMetaData
    {
        AssetHandle handle = AssetHandle(0);
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
