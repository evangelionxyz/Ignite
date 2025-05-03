#pragma once

#include "ignite/asset/asset.hpp"

#include <filesystem>

namespace ignite {

    enum class MeshType
    {
        Static,
        Skeletal
    };

    struct MeshCreateInfo
    {
        MeshType type = MeshType::Static;
    };

    class Mesh : public Asset
    {
    public:
        Mesh(const std::filesystem::path &filepath, const MeshCreateInfo &createInfo = MeshCreateInfo());

    private:
        MeshCreateInfo m_CreateInfo;
    };

}