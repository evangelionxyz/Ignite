#include "asset_importer.hpp"

#include "ignite/graphics/graphics_pipeline.hpp"
#include "ignite/graphics/environment.hpp"

namespace ignite {

    std::vector<std::future<Ref<Model>>> ModelImporter::m_ModelFutures;
    void ModelImporter::LoadAsync(std::vector<Ref<Model>> *outModels, const std::vector<std::string> &filepaths, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline)
    {
        for (const auto &filepath : filepaths)
        {
            m_ModelFutures.push_back(std::async(std::launch::async, LoadModel, outModels, filepath, createInfo, env, pipeline));
        }
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

    Ref<Model> ModelImporter::LoadModel(std::vector<Ref<Model>> *outModels, const std::string &filepath, const ModelCreateInfo &createInfo, const Ref<Environment> &env, const Ref<GraphicsPipeline> &pipeline)
    {
        Ref<Model> model = Model::Create(filepath, createInfo);

        model->SetEnvironmentTexture(env->GetHDRTexture());
        model->CreateBindingSet(pipeline->GetBindingLayout());

        outModels->push_back(model);

        return model;
    }


}