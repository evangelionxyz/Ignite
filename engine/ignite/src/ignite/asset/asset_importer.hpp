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
        
    };

    class ModelImporter : public AssetImporter
    {
    public:
        static void LoadAsync(std::vector<Ref<Model>> *outModels, const std::vector<std::string> &filepaths, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline);
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);

    private:
        static Ref<Model> LoadModel(std::vector<Ref<Model>> *outModels, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline);
        static std::vector<std::future<Ref<Model>>> m_ModelFutures;
    };
}