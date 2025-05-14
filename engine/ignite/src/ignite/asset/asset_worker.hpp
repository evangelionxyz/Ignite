#pragma once

#include "asset.hpp"

#include <queue>
#include <future>
#include <functional>

namespace ignite {

    class Asset;

    class AssetTaskBase
    {
    public:
        virtual ~AssetTaskBase() = default;
        virtual bool Execute(nvrhi::CommandListHandle commandList, nvrhi::IDevice *device) = 0;
    };

    // AssetType: actual asset type (Model, Scene, etc...)
    template<typename AssetType>
    class AssetTask : public AssetTaskBase
    {
    public:
        Ref<AssetType> *asset;
        std::future<Ref<AssetType>> future;

        template<typename Func, typename... Args>
        AssetTask(Ref<AssetType> *asset, Func &&func, Args &&... args)
            : asset(asset)
        {
            future = std::async(std::launch::async,
                std::forward<Func>(func), // Function
                std::forward<Args>(args)...); // Function Arguments
        }

        // Move constructor to allow transfer of ownership of future
        AssetTask(AssetTask &&other) noexcept
            : asset(std::move(other.asset)), future(std::move(other.future))
        {
        }

        // this should be executed in main thread
        virtual bool Execute(nvrhi::CommandListHandle commandList, nvrhi::IDevice *device) override
        {
            if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                Ref<AssetType> _returnedAsset = future.get();
                if (_returnedAsset)
                {
                    commandList->open();

                    // Assign the result to the asset pointer, casting to base class if needed
                    _returnedAsset->WriteBuffer(commandList);
                    *asset = _returnedAsset;

                    commandList->close();
                    device->executeCommandList(commandList);

                    return true;
                }
            }
            return false;
        }

        AssetTask &operator=(AssetTask &other) noexcept
        {
            if (this != &other)
            {
                asset = std::move(other.asset);
                future = std::move(other.future);
            }
            return *this;
        }

        // disable copy constructor and assignment operator
        AssetTask(const AssetTask &) = delete;
        AssetTask &operator=(const AssetTask &) = delete;
    };

    class AssetWorker
    {
    public:

        template<typename T>
        static void Submit(AssetTask<T> &task)
        {
            s_Tasks.push_back(std::make_unique<AssetTask<T>>(std::move(task)));
        }

        static void SyncMainThread(nvrhi::CommandListHandle commandList, nvrhi::IDevice *device)
        {
            for (auto it = s_Tasks.begin(); it != s_Tasks.end(); )
            {
                if ((*it)->Execute(commandList, device))
                {
                    it = s_Tasks.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

    private:
        static std::vector<Scope<AssetTaskBase>> s_Tasks;
        static float s_WaitDelay;
        static float s_Timer;
    };

    

}