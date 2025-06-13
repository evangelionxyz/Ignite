#include "asset_importer.hpp"

#include "ignite/core/application.hpp"
#include "ignite/project/project.hpp"
#include "ignite/serializer/serializer.hpp"
#include "ignite/graphics/scene_renderer.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/graphics/mesh_loader.hpp"
#include "ignite/graphics/mesh.hpp"

#include "ignite/scene/scene.hpp"
#include "ignite/scene/component.hpp"
#include "ignite/scene/scene_manager.hpp"

namespace ignite {

    static std::unordered_map<AssetType, std::function<Ref<Asset>(UUID, const AssetMetaData &)>> s_ImportFunctions =
    {
        { AssetType::Scene, AssetImporter::ImportScene },
        { AssetType::Texture, AssetImporter::ImportTexture },
    };

    void AssetImporter::SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device)
    {
        // ModelImporter::SyncMainThread(commandList, device);
        EnvironmentImporter::SyncMainThread(commandList, device);
    }

    Ref<Asset> AssetImporter::Import(AssetHandle handle, const AssetMetaData &metadata)
    {
        Project *activeProject = Project::GetInstance();

        // should be always importing with full filepath
        AssetMetaData metadataCopy = metadata;
        metadataCopy.filepath = activeProject->GetAssetFilepath(metadata.filepath);

        if (s_ImportFunctions.contains(metadataCopy.type))
            return s_ImportFunctions.at(metadataCopy.type)(handle, metadataCopy);

        return nullptr;
    }

    Ref<Scene> AssetImporter::ImportScene(AssetHandle handle, const AssetMetaData& metadata)
    {
        Ref<Scene> scene = SceneSerializer::Deserialize(metadata.filepath);
        if (scene)
        {
            scene->handle = handle;
        }
        return scene;
    }

    Ref<Texture> AssetImporter::ImportTexture(AssetHandle handle, const AssetMetaData &metadata)
    {
        TextureCreateInfo createInfo;
        createInfo.format = nvrhi::Format::RGBA8_UNORM;

        Ref<Texture> texture = Texture::Create(metadata.filepath, createInfo);
        if (texture)
        {
            texture->handle = handle;
        }

        return texture;
    }

    void AssetImporter::LoadSkinnedMesh(Scene *scene, Entity outEntity, const std::filesystem::path &filepath)
    {
        LOG_ASSERT(std::filesystem::exists(filepath), "[Mesh Loader] File does not exists!");

        SkinnedMesh &skinnedMesh = outEntity.GetComponent<SkinnedMesh>();
        skinnedMesh.filepath = Project::GetInstance()->GetAssetRelativeFilepath(filepath);

        Assimp::Importer importer;
        const aiScene *assimpScene = importer.ReadFile(filepath.generic_string(), ASSIMP_IMPORTER_FLAGS);

        LOG_ASSERT(assimpScene == nullptr || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || assimpScene->mRootNode,
            "[Model] Failed to load {}: {}",
            filepath,
            importer.GetErrorString());

        if (!assimpScene)
        {
            return;
        }

        skinnedMesh.skeleton = CreateRef<Skeleton>();

        if (assimpScene->HasAnimations())
        {
            MeshLoader::LoadAnimation(assimpScene, skinnedMesh.animations);

            // Process Skeleton
            MeshLoader::ExtractSkeleton(assimpScene, skinnedMesh.skeleton);
            MeshLoader::SortJointsHierarchically(skinnedMesh.skeleton);
        }

        std::vector<Ref<Mesh>> meshes;
        meshes.resize(assimpScene->mNumMeshes);
        for (auto &mesh : meshes)
        {
            mesh = CreateRef<Mesh>();
        }

        std::vector<NodeInfo> nodes;

        MeshLoader::ProcessNode(assimpScene, assimpScene->mRootNode, filepath, meshes, nodes, skinnedMesh.skeleton, -1);
        MeshLoader::CalculateWorldTransforms(nodes);

        // First pass: create all node entities
        for (auto &node : nodes)
        {
            if (node.uuid == UUID(0) || node.parentID != -1) // not yet created
            {
                Entity nodeEntity = SceneManager::CreateEntity(scene, node.name, EntityType_Node);
                node.uuid = nodeEntity.GetUUID();

                Transform &tr = nodeEntity.GetComponent<Transform>();

                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(node.localTransform, tr.localScale, tr.localRotation, tr.localTranslation, skew, perspective);

                tr.dirty = true;
            }
        }

        // Second pass: establish hierarchy and add meshes
        for (auto &node : nodes)
        {
            Entity nodeEntity = SceneManager::GetEntity(scene, node.uuid);

            if (node.parentID == -1)
            {
                // Attach the node to root node
                ID &id = outEntity.GetComponent<ID>();
                id.name = filepath.stem().string();

                SceneManager::AddChild(scene, outEntity, nodeEntity);
            }
            else
            {
                // Attach to parent if not root
                const auto &parentNode = nodes[node.parentID];
                Entity parentEntity = SceneManager::GetEntity(scene, parentNode.uuid);
                SceneManager::AddChild(scene, parentEntity, nodeEntity);
            }

            // Attach mesh entities to this node
            for (i32 meshIdx : node.meshIndices)
            {
                const auto &mesh = meshes[meshIdx];

                MeshRenderer &meshRenderer = nodeEntity.AddComponent<MeshRenderer>();
                meshRenderer.meshIndex = meshIdx;
                meshRenderer.root = outEntity.GetUUID();
                meshRenderer.mesh = mesh;
                meshRenderer.mesh->environment = scene->sceneRenderer->GetEnvironment();
                meshRenderer.mesh->CreateBuffers();
                meshRenderer.mesh->CreateBindingSet();
                meshRenderer.mesh->WriteBuffers(nodeEntity);
            }

            // Extract skeleton joints into entity
            for (size_t i = 0; i < skinnedMesh.skeleton->joints.size(); ++i)
            {
                const std::string &name = skinnedMesh.skeleton->joints[i].name;
                for (auto [uuid, e] : scene->entities)
                {
                    Entity jointEntity = { e, scene };
                    if (jointEntity.GetName() == name)
                    {
                        skinnedMesh.skeleton->jointEntityMap[static_cast<i32>(i)] = uuid;
                        jointEntity.GetComponent<ID>().type = EntityType_Joint;
                        break;
                    }
                }
            }
        }

        MeshLoader::ClearTextureCache();
    }

