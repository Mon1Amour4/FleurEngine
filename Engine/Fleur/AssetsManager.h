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
using Callback = void(Fleur::Graphics::Model*);

template <typename T>
class AssetCache
{
    using TRecord = Fleur::AssetRecord<T>;
    using TAsset = Fleur::Asset<T>;
    using TAsyncOp = Fleur::AsyncOperation<T>;
    using TAsyncOpShared = std::shared_ptr<TAsyncOp>;

public:
    Fleur::Asset<T> Load(std::string_view path, AssetID id) {};
    std::shared_ptr<AsyncOperation<T>> LoadAsync(std::string_view path, AssetID id) {};
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
                asyncOperationsToRelease.push_back(operation->second);
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
            asyncOperationsToRelease.push_back(operation->second);
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
        if (auto record = map.find(id); record != map.end())
        {
            std::string name = record->second.Name().data();
            asyncMap.erase(id);
            stringMap.erase(name);
            map.erase(id);
        }
    };
    void Tick()
    {
        std::lock_guard<std::mutex> lc(mutex);
        for (auto it = asyncOperationsToRelease.begin(); it != asyncOperationsToRelease.end();)
        {
            if (it->get()->status == ELoadingSts::READY_TO_TERMINATE || it->get()->status == ELoadingSts::SUCCESS ||
                it->get()->status == ELoadingSts::CORRUPTED)
            {
                AssetID id = it->get()->asset.ID;
                std::string name = it->get()->asset.obj->Name().data();
                asyncMap.erase(id);
                stringMap.erase(name);
                map.erase(id);
                it = asyncOperationsToRelease.erase(it);

                FL_CORE_INFO("[AssetsManager] Asset ({0}, {1}) has been released", id, name);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = asyncMap.begin(); it != asyncMap.end();)
        {
            if (it->second->status == ELoadingSts::CORRUPTED)
            {
                AssetID id = it->second->asset.ID;
                std::string name = it->second->asset.obj->Name().data();
                it = asyncMap.erase(it);
                stringMap.erase(name);
                map.erase(id);
            }
            else if (it->second->status == ELoadingSts::SUCCESS)
            {
                it = asyncMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
    };

    std::vector<Asset<T>> CollectFinishedAsync()
    {
        std::vector<Asset<T>> vec;
        std::lock_guard<std::mutex> lc(mutex);
        for (auto it = asyncMap.begin(); it != asyncMap.end();)
        {
            if (it->second->status == ELoadingSts::SUCCESS)
            {
                vec.emplace_back(it->second->asset);
                it = asyncMap.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return std::move(vec);
    }

private:
    std::list<std::shared_ptr<Fleur::AsyncOperation<T>>> asyncOperationsToRelease;
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

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
            return m_Image2DCache.LoadAsync(path, m_GlobalId++);
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
            return m_ModelCache.LoadAsync(path, m_GlobalId++);
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
        std::mutex mx;
    };
    ImagesUpload m_ImagesToUpload;
};


template <>
Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetCache<Fleur::Graphics::Image2D>::Load(std::string_view path, AssetID id);
template <>
std::shared_ptr<AsyncOperation<Fleur::Graphics::Image2D>> Fleur::AssetCache<Fleur::Graphics::Image2D>::LoadAsync(std::string_view path, AssetID id);
template <>
Fleur::Asset<Fleur::Graphics::Model> Fleur::AssetCache<Fleur::Graphics::Model>::Load(std::string_view path, AssetID id);
template <>
std::shared_ptr<AsyncOperation<Fleur::Graphics::Model>> Fleur::AssetCache<Fleur::Graphics::Model>::LoadAsync(std::string_view path, AssetID id);

}  // namespace Fleur
