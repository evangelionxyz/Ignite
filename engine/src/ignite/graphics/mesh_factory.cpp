#include "mesh_factory.hpp"

namespace ignite {

    std::array<VertexMesh, 24> MeshFactory::CubeVertices = {
        // Front face
        VertexMesh{{-0.5f, -0.5f,  0.5f}, { 0.f,  0.f,  1.f}, {0.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f, -0.5f,  0.5f}, { 0.f,  0.f,  1.f}, {1.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f,  0.5f,  0.5f}, { 0.f,  0.f,  1.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f,  0.5f,  0.5f}, { 0.f,  0.f,  1.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},

        // Back face
        VertexMesh{{ 0.5f, -0.5f, -0.5f}, { 0.f,  0.f, -1.f}, {0.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f, -0.5f, -0.5f}, { 0.f,  0.f, -1.f}, {1.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f,  0.5f, -0.5f}, { 0.f,  0.f, -1.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f,  0.5f, -0.5f}, { 0.f,  0.f, -1.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},

        // Left face
        VertexMesh{{-0.5f, -0.5f, -0.5f}, {-1.f,  0.f,  0.f}, {0.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f, -0.5f,  0.5f}, {-1.f,  0.f,  0.f}, {1.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f,  0.5f,  0.5f}, {-1.f,  0.f,  0.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f,  0.5f, -0.5f}, {-1.f,  0.f,  0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},

        // Right face
        VertexMesh{{ 0.5f, -0.5f,  0.5f}, { 1.f,  0.f,  0.f}, {0.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f, -0.5f, -0.5f}, { 1.f,  0.f,  0.f}, {1.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f,  0.5f, -0.5f}, { 1.f,  0.f,  0.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f,  0.5f,  0.5f}, { 1.f,  0.f,  0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},

        // Top face
        VertexMesh{{-0.5f,  0.5f,  0.5f}, { 0.f,  1.f,  0.f}, {0.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f,  0.5f,  0.5f}, { 0.f,  1.f,  0.f}, {1.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f,  0.5f, -0.5f}, { 0.f,  1.f,  0.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f,  0.5f, -0.5f}, { 0.f,  1.f,  0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},

        // Bottom face
        VertexMesh{{-0.5f, -0.5f, -0.5f}, { 0.f, -1.f,  0.f}, {0.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f, -0.5f, -0.5f}, { 0.f, -1.f,  0.f}, {1.f, 0.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{ 0.5f, -0.5f,  0.5f}, { 0.f, -1.f,  0.f}, {1.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
        VertexMesh{{-0.5f, -0.5f,  0.5f}, { 0.f, -1.f,  0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 1.f, 1.f, 1.f}},
    };
    std::array<uint32_t, 36> MeshFactory::CubeIndices = {
        // Front face
        0, 1, 2,  2, 3, 0,
        // Back face
        4, 5, 6,  6, 7, 4,
        // Left face
        8, 9,10, 10,11, 8,
        // Right face
        12,13,14, 14,15,12,
        // Top face
        16,17,18, 18,19,16,
        // Bottom face
        20,21,22, 22,23,20
    };
}