#pragma once

#include "ignite/core/uuid.hpp"

#include <string>
#include <filesystem>
#include <nvrhi/nvrhi.h>

namespace ignite {

    enum class AssetType
    {
        Invalid,
        Texture,
        Audio,
        Model,
        Scene
    };

    struct AssetRegistry
    {
        UUID uuid;
        AssetType type;
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
        virtual void WriteBuffer(nvrhi::CommandListHandle commandList) { };

        void SetDirtyFlag(bool dirty) 
        { 
            m_IsDirty = dirty; 
        }

        bool IsDirty() const 
        { 
            return m_IsDirty;
        }

    protected:
        bool m_IsDirty = true;
    };
}
