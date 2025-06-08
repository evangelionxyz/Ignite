#include "mesh.hpp"

#include "ignite/scene/scene.hpp"
#include "ignite/scene/scene_manager.hpp"

#include "renderer.hpp"
#include "texture.hpp"
#include "lighting.hpp"
#include "ignite/math/math.hpp"
#include "ignite/core/logger.hpp"
#include "ignite/core/application.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"

#include <queue>
#include <stb_image.h>

namespace ignite {

    static std::unordered_map<std::string, nvrhi::TextureHandle> textureCache;

    // Mesh loader
    void MeshLoader::ProcessNode(const aiScene *scene, aiNode *node, const std::string &filepath, std::vector<Ref<Mesh>> &meshes, std::vector<NodeInfo> &nodes, const Ref<Skeleton> &skeleton, i32 parentNodeID)
    {
        // Create a node entry and get its index
        NodeInfo nodeInfo;
        nodeInfo.localTransform = Math::AssimpToGlmMatrix(node->mTransformation);
        nodeInfo.id = nodes.size();
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
            aiMesh *assimpMesh = scene->mMeshes[meshIndex];

            meshes[meshIndex]->name = assimpMesh->mName.C_Str();

            // Set node
            meshes[meshIndex]->nodeID = currentNodeID;

            // Set parent node
            if (nodeInfo.parentID != -1)
            {
                // Go up 
                NodeInfo parentNode = nodes[nodeInfo.parentID];
                auto it = skeleton->nameToJointMap.find(parentNode.name);
                if (it != skeleton->nameToJointMap.end())
                {
                    meshes[meshIndex]->nodeParentID = nodeInfo.parentID;
                }
            }

            // Store mesh index in the node
            nodes[currentNodeID].meshIndices.push_back(meshIndex);

            if (assimpMesh->mMaterialIndex >= 0)
            {
                aiMaterial *mat = scene->mMaterials[assimpMesh->mMaterialIndex];
                LoadMaterial(scene, mat, filepath, meshes[meshIndex]->material);
            }

            LoadSingleMesh(filepath, scene, assimpMesh, meshIndex, meshes[meshIndex]->data, skeleton, meshes[meshIndex]->aabb);

            // Load bones
            if (assimpMesh->HasBones())
            {
                ProcessBoneWeights(assimpMesh, meshes[meshIndex]->data, meshes[meshIndex]->boneInfo, meshes[meshIndex]->boneMapping, skeleton);
            }

            // Copy vertices to outline vertices
            meshes[meshIndex]->outlineVertices.resize(meshes[meshIndex]->data.vertices.size());
            for (size_t i = 0; i < meshes[meshIndex]->data.vertices.size(); ++i)
            {
                VertexMesh mVertex = meshes[meshIndex]->data.vertices[i];
                meshes[meshIndex]->outlineVertices[i].position = mVertex.position;

                for (int w = 0; w < VERTEX_MAX_BONES; ++w)
                {
                    meshes[meshIndex]->outlineVertices[i].boneIDs[w] = mVertex.boneIDs[w];
                    meshes[meshIndex]->outlineVertices[i].weights[w] = mVertex.weights[w];
                }
            }

            meshes[meshIndex]->CreateBuffers();
        }

