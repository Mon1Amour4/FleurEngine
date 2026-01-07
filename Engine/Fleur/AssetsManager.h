#pragma once

#include <filesystem>
#include <type_traits>

#include "../External/tbb/include/oneapi/tbb/concurrent_unordered_map.h"
#include "Renderer/Color.h"
#include "Renderer/Shader.h"
#include "Services/ServiceInterfaces.hpp"

#define SHARED_RES(Res) std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Res>>
#define CONST_SHARED_RES(Res) const std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Res>>
#define ASSET_HANDLE(Res) const std::shared_ptr<Fleur::ResourceHandle<Res>>

using AssetID = uint32_t;

namespace Fleur::Graphics
{
class Image2D;
class CubemapImage;
class Model;
}  // namespace Fleur::Graphics

namespace Fleur
{

template <typename T>
class AssetHandle
{
    friend class AssetsManager;

public:
    AssetHandle() = default;

private:
    uint32_t ID;
};

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

template <typename T>
class ResourceHandle
{
public:
    ResourceHandle(std::shared_ptr<T> resource, ELoadingSts status, std::optional<EFailure> failure = std::nullopt)
        : m_Obj(resource)
        , m_Status(status)
        , m_Failure(failure) {};

    ResourceHandle(std::shared_ptr<T> resource)
        : m_Obj(resource)
        , m_Status(m_Status) {};
    ResourceHandle() = default;
    ~ResourceHandle() = default;

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

class AssetsManager : public Service<AssetsManager>
{
public:
    friend class Application;
    friend struct Service<AssetsManager>;

    AssetsManager();
    ~AssetsManager();

    void OnShutdown();
    void OnInit();

    [[nodiscard]] CONST_SHARED_RES(Image2D) LoadImage2DFromMemory(std::string_view name, bool flipVertical, unsigned char* data, size_t sizeBytes);
    [[nodiscard]] CONST_SHARED_RES(Image2D) LoadImage2DFromMemoryAsync(std::string_view name, bool flipVertical, unsigned char* data, size_t sizeBytes);

    [[nodiscard]] CONST_SHARED_RES(Image2D)
        LoadImage2DFromRawData(std::string_view name, unsigned char* data, uint16_t channels, uint32_t width, uint32_t height);

    [[nodiscard]] AssetID LoadImage2DFromColor(std::string_view name, Fleur::Graphics::Color color, uint32_t width, uint32_t height);

    template <class Res>
    std::shared_ptr<Fleur::ResourceHandle<Res>> Load(std::string_view path, bool flipVertical = false, bool async = true)
    {
        std::shared_ptr<Fleur::ResourceHandle<Res>> result{nullptr};
        if constexpr (std::is_same_v<std::remove_cv_t<Res>, Fleur::Graphics::Image2D>)
        {
            if (async)
                return load_image2d_async(path, flipVertical);
            else
                return load_image2d(path, flipVertical);
        }
        else if constexpr (std::is_same_v<std::remove_cv_t<Res>, Fleur::Graphics::Model>)
        {
            if (async)
                return load_model_async(path);
            else
                return load_model(path);
        }
        else if constexpr (std::is_same_v<std::remove_cv_t<Res>, Fleur::Graphics::CubemapImage>)
        {
            if (async)
                return load_cubemap_image_async(path, flipVertical);
            else
                return load_cubemap_image(path, flipVertical);
        }
        FL_CORE_ASSERT(false, "");
        return std::shared_ptr<Fleur::ResourceHandle<Res>>{};
    }

    template <class Res>
    [[nodiscard]] std::weak_ptr<Res> Get(std::string_view name)
    {
        if (name.empty())
            return std::weak_ptr<Res>{};

        if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fleur::Graphics::Model>>::value)
        {
            auto it = m_Models.find(name.data());
            if (it != m_Models.end())
                return std::weak_ptr<Res>(it->second);
            else
                return std::weak_ptr<Res>{};
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fleur::Graphics::Image2D>>::value)
        {
            auto it = m_Images2D.find(name.data());
            if (it != m_Images2D.end())
                return std::weak_ptr<Res>(it->second);
            else
                return std::weak_ptr<Res>{};
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fleur::Graphics::CubemapImage>>::value)
        {
            auto it = m_CubemapImages.find(name.data());
            if (it != m_CubemapImages.end())
                return std::weak_ptr<Res>(it->second);
            else
                return std::weak_ptr<Res>{};
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fleur::Graphics::Shader>>::value)
        {
            auto it = m_ShaderMap.find(name.data());
            if (it != m_ShaderMap.end())
                return std::weak_ptr<Res>(it->second);
            else
                return std::weak_ptr<Res>{};
        }
        else
            FL_CORE_ASSERT(nullptr, "[Assets manager] wront graphics resource type")
    }

