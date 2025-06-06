#include "asset_importer.hpp"

#include "ignite/serializer/serializer.hpp"
#include "ignite/project/project.hpp"
#include "ignite/scene/scene.hpp"
#include "ignite/graphics/graphics_pipeline.hpp"
#include "ignite/graphics/environment.hpp"

#include "ignite/core/application.hpp"

namespace ignite {

    static std::unordered_map<AssetType, std::function<Ref<Asset>(UUID, const AssetMetaData &)>> s_ImportFuntions =
    {
        { AssetType::Scene, SceneImporter::Import }
    };

    void AssetImporter::SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device)
    {
        ModelImporter::SyncMainThread(commandList, device);
        EnvironmentImporter::SyncMainThread(commandList, device);
    }

    Ref<Asset> AssetImporter::Import(AssetHandle handle, const AssetMetaData &metadata)
    {
        Project *activeProject = Project::GetInstance();

        // should be always importing with full filepath
        AssetMetaData metadataCopy = metadata;
        metadataCopy.filepath = activeProject->GetAssetFilepath(metadata.filepath);

        if (s_ImportFuntions.contains(metadataCopy.type))
            return s_ImportFuntions.at(metadataCopy.type)(handle, metadataCopy);

        return nullptr;
    }

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

    void EnvironmentImporter::Import(Ref<Environment> *outEnvironment, const std::string &filepath)
    {
        m_Future = std::async(std::launch::async, ImportAsync, outEnvironment, filepath);
    }

    void EnvironmentImporter::UpdateTexture(Ref<Environment> *outEnvironment, const std::string &filepath)
    {
        nvrhi::IDevice *device = Application::GetRenderDevice();

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

            env->m_IsUpdatingTexture = true;
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
        nvrhi::IDevice *device = Application::GetRenderDevice();
        (*outEnvironment)->LoadTexture(filepath);
        return *outEnvironment;
    }
    std::future<Ref<Environment>> EnvironmentImporter::m_Future;

    Ref<Scene> SceneImporter::Import(AssetHandle handle, const AssetMetaData &metadata)
    {
        Ref<Scene> scene = SceneSerializer::Deserialize(metadata.filepath);
        if (scene)
            scene->handle = handle;
        return scene;
    }

}