#if 0
    std::vector<std::future<Ref<Model>>> ModelImporter::m_ModelFutures;
    void ModelImporter::LoadAsync(std::vector<Ref<Model>> *outModels, const std::vector<std::string> &filepaths, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline)
    {
        for (const std::string &filepath : filepaths)
        {
            m_ModelFutures.push_back(std::async(std::launch::async, LoadModels, outModels, filepath, createInfo, env, pipeline));
        }
    }

    void ModelImporter::LoadAsync(Ref<Model> *model, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline)
    {
        m_ModelFutures.push_back(std::async(std::launch::async, LoadModel, model, filepath, createInfo, env, pipeline));
    }

    void ModelImporter::SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device)
    {
        for (auto it = m_ModelFutures.begin(); it != m_ModelFutures.end(); )
        {
            if ((*it).wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                Ref<Model> returnedModel = (*it).get();
                commandList->open();
                returnedModel->WriteBuffer(commandList);
                commandList->close();

                device->executeCommandList(commandList);

                it = m_ModelFutures.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    Ref<Model> ModelImporter::LoadModels(std::vector<Ref<Model>> *outModels, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline)
    {
        Ref<Model> model = Model::Create(filepath, createInfo);

        model->SetEnvironment(env);
        model->CreateBindingSet();

        outModels->push_back(model);

        return model;
    }


    Ref<Model> ModelImporter::LoadModel(Ref<Model> *outModels, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline)
    {
        *outModels = Model::Create(filepath, createInfo);

        (*outModels)->SetEnvironment(env);
        (*outModels)->CreateBindingSet();

        return *outModels;
    }
#endif

    void EnvironmentImporter::Import(Ref<Environment> *outEnvironment, const std::string &filepath)
    {
        m_Future = std::async(std::launch::async, ImportAsync, outEnvironment, filepath);
    }

    void EnvironmentImporter::UpdateTexture(Ref<Environment> *outEnvironment, const std::string &filepath)
    {
        m_Future = std::async(std::launch::async, LoadTextureAsync, outEnvironment, filepath);
    }

    void EnvironmentImporter::SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device)
    {
        if (m_Future.valid() && m_Future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            Ref<Environment> env = m_Future.get();

            commandList->open();
            env->WriteBuffer(commandList);
            commandList->close();
            device->executeCommandList(commandList);

            env->isUpdatingTexture = true;
        }
    }

    Ref<Environment> EnvironmentImporter::ImportAsync(Ref<Environment> *outEnvironment, const std::string &filepath)
    {
        (*outEnvironment) = Environment::Create();
        (*outEnvironment)->LoadTexture(filepath);
        return *outEnvironment;
    }

    Ref<Environment> EnvironmentImporter::LoadTextureAsync(Ref<Environment> *outEnvironment, const std::string &filepath)
    {
        (*outEnvironment)->LoadTexture(filepath);
        return *outEnvironment;
    }

    std::future<Ref<Environment>> EnvironmentImporter::m_Future;

}
