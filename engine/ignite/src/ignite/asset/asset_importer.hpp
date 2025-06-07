#pragma once

#include "asset.hpp"
#include "ignite/graphics/model.hpp"

#include <future>
#include <vector>
#include <nvrhi/nvrhi.h>

namespace ignite {

    class Environment;
    class GraphicsPipeline;
    class Scene;

    class AssetImporter
    {
    public:
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);
        static Ref<Asset> Import(AssetHandle handle, const AssetMetaData &metadata);
    };

    class ModelImporter : public AssetImporter
    {
    public:
        static void LoadAsync(Ref<Model> *model, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline);
        static void LoadAsync(std::vector<Ref<Model>> *outModels, const std::vector<std::string> &filepaths, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline);
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);

    private:
        static Ref<Model> LoadModels(std::vector<Ref<Model>> *outModels, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline);
        static Ref<Model> LoadModel(Ref<Model> *outModels, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline);
        static std::vector<std::future<Ref<Model>>> m_ModelFutures;
    };

    class EnvironmentImporter : public AssetImporter
    {
    public:
        static void Import(Ref<Environment> *outEnvironment, const std::string &filepath);
        static void UpdateTexture(Ref<Environment> *outEnvironment, const std::string &filepath);
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);

    private:
        static Ref<Environment> ImportAsync(Ref<Environment> *outEnvironment, const std::string &filepath);
        static Ref<Environment> LoadTextureAsync(Ref<Environment> *outEnvironment, const std::string &filepath);
        static std::future<Ref<Environment>> m_Future;
    };

    class SceneImporter : public AssetImporter
    {
    public:
        static Ref<Scene> Import(AssetHandle handle, const AssetMetaData &metadata);

    };
}