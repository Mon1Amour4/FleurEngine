#pragma once

#include <filesystem>
#include <type_traits>

#include "AssetTypes.hpp"
#include "Renderer/Color.h"
#include "Renderer/RenderViews.hpp"
#include "Renderer/Shader.h"
#include "Renderer/Skybox.h"
#include "Services/ServiceInterfaces.hpp"

using ImageType = Fleur::Graphics::Image2D;
using ShaderType = Fleur::Graphics::Shader;
using ModelType = Fleur::Graphics::Model;
using CubemapType = Fleur::Graphics::CubemapImage;

using ImageRecord = Fleur::AssetRecord<Fleur::Graphics::Image2D>;
using ModelRecord = Fleur::AssetRecord<Fleur::Graphics::Model>;
using CubemapRecord = Fleur::AssetRecord<Fleur::Graphics::CubemapImage>;

using ImageAsset = Fleur::Asset<ImageType>;
using ModelAsset = Fleur::Asset<ModelType>;
using CubemapAsset = Fleur::Asset<CubemapType>;

using ImageAsyncOp = Fleur::AsyncOperation<ImageType>;
using ModelAsyncOp = Fleur::AsyncOperation<ModelType>;
using CubemapAsyncOp = Fleur::AsyncOperation<CubemapType>;

using ImageAsyncOpShared = std::shared_ptr<ImageAsyncOp>;
using ModelAsyncOpShared = std::shared_ptr<ModelAsyncOp>;
using CubemapAsyncOpShared = std::shared_ptr<CubemapAsyncOp>;


namespace Fleur
{
enum ImageSource
{
    IMAGE_SOURCE_MEMORY,
    IMAGE_SOURCE_COLOR,
    IMAGE_SOURCE_DISK,
    IMAGE_SOURCE_MAX_VALUE
};
enum GammaCorrection
{
    GAMMA_CORRECTION_SRGB,
    GAMMA_CORRECTION_LINEAR,
    GAMMA_CORRECTION_MAX_VALUE
};
struct ImageImportSettings
{
    bool flip{};
    ImageSource imageSource{IMAGE_SOURCE_MAX_VALUE};
    GammaCorrection gammaCorrection{GAMMA_CORRECTION_MAX_VALUE};

    Fleur::Graphics::Color color;

    unsigned char* pMemoryData;
    size_t sizeInMemory;
};
enum CubemapSourceLayout
{
    CUBEMAP_SOURCE_LAYOUT_6_IMAGES,
    CUBEMAP_SOURCE_LAYOUT_EQUIRECTANGULAR_IMAGE,
    CUBEMAP_SOURCE_LAYOUT_CROSS_LAYOUT,
    CUBEMAP_SOURCE_LAYOUT_MAX_VALUE
};
struct CubemapImportSettings
{
    bool flip{};
    CubemapSourceLayout sourceLayout{CUBEMAP_SOURCE_LAYOUT_MAX_VALUE};
};

enum AssetEventType
{
    EVENT_TYPE_MODEL_LOADED,
    EVENT_TYPE_IMAGE2D_LOADED,
    EVENT_TYPE_CUBEMAP_LOADED,
    EVENT_TYPE_MAX_VALUE,
};
struct AssetLoadResult
{
    AssetEventType type = EVENT_TYPE_MAX_VALUE;
    const void* pResource = nullptr;
    AssetID ID = 0;
    EAsyncOperationStatus loadingStatus = LOADING_STATUS_MAX_VALUE;
};

class AssetsManager;
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
    TRecord Register(std::string_view path, AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);

        std::string name = GetNameFromPath(path);
        TRecord record = Exist(name);
        if (record.alreadyExist)
            return record;

        stringMap.emplace(name, id);
        T* ptr = &map.emplace(id, name).first->second;
        m_size++;

