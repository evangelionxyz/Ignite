#include "mesh.hpp"

#include "renderer.hpp"
#include "texture.hpp"
#include "ignite/math/math.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"

#include <stb_image.h>

#define ASSIMP_IMPORTER_FLAGS (aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices)

namespace ignite {

    static std::unordered_map<std::string, nvrhi::TextureHandle> textureCache;

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

    Model::Model(nvrhi::IDevice *device, nvrhi::BindingLayoutHandle bindingLayout, const std::filesystem::path &filepath)
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

        // create global constant buffer
        auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(PushConstantGlobal))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
            .setMaxVersions(16);

        constantBufferDesc.setDebugName("Global constant buffer");
        globalConstantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(globalConstantBuffer, "Failed to create constant buffer");


        for (auto &mesh : m_Meshes)
        {
            // create per mesh constant buffers
            constantBufferDesc.setDebugName("Mesh constant buffer");
            constantBufferDesc.setByteSize(sizeof(PushConstantMesh));
            constantBufferDesc.setMaxVersions(16);
            mesh->modelConstantBuffer = device->createBuffer(constantBufferDesc);
            LOG_ASSERT(mesh->modelConstantBuffer, "[Model] Failed to create model constant buffer");

            constantBufferDesc.setDebugName("Material constant buffer");
            constantBufferDesc.setByteSize(sizeof(PushConstantMaterial));
            constantBufferDesc.setMaxVersions(16);
            mesh->materialConstantBuffer = device->createBuffer(constantBufferDesc);
            LOG_ASSERT(mesh->materialConstantBuffer, "[Model] Failed to create material constant buffer");

            // create per mesh binding sets
            auto bsDesc = nvrhi::BindingSetDesc();
            bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, globalConstantBuffer));
            bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, mesh->modelConstantBuffer));
            bsDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, mesh->materialConstantBuffer));
            bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, mesh->material.textures[aiTextureType_DIFFUSE].handle));
            bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(1, mesh->material.textures[aiTextureType_SPECULAR].handle));
            bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(2, mesh->material.textures[aiTextureType_EMISSIVE].handle));
            bsDesc.addItem(nvrhi::BindingSetItem::Texture_SRV(3, mesh->material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle));

            bsDesc.addItem(nvrhi::BindingSetItem::Sampler(0, mesh->material.sampler));

            mesh->bindingSet = device->createBindingSet(bsDesc, bindingLayout);
            
            LOG_ASSERT(mesh->bindingSet, "Failed to create binding set");
        }
    }

    void Model::WriteBuffer(nvrhi::CommandListHandle commandList)
    {
        for (auto &mesh : m_Meshes)
        {
            commandList->writeBuffer(mesh->vertexBuffer, mesh->vertices.data(), sizeof(VertexMesh) * mesh->vertices.size());
            commandList->writeBuffer(mesh->indexBuffer, mesh->indices.data(), sizeof(uint32_t) * mesh->indices.size());

            // write textures
            if (mesh->material.ShouldWriteTexture())
            {
                mesh->material.WriteBuffer(commandList);
            }
        }
    }

    void Model::OnUpdate(f32 deltaTime)
    {

    }

    void Model::Render(nvrhi::CommandListHandle commandList, nvrhi::IFramebuffer *framebuffer, nvrhi::GraphicsPipelineHandle pipeline, Camera *camera)
    {
        nvrhi::Viewport viewport = framebuffer->getFramebufferInfo().getViewport();

        // write global push constant
        PushConstantGlobal globalPushConstant;
        globalPushConstant.viewProjection = camera->GetViewProjectionMatrix();
        globalPushConstant.cameraPosition = glm::vec4(camera->position, 1.0f);
        commandList->writeBuffer(globalConstantBuffer, &globalPushConstant, sizeof(PushConstantGlobal));

        for (size_t i = 0; i < m_Meshes.size(); ++i)
        {
            auto &mesh = m_Meshes[i];

            // write material constant buffer
            PushConstantMaterial materialPushConstant;
            materialPushConstant.baseColor = mesh->material.baseColor;
            materialPushConstant.diffuseColor = mesh->material.diffuseColor;
            materialPushConstant.emissive = mesh->material.emissive;
            commandList->writeBuffer(mesh->materialConstantBuffer, &materialPushConstant, sizeof(PushConstantMaterial));

            // write model constant buffer

            PushConstantMesh modelPushConstant;
            if (mesh->parentID != -1)
                modelPushConstant.transformMatrix = transform * m_Meshes[mesh->parentID]->localTransform * mesh->localTransform;
            else
                modelPushConstant.transformMatrix = transform * mesh->localTransform;

            glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(modelPushConstant.transformMatrix)));
            modelPushConstant.normalMatrix = glm::mat4(normalMat3);
            commandList->writeBuffer(mesh->modelConstantBuffer, &modelPushConstant, sizeof(PushConstantMesh));

            // render
            auto meshGraphicsState = nvrhi::GraphicsState();
            meshGraphicsState.pipeline = pipeline;
            meshGraphicsState.framebuffer = framebuffer;
            meshGraphicsState.viewport = nvrhi::ViewportState().addViewportAndScissorRect(viewport);
            meshGraphicsState.addVertexBuffer({ mesh->vertexBuffer, 0, 0 });
            meshGraphicsState.indexBuffer = { mesh->indexBuffer, nvrhi::Format::R32_UINT };

            if (mesh->bindingSet != nullptr)
                meshGraphicsState.addBindingSet(mesh->bindingSet);

            commandList->setGraphicsState(meshGraphicsState);

            nvrhi::DrawArguments args;
            args.setVertexCount((uint32_t)mesh->indices.size());
            args.instanceCount = 1;

            commandList->drawIndexed(args);
        }
    }

    Ref<Model> Model::Create(nvrhi::IDevice *device, nvrhi::BindingLayoutHandle bindingLayout, const std::filesystem::path &filepath)
    {
        return CreateRef<Model>(device, bindingLayout, filepath);
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

            if (parentID != -1)
                meshes[parentID]->children.push_back(meshIndex);

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
            LoadMaterial(scene, mat, filepath, meshes[meshIndex]);
        }

        meshes[meshIndex]->CreateBuffers();
    }

    void ModelLoader::LoadMaterial(const aiScene *scene, aiMaterial *material, const std::string &filepath, Ref<Mesh> &mesh)
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

        // load textures
        LoadTextures(scene, material, &mesh->material, aiTextureType_DIFFUSE);
        LoadTextures(scene, material, &mesh->material, aiTextureType_SPECULAR);
        LoadTextures(scene, material, &mesh->material, aiTextureType_EMISSIVE);
        LoadTextures(scene, material, &mesh->material, aiTextureType_DIFFUSE_ROUGHNESS);

        // set transparent and reflectivity
        mesh->material._transparent = false;
        mesh->material._reflective = reflectivity > 0.0f;
    }

    void ModelLoader::LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type)
    {
        if (const i32 texCount = material->GetTextureCount(type))
        {
            for (i32 i = 0; i < texCount; ++i)
            {
                aiString textureFilepath;
                material->GetTexture(type, i, &textureFilepath);

                // try to load from cache
                for (auto &[path, tex] : textureCache)
                {
                    if (std::strcmp(path.c_str(), textureFilepath.C_Str()) == 0)
                    {
                        meshMaterial->textures[type].handle = tex;
                        break;
                    }
                }

                // create new texture
                const aiTexture *embeddedTexture = scene->GetEmbeddedTexture(textureFilepath.C_Str());
                if (embeddedTexture && meshMaterial->textures[type].handle == nullptr)
                {
                    nvrhi::IDevice *device = Application::GetDeviceManager()->GetDevice();

                    i32 width, height, channels;

                    // handle compressed textures
                    if (embeddedTexture->mHeight == 0)
                    {
                        LOG_INFO("[Material] Loading compressed format texture of size {} bytes", embeddedTexture->mWidth);

                        meshMaterial->textures[type].buffer.Data = stbi_load_from_memory(
                            reinterpret_cast<const stbi_uc *>(embeddedTexture->pcData),
                            embeddedTexture->mWidth, &width, &height, &channels, 4);
                    }
                    else
                    {
                        width = embeddedTexture->mWidth;
                        height = embeddedTexture->mHeight;

                        LOG_INFO("[Material] Loading uncompressed texture of size {}x{}", width, height);

                        // for uncompressed texture, convert to RGBA format for consitent handling
                        const unsigned char *srcData = reinterpret_cast<const unsigned char *>(embeddedTexture->pcData);

                        // Allocate space for RGBA8 data
                        unsigned char *dstData = new unsigned char[width * height * 4];

                        // Assimp embedded uncompressed texture data is usually in RGB format without alpha
                        // You can test with alpha channel (or assume RGB with alpha set to 255)
                        for (int i = 0; i < width * height; ++i)
                        {
                            dstData[i * 4 + 0] = srcData[i * 3 + 0]; // R
                            dstData[i * 4 + 1] = srcData[i * 3 + 1]; // G
                            dstData[i * 4 + 2] = srcData[i * 3 + 2]; // B
                            dstData[i * 4 + 3] = 255;               // A
                        }

                        meshMaterial->textures[type].buffer.Data = dstData;
                    }

                    LOG_ASSERT(meshMaterial->textures[type].buffer.Data, "[Material] Failed to load texture");

                    meshMaterial->textures[type].buffer.Size = width * height * 4;
                    meshMaterial->textures[type].rowPitch = width * 4;

                    // create texture
                    const auto textureDesc = nvrhi::TextureDesc()
                        .setDimension(nvrhi::TextureDimension::Texture2D)
                        .setWidth(width)
                        .setHeight(height)
                        .setFormat(nvrhi::Format::RGBA8_UNORM)
                        .setInitialState(nvrhi::ResourceStates::ShaderResource)
                        .setKeepInitialState(true)
                        .setDebugName("Material embedded Texture");

                    meshMaterial->textures[type].handle = device->createTexture(textureDesc);
                    LOG_ASSERT(meshMaterial->textures[type].handle, "[Material] Failed to create texture!");

                    // store to cache
                    textureCache[textureFilepath.C_Str()] = meshMaterial->textures[type].handle;

                    meshMaterial->_shouldWriteTexture = true;
                }
            }
        }

        if (!meshMaterial->textures[type].handle)
            meshMaterial->textures[type].handle = Renderer::GetWhiteTexture()->GetHandle();

        meshMaterial->sampler = Renderer::GetWhiteTexture()->GetSampler();

    }

    void ModelLoader::ClearTextureCache()
    {
        textureCache.clear();
    }

}