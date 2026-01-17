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
    READY_TO_TERMINATE,
    LOADING_STATUS_MAX_VALUE
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
    ELoadingSts status;
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
