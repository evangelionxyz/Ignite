#include "mesh.hpp"

#include "renderer.hpp"
#include "texture.hpp"
#include "lighting.hpp"
#include "ignite/math/math.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/application.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"

#include <queue>
#include <stb_image.h>

namespace ignite {

    static std::unordered_map<std::string, nvrhi::TextureHandle> textureCache;

    void Mesh::CreateConstantBuffers(nvrhi::IDevice *device)
    {
        // create buffers
        auto constantBufferDesc = nvrhi::BufferDesc()
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(16)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer);

        // create per mesh constant buffers
        constantBufferDesc.setDebugName("Mesh constant buffer");
        constantBufferDesc.setByteSize(sizeof(ObjectBuffer));
        objectBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(objectBuffer, "[Model] Failed to create object constant buffer");

        constantBufferDesc.setDebugName("Material constant buffer");
        constantBufferDesc.setByteSize(sizeof(MaterialData));
        materialBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(materialBuffer, "[Model] Failed to create material constant buffer");
    }

    // Mesh class
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

    // Mesh loader
    void MeshLoader::ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, std::vector<NodeInfo> &nodes, const Skeleton &skeleton, i32 parentNodeID)
    {
        // Create a node entry and get its index
        NodeInfo nodeInfo;
        nodeInfo.localTransform = Math::AssimpToGlmMatrix(node->mTransformation);
        nodeInfo.parentID = parentNodeID;
        nodeInfo.name = node->mName.C_Str();
        i32 currentNodeID = nodes.size();

        nodes.push_back(nodeInfo);

        // If parent exists, add this node as a child
        if (parentNodeID != -1)
        {
            nodes[parentNodeID].childrenIDs.push_back(currentNodeID);
        }

        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            i32 meshIndex = node->mMeshes[i];
            aiMesh *mesh = scene->mMeshes[meshIndex];

            if (nodeInfo.parentID != -1)
            {
                // Go up 
                NodeInfo parentNode = nodes[nodeInfo.parentID];
                auto it = skeleton.nameToJointMap.find(parentNode.name);
                if (it != skeleton.nameToJointMap.end())
                {
                    meshes[meshIndex]->nodeID = nodeInfo.parentID;
                }
            }

            // Store mesh index in the node
            nodes[currentNodeID].meshIndices.push_back(meshIndex);

            LoadSingleMesh(scene, meshIndex, mesh, filepath, meshes, skeleton);
        }

        // Process all children with this node as parent
        for (u32 i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(scene, node->mChildren[i], filepath, meshes, nodes, skeleton, currentNodeID);
        }
    }
    
    void MeshLoader::LoadSingleMesh(const aiScene *scene, const uint32_t meshIndex, aiMesh *mesh, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, const Skeleton &skeleton)
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

            // Initialize boneIDs and weights to default values
            for (int j = 0; j < VERTEX_MAX_BONES; ++j)
            {
                vertex.boneIDs[j] = 0;
                vertex.weights[j] = 0.0f;
            }

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

        // Load bones
        if (mesh->HasBones())
        {
            ProcessBodeWeights(mesh, meshes[meshIndex]->vertices, skeleton);
        }

        if (mesh->mMaterialIndex >= 0)
        {
            // TODO: Load material here
            aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
            LoadMaterial(scene, mat, filepath, meshes[meshIndex]);
        }

        meshes[meshIndex]->CreateBuffers();
    }

    void MeshLoader::ProcessBodeWeights(aiMesh *mesh, std::vector<VertexMesh> &vertices, const Skeleton &skeleton)
    {
        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            aiBone *bone = mesh->mBones[boneIndex];
            std::string boneName = bone->mName.C_Str();

            // Get bone ID from skeleton
            auto it = skeleton.nameToJointMap.find(boneName);
            if (it == skeleton.nameToJointMap.end())
            {
                LOG_WARN("[Model Loader]: Bone {} not found in skeleton!", boneName);
                continue;
            }

            uint32_t boneId = it->second;

            // Each vertex can be affected by multimple bones
            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
            {
                uint32_t vertexId = bone->mWeights[weightIndex].mVertexId;
                float weight = bone->mWeights[weightIndex].mWeight;

                // Find the first empty slot in this vertex's bone array
                for (uint32_t j = 0; j < VERTEX_MAX_BONES; ++j)
                {
                    if (vertices[vertexId].weights[j] < 0.00001f)
                    {
                        vertices[vertexId].boneIDs[j] = boneId;
                        vertices[vertexId].weights[j] = weight;
                        break;
                    }
                }
            }
        }

        // Normalize weights to ensure the sume to 1.0
        for (auto &vertex : vertices)
        {
            float totalWeight = 0.0f;
            for (uint32_t i = 0; i < VERTEX_MAX_BONES; ++i)
            {
                totalWeight += vertex.weights[i];
            }

            if (totalWeight > 0.0f)
            {
                for (int i = 0; i < VERTEX_MAX_BONES; ++i)
                {
                    vertex.weights[i] /= totalWeight;
                }
            }
        }
    }

    void MeshLoader::ExtractSkeleton(const aiScene *scene, Skeleton &skeleton)
    {
        // count the number of bones
        std::unordered_set<std::string> uniqueBoneNames;
        for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh *mesh = scene->mMeshes[m];
            for (uint32_t b = 0; b < mesh->mNumBones; ++b)
            {
                uniqueBoneNames.insert(mesh->mBones[b]->mName.C_Str());
            }
        }

        skeleton.joints.reserve(uniqueBoneNames.size());

        // create joints map and collect inverse bind matrices
        std::unordered_map<std::string, glm::mat4> inverseBindMatrices;
        for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh *mesh = scene->mMeshes[m];
            for (uint32_t b = 0; b < mesh->mNumBones; ++b)
            {
                aiBone *bone = mesh->mBones[b];
                std::string boneName = bone->mName.C_Str();
                inverseBindMatrices[boneName] = Math::AssimpToGlmMatrix(bone->mOffsetMatrix);
            }
        }

        // Find all nodes related to the skeleton
        ExtractSkeletonRecursive(scene->mRootNode, -1, skeleton, inverseBindMatrices);
        
    }

    void MeshLoader::ExtractSkeletonRecursive(aiNode *node, i32 parentJointId, Skeleton &skeleton, const std::unordered_map<std::string, glm::mat4> &inverseBindMatrices)
    {
        std::string nodeName = node->mName.C_Str();
        bool isJoint = inverseBindMatrices.contains(nodeName);

        i32 currentJointId = -1;

        if (isJoint)
        {
            // Add this node as a joint
            Joint joint;
            joint.name = nodeName;
            joint.id = skeleton.joints.size();
            joint.parentJointId = parentJointId;
            joint.inverseBindPose = inverseBindMatrices.at(nodeName);
            joint.localTransform = Math::AssimpToGlmMatrix(node->mTransformation);

            currentJointId = joint.id;
            skeleton.nameToJointMap[nodeName] = currentJointId;
            skeleton.joints.push_back(joint);
        }

        // process childe (use parent id if this node is not a joint)
        i32 childParentId = isJoint ? currentJointId : parentJointId;
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            ExtractSkeletonRecursive(node->mChildren[i], childParentId, skeleton, inverseBindMatrices);
        }
    }

    void MeshLoader::SortJointsHierchically(Skeleton &skeleton)
    {
        std::vector<Joint> sortedJoints;
        sortedJoints.reserve(skeleton.joints.size());

        // use a queue to process joints level by level
        std::queue<i32> queue;

        // start with root joints
        for (size_t i = 0; i < skeleton.joints.size(); ++i)
        {
            if (skeleton.joints[i].parentJointId == -1)
                queue.push(i);
        }

        // BFS traversal to ensure parents are processed before children
        while (!queue.empty())
        {
            i32 jointIdx = queue.front();
            queue.pop();

            sortedJoints.push_back(skeleton.joints[jointIdx]);
            i32 newIdx = sortedJoints.size() - 1;

            // Update joint indices in the new array
            if (sortedJoints[newIdx].parentJointId != -1)
            {
                // Find new parent index
                std::string parentName = skeleton.joints[sortedJoints[newIdx].parentJointId].name;
                for (size_t j = 0; j < newIdx; ++j)
                {
                    if (sortedJoints[j].name == parentName)
                    {
                        sortedJoints[newIdx].parentJointId = j;
                        break;
                    }
                }
            }

            // Add children to queue
            for (size_t i = 0; i < skeleton.joints.size(); ++i)
            {
                if (skeleton.joints[i].parentJointId == jointIdx)
                {
                    queue.push(i);
                }
            }
        }

        // Update name to joint name
        skeleton.nameToJointMap.clear();
        for (size_t i = 0; i < sortedJoints.size(); ++i)
        {
            sortedJoints[i].id = i;
            skeleton.nameToJointMap[sortedJoints[i].name] = i;
        }

        skeleton.joints = std::move(sortedJoints);

    }

    void MeshLoader::LoadMaterial(const aiScene *scene, aiMaterial *material, const std::string &filepath, Ref<Mesh> &mesh)
    {
        aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D diffuse_color(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D emmisive_color(0.0f, 0.0f, 0.0f, 0.0f);
        f32 reflectivity = 0.0f;

        material->Get(AI_MATKEY_BASE_COLOR, base_color);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);
        material->Get(AI_MATKEY_COLOR_EMISSIVE, emmisive_color);
        material->Get(AI_MATKEY_METALLIC_FACTOR, mesh->material.data.metallic);
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, mesh->material.data.roughness);
        material->Get(AI_MATKEY_REFLECTIVITY, reflectivity);
        mesh->material.data.baseColor = { base_color.r, base_color.g, base_color.b, 1.0f };
        
        if (diffuse_color.r > 0.0f)
            mesh->material.data.emissive = emmisive_color.r / diffuse_color.r;

        // load textures
        LoadTextures(scene, material, &mesh->material, aiTextureType_DIFFUSE);
        LoadTextures(scene, material, &mesh->material, aiTextureType_SPECULAR);
        LoadTextures(scene, material, &mesh->material, aiTextureType_EMISSIVE);
        LoadTextures(scene, material, &mesh->material, aiTextureType_DIFFUSE_ROUGHNESS);
        LoadTextures(scene, material, &mesh->material, aiTextureType_NORMALS);

        // set transparent and reflectivity
        mesh->material._transparent = false;
        mesh->material._reflective = reflectivity > 0.0f;
    }

    void MeshLoader::LoadAnimation(const aiScene *scene, std::vector<Ref<SkeletalAnimation>> &animations)
    {
        animations.resize(scene->mNumAnimations);

        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            aiAnimation *anim = scene->mAnimations[i];
            animations[i] = CreateRef<SkeletalAnimation>(anim);
        }
    }

    void MeshLoader::LoadTextures(const aiScene *scene, aiMaterial *material, Material *meshMaterial, aiTextureType type)
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

                    stbi_set_flip_vertically_on_load(false);

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
                            dstData[i * 4 + 3] = 255;                // A
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
        {
            meshMaterial->textures[type].handle = Renderer::GetWhiteTexture()->GetHandle();
        }

        meshMaterial->sampler = Renderer::GetWhiteTexture()->GetSampler();

    }

    void MeshLoader::CalculateWorldTransforms(std::vector<NodeInfo> &nodes, std::vector<Ref<Mesh>> &meshes)
    {
        // First pass: calculate world transforms for nodes
        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (nodes[i].parentID == -1)
            {
                // Root node
                nodes[i].worldTransform = nodes[i].localTransform;
            }
            else
            {
                // Child node
                nodes[i].worldTransform = nodes[nodes[i].parentID].worldTransform * nodes[i].localTransform;
                
            }

            // Apply node's world transform to all its meshes
            for (i32 meshIdx : nodes[i].meshIndices)
            {
                meshes[meshIdx]->worldTransform = nodes[i].worldTransform;
            }
        }
    }

    void MeshLoader::ClearTextureCache()
    {
        textureCache.clear();
    }

}