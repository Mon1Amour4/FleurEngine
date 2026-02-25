#pragma once

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
        if (currentStatus == LOADING_STATUS_TO_TERMINATE || (currentStatus == LOADING_STATUS_TO_TERMINATE && status == !CORRUPTED))
            return false;

        currentStatus = status;
        return true;
    }

    inline EAsyncOperationStatus GetStatus() const
    {
        return currentStatus;
    }

private:
    EAsyncOperationStatus currentStatus;
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
    bool isGpuUploaded{false};
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
