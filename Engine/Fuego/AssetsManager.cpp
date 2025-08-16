#include "AssetsManager.h"

#if !defined(CGLTF_IMPLEMENTATION)
#define CGLTF_IMPLEMENTATION
#include "External/cgltf/cgltf.h"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../External/stb_image/stb_image_write.h"

// #define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image/stb_image.h"
#include "FileSystem/FileSystem.h"
#include "Services/ServiceLocator.h"

using Model = Fuego::Graphics::Model;
using Texture = Fuego::Graphics::Texture;
using Image2D = Fuego::Graphics::Image2D;
using CubemapImage = Fuego::Graphics::CubemapImage;

Fuego::AssetsManager::AssetsManager()
    : models_count(0)
    , images2d_count(0)
{
    models.reserve(10);
    images2d.reserve(10);
}

Fuego::AssetsManager::~AssetsManager()
{
    models.clear();
    images2d.clear();
}

// Models:
CONST_SHARED_RES(Model) Fuego::AssetsManager::load_model(std::string_view path)
{
    SHARED_RES(Model) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(models, file_name, handle);
    if (loaded)
        return handle;

    auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

    handle = std::make_shared<Fuego::ResourceHandle<Model>>(std::make_shared<Model>(file_name));

    auto res = fs->GetFullPathToFile(path);
    if (!res)
    {
        handle->SetCorrupted(WRONG_PATH);
        return handle;
    }

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
    if (result != cgltf_result_success)
    {
        handle->SetCorrupted(NO_DATA);
        return handle;
    }
    result = cgltf_load_buffers(&options, data, res->c_str());
    handle = std::make_shared<Fuego::ResourceHandle<Model>>(std::make_shared<Model>(file_name, data));
    handle->SetSuccess();
    models.emplace(std::move(file_name), handle->Resource());
    ++models_count;

    FU_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", models.size(), handle->Resource()->GetName());

    cgltf_free(data);
    return handle;
}
CONST_SHARED_RES(Model) Fuego::AssetsManager::load_model_async(std::string_view path)
{
    SHARED_RES(Model) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(models, file_name, handle);
    if (loaded)
        return handle;
    {
        const auto it = models_to_load_async.find(file_name);
        if (it != models_to_load_async.end() && it->second->Status() != CORRUPTED)
            return it->second;
    }

    handle = models_to_load_async.emplace(file_name, std::make_shared<Fuego::ResourceHandle<Model>>(nullptr)).first->second;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    thread_pool->Submit(
        [this](std::string_view path, std::string_view file_name, std::shared_ptr<Fuego::ResourceHandle<Model>> handle)
        {
            handle->SetStatus(LoadingSts::LOADING);

            auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

            auto res = fs->GetFullPathToFile(path);
            if (!res)
            {
                handle->SetCorrupted(WRONG_PATH);
                return;
            }

            cgltf_options options = {};
            cgltf_data* data = NULL;
            cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
            if (result != cgltf_result_success)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }
            result = cgltf_load_buffers(&options, data, res->c_str());
            if (result != cgltf_result_success)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }

            handle->SetResource(std::make_shared<Model>(file_name, data));
            handle->SetSuccess();
            models.emplace(file_name, handle->Resource());
            FU_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", models.size(), file_name);
            ++models_count;

            auto it = models_to_load_async.find(file_name.data());
            if (it != models_to_load_async.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                models_to_load_async.unsafe_erase(it);
            }
            cgltf_free(data);
        },
        path, file_name, handle);

    return handle;
}

// Image:
CONST_SHARED_RES(Image2D) Fuego::AssetsManager::load_image2d(std::string_view path, bool flip_vertical)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    bool loaded = is_already_loaded(images2d, file_name, handle);
    if (loaded)
        return handle;

    auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

    auto res = fs->GetFullPathToFile(path);
    if (!res)
        return std::make_shared<Fuego::ResourceHandle<Image2D>>(nullptr, CORRUPTED, WRONG_PATH);

    int w, h, channels = 0;
    unsigned char* img_data = stbi_load(res.value().c_str(), &w, &h, &channels, 0);
    if (!img_data)
    {
        FU_CORE_ERROR("Can't load an image: {0} {1}", path, stbi_failure_reason());
        return std::make_shared<Fuego::ResourceHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);
    }

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flip_vertical));

    auto img = images2d.emplace(file_name, std::make_shared<Image2D>(file_name, ext, img_data, w, h, channels, 1)).first->second;
    FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++images2d_count, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fuego::ResourceHandle<Image2D>>(img, SUCCESS);
    stbi_image_free(img_data);
    return handle;
}

