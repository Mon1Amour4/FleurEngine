#pragma once

#include "Renderer/Image2D.h"
#include "Renderer/Model.h"

namespace Fleur
{

using AssetID = uint32_t;

enum ELoadingSts
{
    TO_BE_LOADED,
    LOADING,
    CORRUPTED,
    SUCCESS,
    LOADING_STATUS_TO_TERMINATE,
    LOADING_STATUS_MAX_VALUE
};

class FLAsyncLoadStatus
{
public:
    FLAsyncLoadStatus() = default;
    FLAsyncLoadStatus(ELoadingSts status)
        : currentStatus(status) {};

    bool SetStatus(Fleur::ELoadingSts status)
    {
        if (currentStatus == LOADING_STATUS_TO_TERMINATE || (currentStatus == LOADING_STATUS_TO_TERMINATE && status == !CORRUPTED))
            return false;

        currentStatus = status;
        return true;
    }

    inline ELoadingSts GetStatus() const
    {
        return currentStatus;
    }

private:
    ELoadingSts currentStatus;
};


template <typename T>
struct Asset
{
    Fleur::AssetID ID;
    T* obj;
};

template <typename T>
struct AsyncOperation
{
    Asset<T> asset;
    FLAsyncLoadStatus status;
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