        // Process all children with this node as parent
        for (u32 i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(scene, node->mChildren[i], filepath, meshes, nodes, skeleton, currentNodeID);
        }
    }

    void MeshLoader::LoadSingleMesh(const std::string &filepath, const aiScene *scene, aiMesh *mesh, const uint32_t meshIndex, MeshData &outMeshData, const Ref<Skeleton> &skeleton, AABB &outAABB)
    {
        // vertices;
        VertexMesh vertex;
        // mesh->outlineVertices.resize(assimpMesh->mNumVertices);
        outMeshData.vertices.resize(mesh->mNumVertices);

        outAABB.min = glm::vec3(FLT_MAX);
        outAABB.max = glm::vec3(-FLT_MAX);

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

            outAABB.min = glm::min(outAABB.min, vertex.position);
            outAABB.max = glm::max(outAABB.max, vertex.position);

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

            outMeshData.vertices[i] = vertex;
        }

        outMeshData.indices.reserve(mesh->mNumFaces * 3);
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            outMeshData.indices.push_back(face.mIndices[0]);
            outMeshData.indices.push_back(face.mIndices[1]);
            outMeshData.indices.push_back(face.mIndices[2]);
        }
    }

    void MeshLoader::ProcessBoneWeights(aiMesh *assimpMesh, MeshData &outMeshData, std::vector<BoneInfo> &outBoneInfo, std::unordered_map<std::string, uint32_t> &outBoneMapping, const Ref<Skeleton> &skeleton)
    {
        outBoneMapping.clear();
        outBoneInfo.resize(skeleton->joints.size());

        // Copy bone offset from skeleton
        for (size_t i = 0; i < skeleton->joints.size(); ++i)
        {
            outBoneInfo[i].offsetMatrix = skeleton->joints[i].inverseBindPose;
            outBoneMapping[skeleton->joints[i].name] = static_cast<i32>(i);
        }

        for (uint32_t boneIndex = 0; boneIndex < assimpMesh->mNumBones; ++boneIndex)
        {
            aiBone *bone = assimpMesh->mBones[boneIndex];
            std::string boneName = bone->mName.C_Str();

            // Get bone ID from skeleton
            auto it = skeleton->nameToJointMap.find(boneName);
            if (it == skeleton->nameToJointMap.end())
            {
                LOG_WARN("[Model Loader]: Bone {} not found in skeleton!", boneName);
                continue;
            }

            uint32_t boneId = it->second;

            // Each vertex can be affected by multiple bones
            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
            {
                uint32_t vertexId = bone->mWeights[weightIndex].mVertexId;
                float weight = bone->mWeights[weightIndex].mWeight;

                // Find the first empty slot in this vertex's bone array
                for (uint32_t j = 0; j < VERTEX_MAX_BONES; ++j)
                {
                    if (outMeshData.vertices[vertexId].weights[j] < 0.00001f)
                    {
                        outMeshData.vertices[vertexId].boneIDs[j] = boneId;
                        outMeshData.vertices[vertexId].weights[j] = weight;
                        break;
                    }
                }
            }
        }

        // Normalize weights to ensure the sum to 1.0
        for (auto &vertex : outMeshData.vertices)
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

    void MeshLoader::ExtractSkeleton(const aiScene *scene, Ref<Skeleton> &skeleton)
    {
        // count the number of joints
        std::unordered_set<std::string> uniqueJointNames;
        for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh *mesh = scene->mMeshes[m];
            for (uint32_t b = 0; b < mesh->mNumBones; ++b)
            {
                uniqueJointNames.insert(mesh->mBones[b]->mName.C_Str());
            }
        }

        skeleton->joints.reserve(uniqueJointNames.size());
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

    void MeshLoader::ExtractSkeletonRecursive(aiNode *node, i32 parentJointId, Ref<Skeleton> &skeleton, const std::unordered_map<std::string, glm::mat4> &inverseBindMatrices)
    {
        std::string nodeName = node->mName.C_Str();
        bool isJoint = inverseBindMatrices.contains(nodeName);
        i32 currentJointId = -1;
        if (isJoint)
        {
            // Add this node as a joint
            Joint joint;
            joint.name = nodeName;
            joint.id = static_cast<i32>(skeleton->joints.size());
            joint.parentJointId = parentJointId;
            joint.inverseBindPose = inverseBindMatrices.at(nodeName);
            joint.localTransform = Math::AssimpToGlmMatrix(node->mTransformation);

            currentJointId = joint.id;
            skeleton->nameToJointMap[nodeName] = currentJointId;
            skeleton->joints.push_back(joint);
        }

        // process child (use parent id if this node is not a joint)
        i32 childParentId = isJoint ? currentJointId : parentJointId;
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            ExtractSkeletonRecursive(node->mChildren[i], childParentId, skeleton, inverseBindMatrices);
        }
    }

    void MeshLoader::SortJointsHierarchically(Ref<Skeleton> &skeleton)
    {
        std::vector<Joint> sortedJoints;
        sortedJoints.reserve(skeleton->joints.size());
        // use a queue to process joints level by level
        std::queue<i32> queue;
        // start with root joints
        for (size_t i = 0; i < skeleton->joints.size(); ++i)
        {
            if (skeleton->joints[i].parentJointId == -1)
                queue.push(i);
        }
        // BFS traversal to ensure parents are processed before children
        while (!queue.empty())
        {
            i32 jointIdx = queue.front();
            queue.pop();
            sortedJoints.push_back(skeleton->joints[jointIdx]);
            i32 newIdx = static_cast<int>(sortedJoints.size()) - 1;
            // Update joint indices in the new array
            if (sortedJoints[newIdx].parentJointId != -1)
            {
                // Find new parent index
                std::string parentName = skeleton->joints[sortedJoints[newIdx].parentJointId].name;
                for (i32 j = 0; j < newIdx; ++j)
                {
                    if (sortedJoints[j].name == parentName)
                    {
                        sortedJoints[newIdx].parentJointId = j;
                        break;
                    }
                }
            }
            // Add children to queue
            for (size_t i = 0; i < skeleton->joints.size(); ++i)
            {
                if (skeleton->joints[i].parentJointId == jointIdx)
                {
                    queue.push(i);
                }
            }
        }
        // Update name to joint name
        skeleton->nameToJointMap.clear();
        for (size_t i = 0; i < sortedJoints.size(); ++i)
        {
            sortedJoints[i].id = i;
            skeleton->nameToJointMap[sortedJoints[i].name] = i;
        }
        skeleton->joints = std::move(sortedJoints);
    }

    void MeshLoader::LoadMaterial(const aiScene *scene, aiMaterial *assimpMaterial, const std::string &filepath, Material &material)
    {
        aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 0.0f);
        f32 reflectivity = 0.0f;

        assimpMaterial->Get(AI_MATKEY_BASE_COLOR, baseColor);
        assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
        assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
        assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, material.data.metallic);
        assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.data.roughness);
        assimpMaterial->Get(AI_MATKEY_REFLECTIVITY, reflectivity);
        material.data.baseColor = { baseColor.r, baseColor.g, baseColor.b, 1.0f };
        
        if (diffuseColor.r > 0.0f)
            material.data.emissive = emissiveColor.r / diffuseColor.r;

        // load textures
        LoadTextures(scene, assimpMaterial, &material, aiTextureType_DIFFUSE);
        LoadTextures(scene, assimpMaterial, &material, aiTextureType_SPECULAR);
        LoadTextures(scene, assimpMaterial, &material, aiTextureType_EMISSIVE);
        LoadTextures(scene, assimpMaterial, &material, aiTextureType_DIFFUSE_ROUGHNESS);
        LoadTextures(scene, assimpMaterial, &material, aiTextureType_NORMALS);

        // set transparent and reflectivity
        material._transparent = false;
        material._reflective = reflectivity > 0.0f;
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

    void MeshLoader::CalculateWorldTransforms(std::vector<NodeInfo> &nodes)
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
#if 0
            // Apply node's world transform to all its meshes
            for (i32 meshIdx : nodes[i].meshIndices)
            {
                outMeshIndexGlobalMatrices[meshIdx] = nodes[i].worldTransform;
            }
