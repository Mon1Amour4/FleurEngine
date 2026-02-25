#pragma once

#include <type_traits>

#include "AssetCache.h"
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

enum CallbackInvocationPoint
{
    AFTER_CPU_LOAD,
    AFTER_GPU_UPLOAD,
    _MAX_VALUE
};

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
    AssetHandle handle{};
    EAsyncOperationStatus loadingStatus = LOADING_STATUS_MAX_VALUE;
    bool* notifyOnGpuUpload{nullptr};
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

class AssetsManager : public Service<AssetsManager>, public IUpdatable
{
public:
    // friend class Application;
    friend struct Service<AssetsManager>;

    AssetsManager();
    ~AssetsManager();

    /// Stops asset operations and releases resources managed by the service.
    void OnShutdown();
    /// Initializes the service and preloads built-in assets.
    void OnInit();
    /// Advances async loading and GPU upload state.
    void OnUpdate(float dtTime);

    // ---------- Sync ----------
    /// Loads a model immediately and returns an asset with a valid handle.
    ModelAsset LoadModel(std::string_view path);
    /// Loads a 2D image immediately and returns an asset with a valid handle.
    ImageAsset LoadImage(std::string_view path, ImageImportSettings imageSettings);
    /// Loads a cubemap immediately and returns an asset with a valid handle.
    CubemapAsset LoadCubemap(std::string_view path, CubemapImportSettings settings);

    // ---------- Async ----------
    /// Registers async model loading and returns operation state.
    ModelAsyncOpShared LoadModelAsync(std::string_view path, std::function<void(ModelAsset&)> callback = nullptr);
    /// Registers async image loading and returns operation state.
    ImageAsyncOpShared LoadImageAsync(std::string_view path, ImageImportSettings imageSettings, std::function<void(ImageAsset&)> callback = nullptr,
                                      CallbackInvocationPoint callbackType = _MAX_VALUE);
    /// Registers async cubemap loading and returns operation state.
    CubemapAsyncOpShared LoadCubemapAsync(std::string_view path, CubemapImportSettings cubemapSettings, std::function<void(CubemapAsset&)> callback = nullptr,
                                          CallbackInvocationPoint callbackType = _MAX_VALUE);

    template <typename T>
    /// Returns an asset by logical name.
    Asset<T> Get(std::string_view name)
    {
        if (name.empty())
            return Asset<T>({{}, nullptr});

        if constexpr (std::is_same_v<T, Fleur::Graphics::Shader>)
        {
            AssetID id = m_ShaderMapString[name.data()];
            return Fleur::Asset<T>{{id, 1}, &m_ShaderMap[id]};
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Get(name);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Get(name);
        }

        return Asset<T>({{}, nullptr});
    }
    template <typename T>
    /// Returns an asset by stable handle (ID + generation).
    Asset<T> Get(const AssetHandle& handle)
    {
        if (!handle.IsValid())
            return Asset<T>({{}, nullptr});

        if constexpr (std::is_same_v<T, Fleur::Graphics::Shader>)
        {
            if (auto it = m_ShaderMap.find(handle.id); it != m_ShaderMap.end())
                return Fleur::Asset<T>{handle, &it->second};
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Get(handle);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Get(handle);
        }

        return Asset<T>({{}, nullptr});
    }

    template <typename T>
    /// Releases an asset by logical name.
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
    /// Releases an asset by numeric ID.
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
    template <typename T>
    /// Releases an asset by handle if generation matches.
    void Release(const AssetHandle& handle)
    {
        if (!handle.IsValid())
            return;

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            return m_Image2DCache.Release(handle);
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
        {
            return m_ModelCache.Release(handle);
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
        std::list<bool*> notifyMap;

        bool ReadyToUpload()
        {
            return (framesSinceLastUpload > 5 && images.size() > 0) || images.size() > 10;
        };
        void Add(Fleur::Graphics::SFLImageView view, bool* notify)
        {
            std::lock_guard<std::mutex> lc(mx);
            images.push_back(view);
            if (notify)
                notifyMap.emplace_back(notify);
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

    bool m_ForceAlpha;
    void LoadModelInternal(std::string_view path, ModelAsset* asset);
    void LoadImageInternal(std::string_view path, ImageAsset* asset, ImageImportSettings& imageSettings);

    void LoadImageFromDisk(std::string_view path, ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings);
    void LoadImageFromColor(ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings);
    void LoadImageFromMemory(ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings);

    void LoadModelAsyncInternal(std::string_view path, ModelAsyncOpShared sharedOperation, AssetLoadCallback& internalCallback,
                                std::function<void(ModelAsset&)> clientCallback);

    void LoadImageAsyncInternal(std::string_view path, ImageAsyncOpShared sharedOperation, ImageImportSettings& imageSettings, AssetLoadCallback& callback,
                                std::function<void(ImageAsset&)> clientCallback, CallbackInvocationPoint callbackType);

    void LoadCubemapAsyncInternal(std::string_view path, CubemapAsyncOpShared sharedOperation, CubemapImportSettings& cubemapSettings,
                                  AssetLoadCallback& internalCallback, std::function<void(CubemapAsset&)> clientCallback, CallbackInvocationPoint callbackType);

    constexpr static std::string_view CORRUPTED_ASSET_ERROR_MESSAGE = "[AssetsManager] Error occured during asset loading";
};

}  // namespace Fleur
