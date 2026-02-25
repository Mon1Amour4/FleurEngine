#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "AssetTypes.hpp"

namespace Fleur
{

template <typename T>
class AssetCache
{
    using TRecord = Fleur::AssetRecord<T>;
    using TAsset = Fleur::Asset<T>;
    using TAsyncOp = Fleur::AsyncOperation<T>;
    using TAsyncOpShared = std::shared_ptr<TAsyncOp>;

public:
    /// Registers a synchronous asset by path and numeric ID.
    TRecord Register(std::string_view path, AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);

        std::string name = GetNameFromPath(path);
        TRecord record = Exist(name);
        if (record.alreadyExist)
            return record;

        AssetHandle handle{id, 1};
        stringMap.emplace(name, handle);
        generationMap.emplace(id, handle.generation);
        T* ptr = &map.emplace(id, name).first->second;
        m_size++;

        return {true, false, {handle, ptr}};
    };

    /// Registers an asynchronous asset operation by path and numeric ID.
    TAsyncOpShared RegisterAsync(std::string_view path, AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);

        std::string name = GetNameFromPath(path);

        // 1. Check async map first.
        TAsyncOpShared asyncOperation = FindExistingAsyncOperation(name);
        if (asyncOperation.get()->status.GetStatus() != EAsyncOperationStatus::LOADING_STATUS_MAX_VALUE)
            return asyncOperation;

        // 2. Check completed map.
        TRecord completedRecord = Exist(name);
        if (completedRecord.alreadyExist)
            return std::make_shared<TAsyncOp>(completedRecord.asset, EAsyncOperationStatus::LOADED);

        AssetHandle handle{id, 1};
        T* ptr = &map.emplace(id, name).first->second;
        stringMap.emplace(name, handle);
        generationMap.emplace(id, handle.generation);
        m_size++;
        return asyncMap.emplace(name, std::make_shared<TAsyncOp>(TAsset{handle, ptr}, REGISTERED)).first->second;
    };

    /// Returns existing async operation for asset name or sentinel operation.
    TAsyncOpShared FindExistingAsyncOperation(std::string_view name)
    {
        if (auto operation = asyncMap.find(name.data()); operation != asyncMap.end())
            return operation->second;
        else
            return {std::make_shared<TAsyncOp>(TAsset{{}, nullptr}, EAsyncOperationStatus::LOADING_STATUS_MAX_VALUE)};
    }

    /// Returns cache record by asset logical name.
    AssetRecord<T> Exist(std::string_view name)
    {
        TRecord record{false, false, {{}, nullptr}};

        if (auto rec = stringMap.find(name.data()); rec != stringMap.end())
        {
            const AssetHandle handle = rec->second;
            if (auto gen = generationMap.find(handle.id); gen != generationMap.end() && gen->second == handle.generation)
            {
                if (auto byId = map.find(handle.id); byId != map.end())
                {
                    record.registered = true;
                    record.alreadyExist = true;
                    record.asset.handle = handle;
                    record.asset.obj = &byId->second;
                }
            }
        }

        return record;
    };

    /// Returns asset by logical name.
    Asset<T> Get(std::string_view name)
    {
        std::lock_guard<std::mutex> lc(mutex);
        return Exist(name).asset;
    };

    /// Returns asset by stable handle (ID + generation).
    Asset<T> Get(const AssetHandle& handle)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto gen = generationMap.find(handle.id); gen != generationMap.end() && gen->second == handle.generation)
        {
            if (auto rec = map.find(handle.id); rec != map.end())
                return TAsset{handle, &rec->second};
        }
        return TAsset{{}, nullptr};
    }

    /// Releases asset by logical name.
    void Release(std::string_view name)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto record = stringMap.find(name.data()); record != stringMap.end())
        {
            AssetHandle handle = record->second;
            if (auto operation = asyncMap.find(GetNameFromPath(name)); operation != asyncMap.end())
            {
                asyncOperationsToRelease.emplace(operation->first, operation->second);
                operation->second->status.SetStatus(Fleur::EAsyncOperationStatus::LOADING_STATUS_TO_TERMINATE);
                return;
            }
            stringMap.erase(name.data());
            generationMap.erase(handle.id);
            map.erase(handle.id);
        }
    };

    /// Releases asset by handle if generation matches.
    void Release(const AssetHandle& handle)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto gen = generationMap.find(handle.id); gen == generationMap.end() || gen->second != handle.generation)
            return;
        if (auto record = map.find(handle.id); record != map.end())
        {
            std::string_view name = record->second.GetName();
            if (auto operation = asyncMap.find(name.data()); operation != asyncMap.end())
            {
                asyncOperationsToRelease.emplace(operation->first, operation->second);
                operation->second->status.SetStatus(Fleur::EAsyncOperationStatus::LOADING_STATUS_TO_TERMINATE);
                return;
            }
            stringMap.erase(name.data());
            generationMap.erase(handle.id);
            map.erase(handle.id);
        }
    };

    /// Removes corrupted async asset state by numeric ID.
    void RemoveBrokenAsyncAsset(AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto record = map.find(id); record != map.end())
        {
            std::string name = record->second.GetName().data();
            asyncMap.erase(name);
            stringMap.erase(name);
            generationMap.erase(id);
            map.erase(id);
            asyncOperationsToRelease.erase(name);
        }
    };

    /// Removes finished async operation by logical name.
    void RemoveFromAsyncOperations(std::string_view name)
    {
        asyncMap.erase(name.data());
    }

private:
    std::string GetNameFromPath(std::string_view path)
    {
        return std::filesystem::path(path).filename().string();
    }

    std::unordered_map<std::string, std::shared_ptr<Fleur::AsyncOperation<T>>> asyncOperationsToRelease;
    std::unordered_map<std::string, std::shared_ptr<Fleur::AsyncOperation<T>>> asyncMap;
    std::unordered_map<std::string, AssetHandle> stringMap;
    std::unordered_map<AssetID, uint32_t> generationMap;
    std::unordered_map<AssetID, T> map;
    std::atomic<uint32_t> m_size;
    std::mutex mutex;
};

}  // namespace Fleur
