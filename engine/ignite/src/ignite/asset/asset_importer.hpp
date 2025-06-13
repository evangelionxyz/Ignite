#pragma once

#include "asset.hpp"
#include <future>
#include <nvrhi/nvrhi.h>

#include "ignite/scene/entity.hpp"

namespace ignite {

    class Environment;
    class GraphicsPipeline;
    class Scene;

    class AssetImporter
    {
    public:
        static void SyncMainThread(nvrhi::ICommandList *commandList, nvrhi::IDevice *device);
        static Ref<Asset> Import(AssetHandle handle, const AssetMetaData &metadata);
        static Ref<Scene> ImportScene(AssetHandle handle, const AssetMetaData &metadata);
        static Ref<Texture> ImportTexture(AssetHandle handle, const AssetMetaData &metadata);
        static void LoadSkinnedMesh(Scene *scene, Entity outEntity, const std::filesystem::path& filepath);
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
}
