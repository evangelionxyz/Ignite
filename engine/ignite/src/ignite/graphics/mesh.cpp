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

        if (scene == nullptr)
            return;

        // load animation here

        // count vertices and indices
        m_Meshes.resize(scene->mNumMeshes);

        for (size_t i = 0; i < m_Meshes.size(); ++i)
        {
            // create meshes
            m_Meshes[i] = CreateRef<Mesh>(i); // i = mesh ID
            m_Meshes[i]->parentID = -1;
        }

        ModelLoader::ProcessNode(scene, scene->mRootNode, filepath.generic_string(), m_Meshes);

        // create constant buffer
        auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(PushConstantMesh))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
            .setMaxVersions(16);

        modelConstantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(modelConstantBuffer, "[Model] Failed to create model constant buffer");

        // create constant buffer
        constantBufferDesc.setByteSize(sizeof(PushConstantMaterial));

        materialConstantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(materialConstantBuffer, "[Model] Failed to create material constant buffer");
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

        for (size_t i = 0; i < m_Meshes.size(); ++i)
        {
            auto &mesh = m_Meshes[i];

            // write material constant buffer
            PushConstantMaterial materialPushConstant;
            materialPushConstant.baseColor = mesh->material.baseColor;
            materialPushConstant.diffuseColor = mesh->material.diffuseColor;
            materialPushConstant.emissive = mesh->material.emissive;
            commandList->writeBuffer(materialConstantBuffer, &materialPushConstant, sizeof(PushConstantMaterial));

            // write model constant buffer
            PushConstantMesh modelPushConstant;

            static float rot = 0.0f;
            rot += 10.0f * Application::GetDeltaTime();

            modelPushConstant.transformMatrix = glm::rotate(glm::radians(rot), glm::vec3 { 0.0f, 1.0f, 0.0f });

            if (mesh->parentID != -1)
            {
                modelPushConstant.transformMatrix *= m_Meshes[mesh->parentID]->localTransform * mesh->localTransform;
            }
            else
            {
                modelPushConstant.transformMatrix *= mesh->localTransform;
            }

            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(modelPushConstant.transformMatrix)));
            modelPushConstant.normalMatrix = glm::mat4(normalMat3);
            commandList->writeBuffer(modelConstantBuffer, &modelPushConstant, sizeof(PushConstantMesh));

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
    void ModelLoader::ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, i32 parentID)
    {
        //glm::mat4 meshTransform = scene->mNumAnimations == 0 ? Math::AssimpToGlmMatrix(node->mTransformation) * transform : glm::mat4(1.0f);
        glm::mat4 meshTransform = scene->mNumAnimations == 0 ? Math::AssimpToGlmMatrix(node->mTransformation) : glm::mat4(1.0f);

        // store the starting index for meshes created in this node
        i32 lastMeshIndexInThisNode = -1;

        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            i32 meshIndex = node->mMeshes[i];
            aiMesh *mesh = scene->mMeshes[meshIndex];

            meshes[meshIndex]->localTransform = meshTransform;
            meshes[meshIndex]->parentID = parentID;

            LoadSingleMesh(scene, meshIndex, mesh, filepath, meshes);

            // update the last mesh index
            lastMeshIndexInThisNode = meshIndex;
        }

        i32 newParentID = lastMeshIndexInThisNode != -1 ? lastMeshIndexInThisNode : parentID;

        for (u32 i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(scene, node->mChildren[i], filepath, meshes, newParentID);
        }
    }

    void ModelLoader::LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const std::string &filepath, std::vector<Ref<Mesh>> &meshes)
    {
        meshes[meshIndex]->name = mesh->mName.C_Str();

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
            aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
            LoadMaterial(mat, meshes[meshIndex]);
        }

        meshes[meshIndex]->CreateBuffers();
    }

    void ModelLoader::LoadMaterial(aiMaterial *material, Ref<Mesh> &mesh)
    {
        aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D diffuse_color(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D emmisive_color(0.0f, 0.0f, 0.0f, 0.0f);
        f32 reflectivity = 0.0f;

        material->Get(AI_MATKEY_BASE_COLOR, base_color);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);
        // material->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.buffer_data.rougness);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emmisive_color);
        material->Get(AI_MATKEY_REFLECTIVITY, reflectivity);

        mesh->material.baseColor = { base_color.r, base_color.g, base_color.b, 1.0f };
        mesh->material.diffuseColor = { diffuse_color.r, diffuse_color.g, diffuse_color.b, 1.0f };
        mesh->material.emissive = emmisive_color.r / diffuse_color.r;

        // TODO: load textures
        // material.diffuse_texture = LoadTexture(m_Scene, material, filepath, TextureType::DIFFUSE);
        // material.specular_texture = LoadTexture(m_Scene, material, filepath, TextureType::SPECULAR);
        // material.roughness_texture = LoadTexture(m_Scene, material, filepath, TextureType::DIFFUSE_ROUGHNESS);

        // set transparent and reflectivity
        mesh->material._transparent = false;
        mesh->material._reflective = reflectivity > 0.0f;
    }

}