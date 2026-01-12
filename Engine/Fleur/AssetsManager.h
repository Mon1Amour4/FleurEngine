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
    std::shared_ptr<AsyncOperation<T>> LoadAsync(std::string_view path)
    {
        // if (path.empty())
        //     return nullptr;

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
            return m_Image2DCache.LoadAsync(path, m_GlobalId++);
    }

    template <typename T>
    Asset<T> Load(std::string_view path)
    {
        if (path.empty())
            return Asset<T>{0, nullptr};

        if constexpr (std::is_same_v<T, Fleur::Graphics::Image2D>)
            return m_Image2DCache.Load(path, m_GlobalId++);
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

        return Asset<T>({0, nullptr});
    }

private:
    std::atomic<uint32_t> m_GlobalId = 1;

    void load_all_shaders();
    std::unordered_map<std::string, AssetID> m_ShaderMapString;
    std::unordered_map<AssetID, Fleur::Graphics::Shader> m_ShaderMap;

    AssetCache<Fleur::Graphics::Image2D> m_Image2DCache;
    AssetCache<Fleur::Graphics::Model> m_ModelCache;
};


}  // namespace Fleur
