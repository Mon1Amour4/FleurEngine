#include "AssetsManager.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

#include "External/stb_image/stb_image.h"
#include "FileSystem/FileSystem.h"
#include "Services/ServiceLocator.h"

#define ASSIMP_LOAD_FLAGS aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType

Fuego::AssetsManager::AssetsManager(Fuego::Pipeline::Toolchain::assets_manager& toolchain)
    : models_count(0)
    , images2d_count(0)
    , toolchain(toolchain)
{
    models.reserve(10);
    images2d.reserve(10);
}

Fuego::AssetsManager::~AssetsManager()
{
    models.clear();
    images2d.clear();
}

std::shared_ptr<Fuego::Graphics::Model> Fuego::AssetsManager::load_model(std::string_view path)
{
    if (path.empty())
        return std::shared_ptr<Fuego::Graphics::Model>();

    std::string file_name = std::filesystem::path(path.data()).filename().string();
    auto it = models.find(file_name);
    if (it != models.end())
        return it->second;

    auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

    Assimp::Importer importer{};
    const aiScene* scene = importer.ReadFile(fs->GetFullPathToFile(path.data()), ASSIMP_LOAD_FLAGS);
    if (!scene)
        return nullptr;

    models_async_operations.lock();
    auto model = models.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Model>(scene)).first->second;
    models_async_operations.unlock();
    ++models_count;

    FU_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", models.size(), model->GetName());
    return model;
}

void Fuego::AssetsManager::load_model_async(std::string_view path)
{
    std::string file_name = std::filesystem::path(path.data()).filename().string();
    auto it = models.find(file_name);
    if (it != models.end())
        return;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    thread_pool->Submit(
        [this](std::string_view path)
        {
            auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

            Assimp::Importer importer{};
            const aiScene* scene = importer.ReadFile(fs->GetFullPathToFile(path.data()), ASSIMP_LOAD_FLAGS);
            if (scene)
            {
                std::string file_name = std::filesystem::path(path.data()).filename().replace_extension().string();
                std::lock_guard lock(models_async_operations);
                auto model = models.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Model>(scene)).first->second;
                ++models_count;
            }
        },
        path);
}

std::shared_ptr<Fuego::Graphics::Image2D> Fuego::AssetsManager::load_image2d(std::string_view path)
{
    if (path.empty())
        return std::shared_ptr<Fuego::Graphics::Image2D>();

    std::string file_name = std::filesystem::path(path.data()).filename().string();
    auto image = images2d.find(file_name);
    if (image != images2d.end())
        return image->second;

    auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();

    uint16_t channels = ImageChannels(std::filesystem::path(file_name).extension().string());
    stbi_set_flip_vertically_on_load(1);
    int w, h, bpp = 0;
    unsigned char* data = stbi_load(fs->GetFullPathToFile(path.data()).c_str(), &w, &h, &bpp, channels);
    if (!data)
    {
        FU_CORE_ERROR("Can't load an image: {0} {1}", path, stbi_failure_reason());
        return nullptr;
    }
    auto img = images2d.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Image2D>(file_name, data, w, h, bpp, channels)).first->second;
    ++images2d_count;
    FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", images2d.size(), img->Name(), img->Width(), img->Height());
    return img;
}

std::shared_ptr<Fuego::Graphics::Image2D> Fuego::AssetsManager::load_image2d_async(std::string_view path)
{
    if (path.empty())
        return std::shared_ptr<Fuego::Graphics::Image2D>{nullptr};

    std::string file_name = std::filesystem::path(path.data()).filename().string();
    auto it = images2d.find(file_name);
    if (it != images2d.end())
        return it->second;

    images2d_async_operations.lock();
    // shared_ptr
    auto placed_img = images2d.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Image2D>(file_name)).first->second;
    FU_CORE_INFO("async placed_img: {0}", (void*)placed_img.get());

    images2d_async_operations.unlock();
    ++images2d_count;
    // TODO we create an empty image return it but we fill it in different thread further
    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    thread_pool->Submit(
        [this](std::shared_ptr<Fuego::Graphics::Image2D> img)
        {
            auto fs = ServiceLocator::instance().GetService<Fuego::FS::FileSystem>();
            const char* img_name = img->Name().data();
            uint16_t channels = ImageChannels(std::filesystem::path(img_name).extension().string());
            stbi_set_flip_vertically_on_load(1);
            int w, h, bpp = 0;
            unsigned char* data = stbi_load(fs->GetFullPathToFile(img_name).c_str(), &w, &h, &bpp, channels);
            // TODO to think about what to do if  data is corrupted
            if (!data)
                return;

            images2d_async_operations.lock();

            Fuego::Graphics::Image2D::Image2DPostCreateion settings{w, h, bpp, channels, data};
            FU_CORE_INFO("thread async placed_img: {0}", (void*)img.get());
            img->PostCreate(settings);
            // auto img = images2d.emplace(std::move(name), std::make_shared<Fuego::Graphics::Image2D>(name.data(), data, w, h, bpp, channels)).first->second;
            images2d_async_operations.unlock();
            //++images2d_count;
            FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", images2d.size(), img->Name(), img->Width(), img->Height());
        },
        placed_img);
    return placed_img;
}


std::shared_ptr<Fuego::Graphics::Image2D> Fuego::AssetsManager::LoadImage2DFromMemory(std::string_view name, unsigned char* data, uint32_t size_b,
                                                                                      uint16_t channels)
{
    if (!data)
        return std::shared_ptr<Fuego::Graphics::Image2D>();

    std::string file_name = std::filesystem::path(name.data()).filename().string();
    auto image = images2d.find(file_name);
    if (image != images2d.end())
        return image->second;

    int w, h, bpp = 0;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* img_data = stbi_load_from_memory(data, size_b, &w, &h, &bpp, channels);

    if (img_data)
    {
        images2d_async_operations.lock();
        auto img = images2d.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Image2D>(file_name, img_data, w, h, bpp, channels)).first->second;
        images2d_async_operations.unlock();
        ++images2d_count;
        FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", images2d.size(), img->Name(), img->Width(), img->Height());
        return img;
    }
}

std::shared_ptr<Fuego::Graphics::Image2D> Fuego::AssetsManager::LoadImage2DFromRawData(std::string_view name, unsigned char* data, uint32_t channels,
                                                                                       uint16_t bpp, uint32_t width, uint32_t height)
{
    if (!data || name.empty())
        return std::shared_ptr<Fuego::Graphics::Image2D>();

    std::string file_name = std::filesystem::path(name.data()).filename().string();
    auto image = images2d.find(file_name);
    if (image != images2d.end())
        return image->second;

    images2d_async_operations.lock();
    auto img = images2d.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Image2D>(file_name, data, width, height, bpp, channels)).first->second;
    images2d_async_operations.unlock();
    ++images2d_count;
    FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", images2d.size(), img->Name(), img->Width(), img->Height());
    return img;
}

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
