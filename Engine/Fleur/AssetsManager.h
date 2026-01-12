#pragma once

#include <filesystem>
#include <type_traits>

#include "Renderer/Color.h"
#include "Renderer/Model.h"
#include "Renderer/Image2D.h"
#include "Renderer/RenderViews.hpp"
#include "Renderer/Shader.h"
#include "Services/ServiceInterfaces.hpp"

namespace Fleur
{
#pragma region Structs&Enums

using AssetID = uint32_t;

enum ELoadingSts
{
    TO_BE_LOADED,
    LOADING,
    CORRUPTED,
    SUCCESS
};

template <typename T>
struct Asset
{
    friend class AssetsManager;

    AssetID ID;
    T* obj;
};

template <typename T>
struct AsyncOperation
{
    Asset<T> asset;
    ELoadingSts status;
};

template<typename T>
class AssetCache
{
public:
    Fleur::Asset<T> Load(std::string_view name, AssetID id) {};
    std::shared_ptr<AsyncOperation<T>> LoadAsync(std::string_view path, AssetID id) {};
    Asset<T> Add(std::string_view name, AssetID id) {};

    // Get
    // Release

    private:
    //std::vector<AsyncOperation<T>> operations;
    std::unordered_map<std::string, AssetID> stringMap;
    std::unordered_map<AssetID, T> map;
    std::atomic<uint32_t> m_size;
};

template <>
class AssetCache<Fleur::Graphics::Image2D>
{
    using type = Fleur::Graphics::Image2D;

public:
    Fleur::Asset<type> Load(std::string_view name, AssetID id);
    std::shared_ptr<AsyncOperation<type>> LoadAsync(std::string_view path, AssetID id);
    Asset<type> Add(std::string_view name, AssetID id);
    // Get
    // Release

    private:
    // std::vector<AsyncOperation<T>> operations;
    std::unordered_map<std::string, AssetID> stringMap;
    std::unordered_map<AssetID, type> map;
    std::atomic<uint32_t> m_size;
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
    std::shared_ptr< AsyncOperation<T>> LoadAsync(std::string_view path)
    {
        //if (path.empty())
        //    return nullptr;

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
            return m_Image2DCache.LoadAsync(path);
    }

    Asset<Fleur::Graphics::Image2D> FromColor(std::string_view name, Fleur::Graphics::Color color);
    Asset<Fleur::Graphics::Image2D> LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size);

    template<typename T>
    Asset<T> Get(std::string_view name)
    {
        if (name.empty())
            return Asset<T>({0, nullptr});

        if constexpr (std::is_same_v<T, Fleur::Graphics::Shader>)
        {
            return m_ShaderMap.find(name.data())->second;
        }
        else if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
        {
        }
    }

private:
    std::atomic<uint32_t> m_GlobalId = 1;

    void load_all_shaders();
    std::unordered_map<std::string, Asset<Fleur::Graphics::Shader>> m_ShaderMap;

    AssetCache<Fleur::Graphics::Image2D> m_Image2DCache;
};


}  // namespace Fleur
