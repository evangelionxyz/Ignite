#include "asset_worker.hpp"

namespace ignite {
    std::vector<Scope<AssetTaskBase>> AssetWorker::s_Tasks;
    float AssetWorker::s_WaitDelay = 0.0f;
    float AssetWorker::s_Timer = 0.0f;
}