#pragma once

#include "ignite/graphics/model.hpp"

#include <future>
#include <vector>
#include <nvrhi/nvrhi.h>

namespace ignite {

    class Environment;
    class GraphicsPipeline;

    class AssetImporter
    {
    public:
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);
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
        static void Import(Ref<Environment> *outEnvironment, nvrhi::IDevice *device, const std::string &filepath, const Ref<GraphicsPipeline> &pipeline);
        static void UpdateTexture(Ref<Environment> *outEnvironment, nvrhi::IDevice *device, const std::string &filepath, const Ref<GraphicsPipeline> &pipeline);
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);

    private:
        static Ref<Environment> ImportAsync(Ref<Environment> *outEnvironment, nvrhi::IDevice *device, const std::string &filepath, const Ref<GraphicsPipeline> &pipeline);
        static Ref<Environment> LoadTextureAsync(Ref<Environment> *outEnvironment, nvrhi::IDevice *device, const std::string &filepath, const Ref<GraphicsPipeline> &pipeline);
        static std::future<Ref<Environment>> m_Future;
    };
}