#endif
        }
    }

    void MeshLoader::ClearTextureCache()
    {
        textureCache.clear();
    }


    // Entity Mesh class
    void Mesh::CreateBuffers()
    {
        nvrhi::IDevice *device = Application::GetDeviceManager()->GetDevice();

        // create vertex buffer
        nvrhi::BufferDesc vbDesc = nvrhi::BufferDesc();
        vbDesc.isVertexBuffer = true;
        vbDesc.byteSize = sizeof(VertexMesh) * data.vertices.size();
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        vbDesc.keepInitialState = true;
        vbDesc.debugName = "[Mesh] vertex buffer";

        vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(vertexBuffer, "[Mesh] Failed to create Vertex Buffer");

        // Outline
        {
            vbDesc = nvrhi::BufferDesc();
            vbDesc.isVertexBuffer = true;
            vbDesc.byteSize = sizeof(VertexMeshOutline) * outlineVertices.size();
            vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
            vbDesc.keepInitialState = true;
            vbDesc.debugName = "[Mesh] vertex buffer";
            outlineVertexBuffer = device->createBuffer(vbDesc);
            LOG_ASSERT(outlineVertexBuffer, "[Mesh] Failed to create Vertex Buffer");
        }

        // create index buffer
        nvrhi::BufferDesc ibDesc = nvrhi::BufferDesc();
        ibDesc.isIndexBuffer = true;
        ibDesc.byteSize = sizeof(uint32_t) * data.indices.size();
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
        ibDesc.keepInitialState = true;
        ibDesc.debugName = "[Mesh] index buffer";

        indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(indexBuffer, "[Mesh] Failed to create Index Buffer");

        // Constant buffer
        auto constantBufferDesc = nvrhi::BufferDesc()
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setMaxVersions(16)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer);

        // create per mesh constant buffers
        constantBufferDesc.setDebugName("Mesh constant buffer");
        constantBufferDesc.setByteSize(sizeof(ObjectBuffer));
        objectBufferHandle = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(objectBufferHandle, "[Model] Failed to create object constant buffer");

        constantBufferDesc.setDebugName("Material constant buffer");
        constantBufferDesc.setByteSize(sizeof(MaterialData));
        materialBufferHandle = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(materialBufferHandle, "[Model] Failed to create material constant buffer");
    }

}