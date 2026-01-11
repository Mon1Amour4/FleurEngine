#pragma once

#include <filesystem>
#include <type_traits>

#include "../External/tbb/include/oneapi/tbb/concurrent_unordered_map.h"
#include "Renderer/Color.h"
#include "Renderer/Image2D.h"
#include "Renderer/RenderViews.hpp"
#include "Renderer/Shader.h"
#include "Services/ServiceInterfaces.hpp"

#define SHARED_RES(Res) std::shared_ptr<Fleur::AsyncOperationHandle<Fleur::Graphics::Res>>
#define CONST_SHARED_RES(Res) const std::shared_ptr<Fleur::AsyncOperationHandle<Fleur::Graphics::Res>>
#define ASSET_HANDLE(Res) const std::shared_ptr<Fleur::AsyncOperationHandle<Res>>


namespace Fleur::Graphics
{
class Image2D;
class CubemapImage;
class Model;
}  // namespace Fleur::Graphics

namespace Fleur
{

#pragma region Structs&Enums

enum EFailure
{
    WRONG_PATH,
    NO_DATA
};

enum ELoadingSts
{
    TO_BE_LOADED,
    LOADING,
    CORRUPTED,
    SUCCESS
};

using AssetID = uint32_t;

template <typename T>
struct Asset
{
    AssetID ID;
    T* obj;
};

template <typename T>
struct AsyncOperation
{
    Asset<T> asset;
    ELoadingSts status;
};

template <typename T>
class AsyncOperationHandle
{
public:
    AsyncOperationHandle(std::shared_ptr<T> resource, ELoadingSts status, std::optional<EFailure> failure = std::nullopt)
        : m_Obj(resource)
        , m_Status(status)
        , m_Failure(failure) {};

    AsyncOperationHandle(std::shared_ptr<T> resource)
        : m_Obj(resource)
        , m_Status(m_Status) {};
    AsyncOperationHandle() = default;
    ~AsyncOperationHandle() = default;

    ELoadingSts Status()
    {
        return m_Status;
    }
    std::optional<EFailure> FailureReason()
    {
        return m_Failure;
    }
    void SetCorrupted(EFailure failure)
    {
        m_Status = CORRUPTED;
        failure = failure;
    }
    void SetSuccess()
    {
        m_Status = SUCCESS;
        m_Failure = std::nullopt;
    }
    void SetStatus(ELoadingSts st)
    {
        m_Status = st;
    }
    void SetFailure(EFailure fail)
    {
        m_Failure = fail;
    }
    const std::shared_ptr<T> Resource() const
    {
        return m_Obj;
    }
    void SetResource(std::shared_ptr<T> res)
    {
        m_Obj = res;
    }

private:
    std::shared_ptr<T> m_Obj;
    ELoadingSts m_Status{TO_BE_LOADED};
    std::optional<EFailure> m_Failure;
};

#pragma endregion

class AssetsManager : public Service<AssetsManager>, public IUpdatable
{
public:
    friend class Application;
    friend struct Service<AssetsManager>;

    AssetsManager();
    ~AssetsManager();

    void OnShutdown();
    void OnInit();
    void OnUpdate(float dtTime);

    // Load
    template <typename T>
    const AsyncOperation<T>* LoadAsync(std::string_view path)
    {
        if (path.empty())
            return nullptr;

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
            return load_async_image2D(path);
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
            return load_async_model(path);

        return nullptr;
    }

    template <typename T>
    const Asset<T> Load(std::string_view path)
    {
        if (path.empty())
            return nullptr;

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
            return load_async_image2D(path);
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Model>)
            return LoadModelAsync(path);

        return nullptr;
    }

    // Image2D

    Asset<Fleur::Graphics::Image2D> FromColor(std::string_view name, Fleur::Graphics::Color c);
    AssetID LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size);


    // Get
    template <typename T>
    [[nodiscard]] Asset<T> GetAsset(std::string_view name)
    {
        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            auto pair = m_StringToIDMap.find(name.data());
            if (auto search = m_StringToIDMap.find(name.data()); search != m_StringToIDMap.end())
            {
                return {search->second, &m_Image2DMap.find(search->second)->second};
            }
        }
        return {0, nullptr};
    }


private:
    // Image2D
    const Fleur::AsyncOperation<Fleur::Graphics::Image2D>* load_async_image2D(std::string_view path);
    Asset<Fleur::Graphics::Image2D> load_image2d(std::string_view path, bool flipVertical);
    // Model
    const Fleur::AsyncOperation<Fleur::Graphics::Model>* load_async_model(std::string_view path);

    std::atomic<bool> needToUploadResources;

    //[[nodiscard]] CONST_SHARED_RES(Model) load_model(std::string_view path);
    //[[nodiscard]] CONST_SHARED_RES(Model) load_model_async(std::string_view path);

    //[[nodiscard]] CONST_SHARED_RES(Image2D) load_image2d(std::string_view path, bool flipVertical);
    //[[nodiscard]] CONST_SHARED_RES(Image2D) load_image2d_async(std::string_view path, bool flipVertical);

    //[[nodiscard]] CONST_SHARED_RES(CubemapImage) load_cubemap_image(std::string_view path, bool flipVertical);
    //[[nodiscard]] CONST_SHARED_RES(CubemapImage) load_cubemap_image_async(std::string_view path, bool flipVertical);


    std::atomic<uint32_t> m_CubemapImagesCount;

    uint16_t ImageChannels(std::string_view image2DExt);


    std::unordered_map<std::string, std::shared_ptr<Fleur::Graphics::Shader>> m_ShaderMap;
    void load_all_shaders();

    std::vector<Fleur::Graphics::SFLImageView> m_ImagesToUpload;

    std::atomic<AssetID> m_AssetIDCounter = 0;
    std::unordered_map<std::string, AssetID> m_StringToIDMap;

    std::unordered_map<AssetID, Fleur::Graphics::Image2D> m_Image2DMap;
    std::unordered_map<AssetID, Fleur::Graphics::Model> m_ModelMap;

    std::unordered_map<AssetID, Fleur::AsyncOperation<Fleur::Graphics::Image2D>> m_Image2DAsyncOperationsMap;
    std::unordered_map<AssetID, Fleur::AsyncOperation<Fleur::Graphics::Model>> m_ModelAsyncOperationsMap;

    std::atomic<uint32_t> m_Images2DCount;
    std::atomic<uint32_t> m_ModelsCount;

    void AddImageToUpload(Fleur::Graphics::SFLImageView imageView);
};


}  // namespace Fleur
