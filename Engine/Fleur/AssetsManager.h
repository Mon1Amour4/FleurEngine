#pragma once

#include <filesystem>
#include <type_traits>

#include "AssetTypes.hpp"
#include "Renderer/Color.h"
#include "Renderer/RenderViews.hpp"
#include "Renderer/Shader.h"
#include "Services/ServiceInterfaces.hpp"

namespace Fleur
{

class AssetsManager;

enum AssetEventType
{
    EVENT_TYPE_MODEL_LOADED,
    EVENT_TYPE_IMAGE2D_LOADED,
    EVENT_TYPE_MAX_VALUE,
};
struct AssetLoadResult
{
    AssetEventType type = EVENT_TYPE_MAX_VALUE;
    const void* pResource = nullptr;
    AssetID ID = 0;
    ELoadingSts loadingStatus = LOADING_STATUS_MAX_VALUE;
};

struct AssetLoadCallback
{
    using CallbackFn = void (Fleur::AssetsManager::*)(AssetLoadResult);

    AssetLoadResult result;
    Fleur::AssetsManager* manager = nullptr;

    inline void operator()()
    {
        (manager->*callback)(result);
    }

    CallbackFn callback = nullptr;
};

template <typename T>
class AssetCache
{
    using TRecord = Fleur::AssetRecord<T>;
    using TAsset = Fleur::Asset<T>;
    using TAsyncOp = Fleur::AsyncOperation<T>;
    using TAsyncOpShared = std::shared_ptr<TAsyncOp>;

public:
    Fleur::Asset<T> Load(std::string_view path, AssetID id) {};
    std::shared_ptr<AsyncOperation<T>> LoadAsync(std::string_view path, AssetID id, AssetLoadCallback callback) {};
    AssetRecord<T> Add(std::string_view name, AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);

        TRecord record = Exist(name);
        if (record.alreadyExist)
            return record;

        stringMap.emplace(name, id);
        T* ptr = &map.emplace(id, T(name)).first->second;
        m_size++;

        return {true, false, {id, ptr}};
    };
    AssetRecord<T> Exist(std::string_view name)
    {
        TRecord record{false, false, {0, nullptr}};

        if (auto rec = stringMap.find(name.data()); rec != stringMap.end())
        {
            record.registered = true;
            record.alreadyExist = true;
            record.asset.ID = stringMap[name.data()];
            record.asset.obj = &map[record.asset.ID];
        }

        return record;
    };
    Asset<T> Get(std::string_view name)
    {
        std::lock_guard<std::mutex> lc(mutex);
        return Exist(name).asset;
    };
    Asset<T> Get(AssetID id)
    {
        TAsset asset{0, nullptr};
        std::lock_guard<std::mutex> lc(mutex);
        if (auto rec = map.find(id); rec != map.end())
        {
            asset.ID = id;
            asset.obj = &rec->second;
        }

        return asset;
    };
    void Release(std::string_view name)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto record = stringMap.find(name.data()); record != stringMap.end())
        {
            AssetID id = record->second;
            if (auto operation = asyncMap.find(id); operation != asyncMap.end())
            {
                asyncOperationsToRelease.emplace(operation->first, operation->second);
                operation->second->status.SetStatus(Fleur::ELoadingSts::LOADING_STATUS_TO_TERMINATE);
                return;
            }
            stringMap.erase(name.data());
            map.erase(id);
        }
    };
    void Release(AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto operation = asyncMap.find(id); operation != asyncMap.end())
        {
            asyncOperationsToRelease.emplace(operation->first, operation->second);
            operation->second->status.SetStatus(Fleur::ELoadingSts::LOADING_STATUS_TO_TERMINATE);
            return;
        }
        if (auto object = map.find(id); object != map.end())
        {
            std::string name = object->second.Name().data();
            map.erase(id);
            stringMap.erase(name);
        }
    };
    void RemoveBrokenAsyncAsset(AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto record = map.find(id); record != map.end())
        {
            std::string name = record->second.Name().data();
            asyncMap.erase(id);
            stringMap.erase(name);
            map.erase(id);
            asyncOperationsToRelease.erase(id);
        }
    };
    void RemoveFromAsyncOperations(AssetID id)
    {
        asyncMap.erase(id);
    }

private:
    std::unordered_map<AssetID, std::shared_ptr<Fleur::AsyncOperation<T>>> asyncOperationsToRelease;
    std::unordered_map<AssetID, std::shared_ptr<Fleur::AsyncOperation<T>>> asyncMap;
    std::unordered_map<std::string, AssetID> stringMap;
    std::unordered_map<AssetID, T> map;
    std::atomic<uint32_t> m_size;
    std::mutex mutex;
};

