#pragma once

#include "vertex_data.hpp"

namespace ignite {

    struct MeshFactory
    {
        static std::array<VertexMesh, 24> CubeVertices;
        static std::array<uint32_t, 36> CubeIndices;
    };
}