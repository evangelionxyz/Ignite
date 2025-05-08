#include "mesh.hpp"

#include "renderer.hpp"
#include "texture.hpp"
#include "ignite/math/math.hpp"
#include "ignite/core/logger.hpp"

#include "ignite/core/application.hpp"

#define ASSIMP_IMPORTER_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

namespace ignite {

    // Mesh class
    Mesh::Mesh(i32 id)
        : meshID(id)
    {
    }

    void Mesh::CreateBuffers()
    {
        nvrhi::IDevice *device = Application::GetDeviceManager()->GetDevice();

        // create vertex buffer
        nvrhi::BufferDesc vbDesc = nvrhi::BufferDesc();
        vbDesc.isVertexBuffer = true;
        vbDesc.byteSize = sizeof(VertexMesh) * vertices.size();
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;
        vbDesc.debugName = "[Mesh] vertex buffer";

        vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(vertexBuffer, "[Mesh] Failed to create Vertex Buffer");


        // create index buffer
        nvrhi::BufferDesc ibDesc = nvrhi::BufferDesc();
        ibDesc.isIndexBuffer = true;
        ibDesc.byteSize = sizeof(uint32_t) * indices.size();
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;
        ibDesc.debugName = "[Mesh] index buffer";

        indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(indexBuffer, "[Mesh] Failed to create Index Buffer");
    }

    // Model class

    Model::Model(nvrhi::IDevice *device, const std::filesystem::path &filepath)
    {
        LOG_ASSERT(std::filesystem::exists(filepath), "[Model] Filepath does not exists!");

        static Assimp::Importer importer;

        const aiScene *scene = importer.ReadFile(filepath.generic_string(), ASSIMP_IMPORTER_FLAGS);

        LOG_ASSERT(scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode,
            "[Model] Failed to load {}: {}", filepath, importer.GetErrorString());

        // load animation here

        // count vertices and indices
        m_Meshes.resize(scene->mNumMeshes);

        uint32_t numVertices = 0;
        uint32_t numIndices = 0;

        for (size_t i = 0; i < m_Meshes.size(); ++i)
        {
            numVertices += scene->mMeshes[i]->mNumVertices;
            numIndices += scene->mMeshes[i]->mNumFaces * 3;

            // create meshes
            m_Meshes[i] = CreateRef<Mesh>(i); // i = mesh ID
        }

        ModelLoader::ProcessNode(scene, scene->mRootNode, glm::mat4(1.0f), filepath.generic_string(), m_Meshes);

        // create constant buffer
        const auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(PushConstantGlobal))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
            .setMaxVersions(16);

        constantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(constantBuffer, "[Model] Failed to create constant buffer");
    }


    void Model::WriteBuffer(nvrhi::CommandListHandle commandList)
    {
        for (auto &mesh : m_Meshes)
        {
            commandList->writeBuffer(mesh->vertexBuffer, mesh->vertices.data(), sizeof(VertexMesh) * mesh->vertices.size());
            commandList->writeBuffer(mesh->indexBuffer, mesh->indices.data(), sizeof(uint32_t) * mesh->indices.size());
        }
    }

    void Model::OnUpdate(f32 deltaTime)
    {

    }

    void Model::Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, nvrhi::BindingSetHandle bindingSet)
    {
        nvrhi::Viewport viewport = framebuffer->getFramebufferInfo().getViewport();

        for (auto &mesh : m_Meshes)
        {
            // write push constant
            PushConstantMesh pushConstant;

            static float rotatex = 0.0f;
            rotatex += 1.0f * 0.05f;
            pushConstant.transformMatrix = mesh->localTransform * glm::rotate(glm::radians(rotatex), glm::vec3{ 1.0f, 0.0f, 0.0f });

            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(pushConstant.transformMatrix)));
            pushConstant.normalMatrix = glm::mat4(normalMat3);
            commandList->writeBuffer(constantBuffer, &pushConstant, sizeof(PushConstantMesh));

            // render
            auto meshGraphicsState = nvrhi::GraphicsState();
            meshGraphicsState.pipeline = pipeline;
            meshGraphicsState.framebuffer = framebuffer;
            meshGraphicsState.viewport = nvrhi::ViewportState().addViewportAndScissorRect(viewport);
            meshGraphicsState.addVertexBuffer({ mesh->vertexBuffer, 0, 0 });
            meshGraphicsState.indexBuffer = { mesh->indexBuffer, nvrhi::Format::R32_UINT };

            if (bindingSet != nullptr)
                meshGraphicsState.addBindingSet(bindingSet);

            commandList->setGraphicsState(meshGraphicsState);

            nvrhi::DrawArguments args;
            args.setVertexCount((uint32_t)mesh->indices.size());
            args.instanceCount = 1;

            commandList->drawIndexed(args);
        }
    }

    // Mesh loader
    void ModelLoader::ProcessNode(const aiScene *scene, aiNode *node, const glm::mat4 &transform, const std::string &filepath, std::vector<Ref<Mesh>> &meshes)
    {
        glm::mat4 meshTransform = scene->mNumAnimations == 0 ? Math::AssimpToGlmMatrix(node->mTransformation) * transform : glm::mat4(1.0f);

        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            i32 meshIndex = node->mMeshes[i];
            aiMesh *mesh = scene->mMeshes[meshIndex];

            LoadSingleMesh(scene, meshIndex, mesh, meshTransform, filepath, meshes);
        }

        for (u32 i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(scene, node->mChildren[i], meshTransform, filepath, meshes);
        }
    }

    void ModelLoader::LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const glm::mat4 &transform, const std::string &filepath, std::vector<Ref<Mesh>> &meshes)
    {
        meshes[meshIndex]->name = mesh->mName.C_Str();
        meshes[meshIndex]->localTransform = transform;

        // vertices;
        VertexMesh vertex;
        meshes[meshIndex]->vertices.resize(mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

            if (mesh->HasNormals())
                vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            else 
                vertex.normal = { 0.0f, 1.0f, 0.0f }; // default normals

            if (mesh->mTextureCoords[0])
                vertex.texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            else
                vertex.texCoord = { 0.0f, 0.0f };

            meshes[meshIndex]->vertices[i] = vertex;
        }

        meshes[meshIndex]->indices.reserve(mesh->mNumFaces * 3);
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            meshes[meshIndex]->indices.push_back(face.mIndices[0]);
            meshes[meshIndex]->indices.push_back(face.mIndices[1]);
            meshes[meshIndex]->indices.push_back(face.mIndices[2]);
        }

        if (scene->HasAnimations())
        {
            // TODO: Load bones here
        }

        if (mesh->mMaterialIndex >= 0)
        {
            // TODO: Load material here
        }

        meshes[meshIndex]->CreateBuffers();
    }

}