CONST_SHARED_RES(Image2D) Fuego::AssetsManager::load_image2d_async(std::string_view path, bool flip_vertical)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    bool loaded = is_already_loaded(images2d, file_name, handle);
    if (loaded)
        return handle;
    {
        auto it = images2d_to_load_async.find(file_name);
        if (it != images2d_to_load_async.end() && it->second->Status() != CORRUPTED)
            return it->second;
    }

    handle =
        images2d_to_load_async.emplace(file_name, std::make_shared<Fuego::ResourceHandle<Image2D>>(std::make_shared<Image2D>(file_name, ext))).first->second;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    thread_pool->Submit(
        [this](std::shared_ptr<Fuego::ResourceHandle<Image2D>> handle, bool flip_vertical)
        {
            auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();
            auto img = handle->Resource();

            int w, h, channels = 0;

            std::filesystem::path full_path = img->Name();
            full_path.replace_extension(img->Ext());

            auto res = fs->GetFullPathToFile(full_path.string());
            if (!res)
            {
                handle->SetCorrupted(WRONG_PATH);
                return;
            }

            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flip_vertical));
            unsigned char* img_data = stbi_load(res.value().c_str(), &w, &h, &channels, 0);

            if (!img_data)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }
            Fuego::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(channels), 1, img_data};
            handle->Resource()->PostCreate(settings);

            auto image = images2d.emplace(handle->Resource()->Name(), handle->Resource()).first->second;
            FU_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", image->Name());
            ++images2d_count;
            handle->SetSuccess();

            auto it = images2d_to_load_async.find(handle->Resource()->Name().data());
            if (it != images2d_to_load_async.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                images2d_to_load_async.unsafe_erase(it);
            }

            stbi_image_free(img_data);
        },
        handle, flip_vertical);
    return handle;
}

CONST_SHARED_RES(Image2D) Fuego::AssetsManager::LoadImage2DFromMemory(std::string_view name, bool flip_vertical, unsigned char* data, uint32_t size_b)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (!data)
        return handle;

    std::string file_name = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(images2d, file_name, handle);
    if (loaded)
        return handle;

    int w, h, channels = 0;
    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flip_vertical));
    unsigned char* img_data = stbi_load_from_memory(data, size_b, &w, &h, &channels, 0);

    if (!img_data)
        return std::make_shared<Fuego::ResourceHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);

    auto img = images2d.emplace(file_name, std::make_shared<Image2D>(file_name, ext, img_data, w, h, channels, 1)).first->second;
    FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++images2d_count, img->Name(), img->Width(), img->Height());
    return std::make_shared<Fuego::ResourceHandle<Image2D>>(img, SUCCESS);

    stbi_image_free(img_data);
}

CONST_SHARED_RES(Image2D) Fuego::AssetsManager::LoadImage2DFromMemoryAsync(std::string_view name, bool flip_vertical, unsigned char* data, uint32_t size_b)
{
    std::shared_ptr<Fuego::ResourceHandle<Fuego::Graphics::Image2D>> handle{nullptr};
    if (!data)
        return handle;

    std::string file_name = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(images2d, file_name, handle);
    if (loaded)
        return handle;

    auto it = images2d_to_load_async.find(file_name);
    if (it != images2d_to_load_async.end() && it->second->Status() != CORRUPTED)
        return it->second;

    handle =
        images2d_to_load_async.emplace(file_name, std::make_shared<Fuego::ResourceHandle<Image2D>>(std::make_shared<Image2D>(file_name, ext))).first->second;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();
    thread_pool->Submit(
        [this](std::shared_ptr<Fuego::ResourceHandle<Image2D>> handle, bool flip_vertical, unsigned char* data, uint32_t size_b)
        {
            if (!data)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }

            int w, h, channels = 0;
            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flip_vertical));
            unsigned char* img_data = stbi_load_from_memory(data, size_b, &w, &h, &channels, 0);

            if (!img_data)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }

            auto it = images2d_to_load_async.find(handle->Resource()->Name().data());
            if (it != images2d_to_load_async.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                images2d_to_load_async.unsafe_erase(it);
            }

            Fuego::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(channels), 1, img_data};
            handle->Resource()->PostCreate(settings);
            auto image = images2d.emplace(handle->Resource()->Name(), handle->Resource()).first->second;
            FU_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", image->Name());
            ++images2d_count;
            handle->SetSuccess();

            {
                auto it = images2d_to_load_async.find(handle->Resource()->Name().data());
                if (it != images2d_to_load_async.end())
                {
                    std::mutex mtx;
                    std::lock_guard<std::mutex> lock(mtx);
                    images2d_to_load_async.unsafe_erase(it);
                }
            }

            stbi_image_free(img_data);
        },
        handle, flip_vertical, data, size_b);
    return handle;
}

CONST_SHARED_RES(Image2D)
Fuego::AssetsManager::LoadImage2DFromRawData(std::string_view name, unsigned char* data, uint32_t channels, uint32_t width, uint32_t height)
{
    std::shared_ptr<Fuego::ResourceHandle<Fuego::Graphics::Image2D>> handle{nullptr};
    if (!data || name.empty())
        return handle;

    std::string file_name = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(images2d, file_name, handle);
    if (loaded)
        return handle;

    auto img = images2d.emplace(file_name, std::make_shared<Image2D>(file_name, ext, data, width, height, channels, 1)).first->second;
    FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++images2d_count, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fuego::ResourceHandle<Image2D>>(img, SUCCESS);
    return handle;
}

