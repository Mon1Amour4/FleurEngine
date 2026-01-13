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
    READY_TO_TERMINATE
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

template <typename T>
class AssetCache
{
public:
    Fleur::Asset<T> Load(std::string_view name, AssetID id) {};
    std::shared_ptr<AsyncOperation<T>> LoadAsync(std::string_view path, AssetID id) {};
    AssetRecord<T> Add(std::string_view name, AssetID id) {};
    AssetRecord<T> Exist(std::string_view name) {};
    Asset<T> Get(std::string_view name) {};
    Asset<T> Get(AssetID id) {};
    void Release(std::string_view){};
    void Release(AssetID id) {};
    void RemoveBrokenAsyncAsset(AssetID id) {};
    void Tick() {};

private:
    std::list<std::shared_ptr<Fleur::AsyncOperation<T>>> asyncOperationsToRelease;
    std::unordered_map<AssetID, std::shared_ptr<Fleur::AsyncOperation<T>>> asyncMap;
    std::unordered_map<std::string, AssetID> stringMap;
    std::unordered_map<AssetID, T> map;
    std::atomic<uint32_t> m_size;
    std::mutex mutex;
};

template <>
class AssetCache<Fleur::Graphics::Image2D>
{
    using type = Fleur::Graphics::Image2D;

public:
    Fleur::Asset<type> Load(std::string_view path, AssetID id);
    std::shared_ptr<AsyncOperation<type>> LoadAsync(std::string_view path, AssetID id);
    AssetRecord<type> Add(std::string_view name, AssetID id);
    AssetRecord<type> Exist(std::string_view name);
    Asset<type> Get(std::string_view name);
    Asset<type> Get(AssetID id);
    void Release(std::string_view);
    void Release(AssetID id);
    void RemoveBrokenAsyncAsset(AssetID id);
    void Tick();

private:
    std::list<std::shared_ptr<Fleur::AsyncOperation<type>>> asyncOperationsToRelease;
    std::unordered_map<AssetID, std::shared_ptr<Fleur::AsyncOperation<type>>> asyncMap;
    std::unordered_map<std::string, AssetID> stringMap;
    std::unordered_map<AssetID, type> map;
    std::atomic<uint32_t> m_size;
    std::mutex mutex;
};

// Model
template <>
class AssetCache<Fleur::Graphics::Model>
{
    using type = Fleur::Graphics::Model;

public:
    Fleur::Asset<type> Load(std::string_view path, AssetID id);
    std::shared_ptr<AsyncOperation<type>> LoadAsync(std::string_view path, AssetID id);
    AssetRecord<type> Add(std::string_view name, AssetID id);
    AssetRecord<type> Exist(std::string_view name);
    // Get
    // Release

private:
    std::list<AssetID> asyncOperationsToRelease;
    std::unordered_map<AssetID, std::shared_ptr<Fleur::AsyncOperation<type>>> asyncMap;
    std::unordered_map<std::string, AssetID> stringMap;
    std::unordered_map<AssetID, type> map;
    std::atomic<uint32_t> m_size;
    std::mutex mutex;
};
}  // namespace Fleur
