#include "component.hpp"

#include "ignite/graphics/mesh.hpp"

namespace ignite
{
    MeshRenderer::MeshRenderer(const MeshRenderer& other)
        : IComponent(other)
    {
        if (!other.mesh)
            return;

        mesh = CreateRef<Mesh>(*other.mesh.get());
        cullMode = other.cullMode;
        fillMode = other.fillMode;

        meshBuffer = other.meshBuffer;
        meshSource = other.meshSource;
        meshIndex = other.meshIndex;
        root = other.root;
    }
}
