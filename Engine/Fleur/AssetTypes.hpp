#pragma once

#include <atomic>
#include <memory>

#include "AssetHandle.h"
#include "Renderer/Image2D.h"
#include "Renderer/Model.h"

namespace Fleur
{

using AssetID = uint32_t;

enum EAsyncOperationStatus
{
    REGISTERED,

    TO_BE_LOADED,
    LOADING,

    CORRUPTED,
    LOADING_STATUS_TO_TERMINATE,

    LOADED,

    LOADING_STATUS_MAX_VALUE
};

class FLAsyncLoadStatus
{
public:
    FLAsyncLoadStatus() = default;
    FLAsyncLoadStatus(EAsyncOperationStatus status)
        : currentStatus(status) {};

    bool SetStatus(Fleur::EAsyncOperationStatus status)
    {
        const EAsyncOperationStatus current = currentStatus.load(std::memory_order_acquire);
        if (current == LOADING_STATUS_TO_TERMINATE && status != CORRUPTED)
            return false;

        currentStatus.store(status, std::memory_order_release);
        return true;
    }

    inline EAsyncOperationStatus GetStatus() const
    {
        return currentStatus.load(std::memory_order_acquire);
    }

private:
    std::atomic<EAsyncOperationStatus> currentStatus{LOADING_STATUS_MAX_VALUE};
};


template <typename T>
struct Asset
{
    Fleur::AssetHandle handle{};
    T* obj = nullptr;
};

template <typename T>
struct AsyncOperation
{
    Asset<T> asset;
    FLAsyncLoadStatus status;
    std::shared_ptr<std::atomic_bool> isGpuUploaded = std::make_shared<std::atomic_bool>(false);
};

template <typename T>
struct AssetRecord
{
    bool registered;
    bool alreadyExist;
    Asset<T> asset;
};

struct FinishedAsset
{
    AssetID id;
    const char* name;
};


}  // namespace Fleur