CONST_SHARED_RES(Image2D) Fuego::AssetsManager::LoadImage2DFromColor(std::string_view name, Fuego::Graphics::Color color, uint32_t width, uint32_t height)
{
    std::shared_ptr<Fuego::ResourceHandle<Fuego::Graphics::Image2D>> handle{nullptr};
    if (name.empty())
        return handle;

    uint32_t channels = Fuego::Graphics::Color::Channels(color);
    size_t size = width * height * channels;

    uint32_t color_data = color.Data();

    unsigned char* data = new unsigned char[size];
    for (size_t i = 0; i < width * height; ++i)
    {
        std::memcpy(data + i * channels, &color_data, channels);
    }
    auto img = images2d.emplace(name, std::make_shared<Image2D>(name, "-", data, width, height, channels, 1)).first->second;

    return std::make_shared<Fuego::ResourceHandle<Image2D>>(img, LoadingSts::SUCCESS);
}

// CubemapImage:
CONST_SHARED_RES(CubemapImage) Fuego::AssetsManager::load_cubemap_image(std::string_view path, bool flip_vertical)
{
    std::shared_ptr<Fuego::ResourceHandle<Fuego::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(cubemap_images, file_name, handle);
    if (loaded)
        return handle;

    SHARED_RES(Image2D) image2d = load_image2d(path, flip_vertical);
    Fuego::Graphics::Image2D cross_layout = image2d->Resource()->FromEquirectangularToCross();
    auto cubemap_img = cubemap_images.emplace(file_name, std::make_shared<CubemapImage>(cross_layout.FromCrossToCubemap())).first->second;
    ++cubemap_images_count;
    FU_CORE_INFO("CubemapImage was emplaced: {0}", cubemap_img->Name());
}

CONST_SHARED_RES(CubemapImage) Fuego::AssetsManager::load_cubemap_image_async(std::string_view path, bool flip_vertical)
{
    std::shared_ptr<Fuego::ResourceHandle<Fuego::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    SHARED_RES(Image2D) image_handle = load_image2d_async(path, flip_vertical);

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    handle = cubemap_images_to_load_async.emplace(path, std::make_shared<Fuego::ResourceHandle<CubemapImage>>()).first->second;

    thread_pool->Submit(
        [this](std::shared_ptr<Fuego::ResourceHandle<Image2D>> img_handle, std::shared_ptr<Fuego::ResourceHandle<CubemapImage>> cubemap_handle,
               bool flip_vertical)
        {
            auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

            while (img_handle->Status() != LoadingSts::SUCCESS && img_handle->Status() != LoadingSts::CORRUPTED)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (img_handle->Status() == LoadingSts::CORRUPTED)
            {
                cubemap_handle->SetCorrupted(img_handle->FailureReason().value());
                return;
            }


            // Determine is it cross layout or equirectangular image:
            uint32_t image_ration = img_handle->Resource()->Width() / img_handle->Resource()->Height();
            if (image_ration == 2)
            {
                // equirectangular image
                auto cross_layout = images2d
                                        .emplace(img_handle->Resource()->Name().data() + std::string("_cross_layout"),
                                                 std::make_shared<Image2D>(img_handle->Resource()->FromEquirectangularToCross()))
                                        .first->second;


                stbi_write_jpg("D:\\Engine\\GameEngine\\Sandbox\\Resources\\Images\\MyTestCross.jpg", cross_layout->Width(), cross_layout->Height(),
                               cross_layout->Channels(), cross_layout->Data(), 100 /* 1-100 */);

                FU_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", cross_layout->Name());
                ++images2d_count;
                Fuego::Graphics::CubemapImage cubemap = cross_layout->FromCrossToCubemap();
                cubemap_handle->SetResource(std::make_shared<CubemapImage>(std::move(cubemap)));
                cubemap_handle->SetSuccess();
            }
            else
            {
                Fuego::Graphics::CubemapImage cubemap = img_handle->Resource()->FromCrossToCubemap();
                cubemap_handle->SetResource(std::make_shared<CubemapImage>(std::move(cubemap)));
                cubemap_handle->SetSuccess();
            }

            auto image = cubemap_images.emplace(cubemap_handle->Resource()->Name(), cubemap_handle->Resource());

            FU_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", cubemap_handle->Resource()->Name());
            ++cubemap_images_count;

            auto it = cubemap_images_to_load_async.find(cubemap_handle->Resource()->Name().data());
            if (it != cubemap_images_to_load_async.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                cubemap_images_to_load_async.unsafe_erase(it);
            }
        },
        image_handle, handle, flip_vertical);
    return handle;
}

// Other:
uint16_t Fuego::AssetsManager::ImageChannels(std::string_view image2d_ext)
{
    if (image2d_ext.empty() || image2d_ext.size() > 3)
        return static_cast<uint16_t>(3);

    if (image2d_ext.compare("jpg"))
    {
        return static_cast<uint16_t>(3);
    }
    else if (image2d_ext.compare("png"))
    {
        return static_cast<uint16_t>(4);
    }
    else
    {
        return static_cast<uint16_t>(3);
    }
}
