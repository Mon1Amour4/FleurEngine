#pragma once

#include <filesystem>
#include <type_traits>

#include "Services/ServiceInterfaces.hpp"

namespace Fuego::Graphics
{
class Image2D;
class Model;
enum ImageFormat;
}  // namespace Fuego::Graphics

namespace Fuego
{


class AssetsManager : public Service<AssetsManager>
{
public:
    friend class Service<AssetsManager>;

    AssetsManager();
    ~AssetsManager();

    template <class Res, typename... Args>
    std::shared_ptr<Res> Load(std::string_view path, Args... args)
    {
        if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Model>>::value)
        {
            auto model = load_model(path);
            FU_CORE_ASSERT(model, "[Assets manager] cannot load model")
            return model;
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Image2D>>::value)
        {
            auto format = Fuego::Graphics::ImageFormat::RGB;
            if constexpr (sizeof...(Args) >= 1)
            {
                std::tuple<Args...> tuple(std::forward<Args>(args)...);
                format = std::get<0>(tuple);
            }
            auto img = load_image2d(path, format);
            FU_CORE_ASSERT(img, "[Assets manager] cannot load image2d");
            return img;
        }
        else
        {
            FU_CORE_ASSERT(nullptr, "[Assets manager] wront graphics resource type")
        }
    }

    template <class Res, typename... Args>
    void LoadAsync(std::string_view path, Args... args)
    {
        if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Model>>::value)
        {
            load_model_async(path);
        }
    }

    template <class Res>
    std::weak_ptr<Res> Get(std::string_view name)
    {
        if (name.empty())
            return std::weak_ptr<Res>{};

        if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Model>>::value)
        {
            auto it = models.find(name.data());
            if (it != models.end())
                return std::weak_ptr<Res>(it->second);
            else
                return std::weak_ptr<Res>{};
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Image2D>>::value)
        {
            auto it = images2d.find(name.data());
            if (it != images2d.end())
                return std::weak_ptr<Res>(it->second);
            else
                return std::weak_ptr<Res>{};
        }
        else
            FU_CORE_ASSERT(nullptr, "[Assets manager] wront graphics resource type")
    }

    template <class Res>
    void Unload(std::string_view res_name)
    {
        if (res_name.empty())
            return;

        std::string name = std::filesystem::path(res_name).stem().string();

        if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Model>>::value)
        {
            auto it = models.find(name);
            if (it != models.end())
            {
                FU_CORE_INFO("[Assets manager] Model: {0} has been erased", it->first);
                models.erase(it);
                --models_count;
            }
        }
        else if constexpr (std::is_same<std::remove_cv_t<Res>, std::remove_cv_t<Fuego::Graphics::Image2D>>::value)
        {
            auto it = images2d.find(name);
            if (it != images2d.end())
            {
                FU_CORE_INFO("[Assets manager] Image2D: {0} has been erased", it->first);
                images2d.erase(it);
                --images2d_count;
            }
        }
        else
        {
            FU_CORE_ASSERT(nullptr, "[Assets manager] wront graphics resource type")
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Fuego::Graphics::Model>> models;
    std::unordered_map<std::string, std::shared_ptr<Fuego::Graphics::Image2D>> images2d;

    std::shared_ptr<Fuego::Graphics::Model> load_model(std::string_view path);
    void load_model_async(std::string_view path);
    std::shared_ptr<Fuego::Graphics::Image2D> load_image2d(std::string_view path, Fuego::Graphics::ImageFormat format);

    uint32_t models_count;
    uint32_t images2d_count;

};


}  // namespace Fuego