    template <class Res>
    void Unload(std::string_view resourceName)
    {
        if (resourceName.empty())
            return;

        std::string name = std::filesystem::path(resourceName).stem().string();

        if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fleur::Graphics::Model>>::value)
        {
            auto it = m_Models.find(name);
            if (it != m_Models.end())
            {
                FL_CORE_INFO("[Assets manager] Model: {0} has been erased", it->first);
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_Models.unsafe_erase(it);
            }
            --m_ModelsCount;
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fleur::Graphics::Image2D>>::value)
        {
            auto it = m_Images2D.find(name);
            if (it != m_Images2D.end())
            {
                FL_CORE_INFO("[Assets manager] Image2D: {0} has been erased", it->first);
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_Images2D.unsafe_erase(it);
            }
            --m_Images2DCount;
        }
        else
            FL_CORE_ASSERT(nullptr, "[Assets manager] wront graphics resource type");
    }

    template <typename T>
    AssetID MakeHandle(std::string_view name)
    {
        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
            AssetID ID = m_StaticID;
            auto pair = m_ImagesMap.emplace(m_StaticID, Fleur::Graphics::Image2D(name));
            m_AssetToLoadID.push_back(ID);
            m_StaticID++;

            return ID;
        }
    }

    AssetID LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size);

private:
    tbb::concurrent_unordered_map<std::string, std::shared_ptr<Fleur::Graphics::Model>> m_Models;

    tbb::concurrent_unordered_map<std::string, std::shared_ptr<Fleur::Graphics::Image2D>> m_Images2D;
    tbb::concurrent_unordered_map<std::string, std::shared_ptr<Fleur::Graphics::CubemapImage>> m_CubemapImages;

    //  TODO: What to do with corrupted models?
    tbb::concurrent_unordered_map<std::string, CONST_SHARED_RES(Model)> m_ModelsToLoadAsync;
    tbb::concurrent_unordered_map<std::string, CONST_SHARED_RES(Image2D)> m_Images2DToLoadAsync;
    tbb::concurrent_unordered_map<std::string, CONST_SHARED_RES(CubemapImage)> m_CubemapImagesToLoadAsync;

    [[nodiscard]] CONST_SHARED_RES(Model) load_model(std::string_view path);
    [[nodiscard]] CONST_SHARED_RES(Model) load_model_async(std::string_view path);

    [[nodiscard]] CONST_SHARED_RES(Image2D) load_image2d(std::string_view path, bool flipVertical);
    [[nodiscard]] CONST_SHARED_RES(Image2D) load_image2d_async(std::string_view path, bool flipVertical);

    [[nodiscard]] CONST_SHARED_RES(CubemapImage) load_cubemap_image(std::string_view path, bool flipVertical);
    [[nodiscard]] CONST_SHARED_RES(CubemapImage) load_cubemap_image_async(std::string_view path, bool flipVertical);

    std::atomic<uint32_t> m_ModelsCount;
    std::atomic<uint32_t> m_Images2DCount;
    std::atomic<uint32_t> m_CubemapImagesCount;

    uint16_t ImageChannels(std::string_view image2DExt);

    template <typename Map>
    bool is_already_loaded(const Map& map, const std::string& key, std::shared_ptr<ResourceHandle<typename Map::mapped_type::element_type>>& OUT handleOut)
    {
        auto it = map.find(key);
        if (it != map.end())
        {
            handleOut = std::make_shared<ResourceHandle<typename Map::mapped_type::element_type>>(it->second);
            handleOut->SetSuccess();
            return true;
        }
        return false;
    }

    std::unordered_map<std::string, std::shared_ptr<Fleur::Graphics::Shader>> m_ShaderMap;
    void load_all_shaders();

    std::unordered_map<uint32_t, Fleur::Graphics::Image2D> m_ImagesMap;
    std::vector<uint32_t> m_AssetToLoadID;
    static std::atomic<uint32_t> m_StaticID;
};


}  // namespace Fleur