        return {true, false, {id, ptr}};
    };
    TAsyncOpShared RegisterAsync(std::string_view path, AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);

        std::string name = GetNameFromPath(path);

        // 1. Check async map first
        TAsyncOpShared asyncOperation = FindExistingAsyncOperation(name);
        if (asyncOperation.get()->status.GetStatus() != EAsyncOperationStatus::LOADING_STATUS_MAX_VALUE)
            return asyncOperation;

        // 2. Check map
        TRecord completedRecord = Exist(name);
        if (completedRecord.alreadyExist)
            return std::make_shared<TAsyncOp>(completedRecord.asset, EAsyncOperationStatus::LOADED);

        T* ptr = &map.emplace(id, name).first->second;
        stringMap.emplace(name, id);
        m_size++;
        return asyncMap.emplace(name, std::make_shared<TAsyncOp>(TAsset(id, ptr), REGISTERED)).first->second;
    };

    TAsyncOpShared FindExistingAsyncOperation(std::string_view name)
    {
        // Check only async map is enought
        if (auto operation = asyncMap.find(name.data()); operation != asyncMap.end())
            return operation->second;
        else
            return {std::make_shared<TAsyncOp>(TAsset(0, nullptr), EAsyncOperationStatus::LOADING_STATUS_MAX_VALUE)};
    }

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
            if (auto operation = asyncMap.find(GetNameFromPath(name)); operation != asyncMap.end())
            {
                asyncOperationsToRelease.emplace(operation->first, operation->second);
                operation->second->status.SetStatus(Fleur::EAsyncOperationStatus::LOADING_STATUS_TO_TERMINATE);
                return;
            }
            stringMap.erase(name.data());
            map.erase(id);
        }
    };
    void Release(AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto record = map.find(id); record != map.end())
        {
            std::string_view name = record->second.GetName();
            if (auto operation = asyncMap.find(name.data()); operation != asyncMap.end())
            {
                asyncOperationsToRelease.emplace(operation->first, operation->second);
                operation->second->status.SetStatus(Fleur::EAsyncOperationStatus::LOADING_STATUS_TO_TERMINATE);
                return;
            }
            stringMap.erase(name.data());
            map.erase(id);
        }
    };
    void RemoveBrokenAsyncAsset(AssetID id)
    {
        std::lock_guard<std::mutex> lc(mutex);
        if (auto record = map.find(id); record != map.end())
        {
            std::string name = record->second.GetName().data();
            asyncMap.erase(name);
            stringMap.erase(name);
            map.erase(id);
            asyncOperationsToRelease.erase(name);
        }
    };
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

    // ---------- Sync ----------
    ModelAsset LoadModel(std::string_view path);
    ImageAsset LoadImage(std::string_view path, ImageImportSettings imageSettings);
    CubemapAsset LoadCubemap(std::string_view path, CubemapImportSettings settings);

    // ---------- Async ----------
    ModelAsyncOpShared LoadModelAsync(std::string_view path, std::function<void(ModelAsset&)> callback = nullptr);
    ImageAsyncOpShared LoadImageAsync(std::string_view path, ImageImportSettings imageSettings, std::function<void(ImageAsset&)> callback = nullptr);
    CubemapAsyncOpShared LoadCubemapAsync(std::string_view path, CubemapImportSettings cubemapSettings, std::function<void(CubemapAsset&)> callback = nullptr);

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
    Fleur::AssetCache<Fleur::Graphics::CubemapImage> m_CubemapCache;


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

    AssetID GetNextID();

    void LoadModelInternal(std::string_view path, ModelAsset* asset);
    void LoadImageInternal(std::string_view path, ImageAsset* asset, ImageImportSettings& imageSettings);

    void LoadImageFromDisk(std::string_view path, ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings);
    void LoadImageFromColor(ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings);
    void LoadImageFromMemory(ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings);

    void LoadModelAsyncInternal(std::string_view path, ModelAsyncOpShared sharedOperation, AssetLoadCallback& internalCallback,
                                std::function<void(ModelAsset&)> clientCallback);

    void LoadImageAsyncInternal(std::string_view path, ImageAsyncOpShared sharedOperation, ImageImportSettings& imageSettings, AssetLoadCallback& callback,
                                std::function<void(ImageAsset&)> clientCallback);

    void LoadCubemapAsyncInternal(std::string_view path, CubemapAsyncOpShared sharedOperation, CubemapImportSettings& cubemapSettings,
                                  AssetLoadCallback& internalCallback, std::function<void(CubemapAsset&)> clientCallback);

    constexpr static std::string_view CORRUPTED_ASSET_ERROR_MESSAGE = "[AssetsManager] Error occured during asset loading";
};

}  // namespace Fleur
