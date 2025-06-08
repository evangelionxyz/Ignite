#pragma once

#include "asset.hpp"

#include "ignite/graphics/mesh.hpp"
#include "ignite/graphics/vertex_data.hpp"

#include "ignite/animation/skeletal_animation.hpp"

namespace ignite
{
    // Storage
    class MeshSource : public Asset
    {
    public:
        std::vector<MeshData> meshes;
        std::vector<Material> materials;
        std::vector<SkeletalAnimation> animations;

        static AssetType GetStaticType() { return AssetType::MeshSource; }
        virtual AssetType GetType() override { return GetStaticType(); }
    };
}