class AssetsManager : public Service<AssetsManager>, public IUpdatable
{
public:
    // friend class Application;
    friend struct Service<AssetsManager>;

    AssetsManager();
    ~AssetsManager();

    void OnShutdown();
    void OnInit();
    void OnUpdate(float dtTime);

    // Load
    template <typename T>
    std::shared_ptr<AsyncOperation<T>> LoadAsync(std::string_view path)
    {
        if (path.empty())
            return std::shared_ptr<AsyncOperation<T>>(nullptr);

        AssetLoadCallback callback{};
        callback.manager = this;
        callback.callback = &Fleur::AssetsManager::OnAssetLoaded;

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.LoadAsync(path, m_GlobalId++, callback);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.LoadAsync(path, m_GlobalId++, callback);
        }
    }

    template <typename T>
    Asset<T> Load(std::string_view path)
    {
        if (path.empty())
            return Asset<T>{0, nullptr};

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            Asset<T> asset = m_Image2DCache.Load(path, m_GlobalId++);

            Fleur::Graphics::SFLImageView imageView = asset.obj->GetView();
            imageView.ID = asset.ID;
            AddImageToUpload(imageView);

            return asset;
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
            return m_ModelCache.Load(path, m_GlobalId++);
    }

    Asset<Fleur::Graphics::Image2D> FromColor(std::string_view name, Fleur::Graphics::Color color);
    Asset<Fleur::Graphics::Image2D> LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size);

    template <typename T>
    Asset<T> Get(std::string_view name)
    {
        if (name.empty())
            return Asset<T>({0, nullptr});

        if constexpr (std::is_same_v<T, Fleur::Graphics::Shader>)
        {
            AssetID id = m_ShaderMapString[name.data()];
            return Fleur::Asset<T>{id, &m_ShaderMap[id]};
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Get(name);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Get(name);
        }

        return Asset<T>({0, nullptr});
    }

    template <typename T>
    Asset<T> Get(AssetID id)
    {
        if (id == 0)
            return Asset<T>({0, nullptr});

        if constexpr (std::is_same_v<T, Fleur::Graphics::Shader>)
        {
            /*AssetID id = m_ShaderMapString[name.data()];
            return Fleur::Asset<T>{id, &m_ShaderMap[id]};*/
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Get(id);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Get(id);
        }

        return Asset<T>({0, nullptr});
    }

    template <typename T>
    void Release(std::string_view name)
    {
        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Release(name);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Release(name);
        }
    }

    template <typename T>
    void Release(AssetID id)
    {
        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Release(id);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Release(id);
        }
    }

    void OnAssetLoaded(AssetLoadResult info)
    {
        std::lock_guard<std::mutex> lc(m_MessageMutex);
        m_MessageQueue.push_back(info);
    }

private:
    std::atomic<uint32_t> m_GlobalId = 1;

    void load_all_shaders();
    std::unordered_map<std::string, AssetID> m_ShaderMapString;
    std::unordered_map<AssetID, Fleur::Graphics::Shader> m_ShaderMap;

    Fleur::AssetCache<Fleur::Graphics::Image2D> m_Image2DCache;
    Fleur::AssetCache<Fleur::Graphics::Model> m_ModelCache;


    struct ImagesUpload
    {
        uint32_t framesSinceLastUpload = 0;
        std::vector<Fleur::Graphics::SFLImageView> images;

        bool ReadyToUpload()
        {
            return (framesSinceLastUpload > 5 && images.size() > 0) || images.size() > 10;
        };
        void Add(Fleur::Graphics::SFLImageView view)
        {
            std::lock_guard<std::mutex> lc(mx);
            images.push_back(view);
        }
        void Clear()
        {
            images.clear();
        }
        std::mutex mx;
    };
    ImagesUpload m_ImagesToUpload;

    std::mutex m_MessageMutex;
    std::deque<AssetLoadResult> m_MessageQueue;
    void PollMessages();
};

template <>
Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetCache<Fleur::Graphics::Image2D>::Load(std::string_view path, AssetID id);
template <>
std::shared_ptr<AsyncOperation<Fleur::Graphics::Image2D>> Fleur::AssetCache<Fleur::Graphics::Image2D>::LoadAsync(std::string_view path, AssetID id,
                                                                                                                 AssetLoadCallback callback);
template <>
Fleur::Asset<Fleur::Graphics::Model> Fleur::AssetCache<Fleur::Graphics::Model>::Load(std::string_view path, AssetID id);
template <>
std::shared_ptr<AsyncOperation<Fleur::Graphics::Model>> Fleur::AssetCache<Fleur::Graphics::Model>::LoadAsync(std::string_view path, AssetID id,
                                                                                                             AssetLoadCallback callback);

}  // namespace Fleur
