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

using Model = Fleur::Graphics::Model;
using Texture = Fleur::Graphics::Texture;
using Image2D = Fleur::Graphics::Image2D;
using CubemapImage = Fleur::Graphics::CubemapImage;

Fleur::AssetsManager::AssetsManager()
    : m_ModelsCount(0)
    , m_Images2DCount(0)
{
    m_Models.reserve(10);
    m_Images2D.reserve(10);
}

Fleur::AssetsManager::~AssetsManager()
{
    m_Models.clear();
    m_Images2D.clear();
}

// Models:
CONST_SHARED_RES(Model) Fleur::AssetsManager::load_model(std::string_view path)
{
    SHARED_RES(Model) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(m_Models, file_name, handle);
    if (loaded)
        return handle;

    auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    handle = std::make_shared<Fleur::ResourceHandle<Model>>(std::make_shared<Model>(file_name));

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
    handle = std::make_shared<Fleur::ResourceHandle<Model>>(std::make_shared<Model>(file_name, data));
    handle->SetSuccess();
    m_Models.emplace(std::move(file_name), handle->Resource());
    ++m_ModelsCount;

    FL_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", m_Models.size(), handle->Resource()->GetName());

    cgltf_free(data);
    return handle;
}
CONST_SHARED_RES(Model) Fleur::AssetsManager::load_model_async(std::string_view path)
{
    SHARED_RES(Model) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(m_Models, file_name, handle);
    if (loaded)
        return handle;
    {
        const auto it = m_ModelsToLoadAsync.find(file_name);
        if (it != m_ModelsToLoadAsync.end() && it->second->Status() != CORRUPTED)
            return it->second;
    }

    handle = m_ModelsToLoadAsync.emplace(file_name, std::make_shared<Fleur::ResourceHandle<Model>>(nullptr)).first->second;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    thread_pool->Submit(
        [this](std::string_view path, std::string_view file_name, std::shared_ptr<Fleur::ResourceHandle<Model>> handle)
        {
            handle->SetStatus(ELoadingSts::LOADING);

            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

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
            m_Models.emplace(file_name, handle->Resource());
            FL_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", m_Models.size(), file_name);
            ++m_ModelsCount;

            auto it = m_ModelsToLoadAsync.find(file_name.data());
            if (it != m_ModelsToLoadAsync.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_ModelsToLoadAsync.unsafe_erase(it);
            }
            cgltf_free(data);
        },
        path, file_name, handle);

    return handle;
}

// Image:
CONST_SHARED_RES(Image2D) Fleur::AssetsManager::load_image2d(std::string_view path, bool flip_vertical)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    bool loaded = is_already_loaded(m_Images2D, file_name, handle);
    if (loaded)
        return handle;

    auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fs->GetFullPathToFile(path);
    if (!res)
        return std::make_shared<Fleur::ResourceHandle<Image2D>>(nullptr, CORRUPTED, WRONG_PATH);

    int w, h, channels = 0;
    unsigned char* img_data = stbi_load(res.value().c_str(), &w, &h, &channels, 0);
    if (!img_data)
    {
        FL_CORE_ERROR("Can't load an image: {0} {1}", path, stbi_failure_reason());
        return std::make_shared<Fleur::ResourceHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);
    }

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flip_vertical));

    auto img = m_Images2D.emplace(file_name, std::make_shared<Image2D>(file_name, ext, img_data, w, h, channels, 1)).first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fleur::ResourceHandle<Image2D>>(img, SUCCESS);
    stbi_image_free(img_data);
    return handle;
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::load_image2d_async(std::string_view path, bool flip_vertical)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    bool loaded = is_already_loaded(m_Images2D, file_name, handle);
    if (loaded)
        return handle;
    {
        auto it = m_Images2DToLoadAsync.find(file_name);
        if (it != m_Images2DToLoadAsync.end() && it->second->Status() != CORRUPTED)
            return it->second;
    }

    handle =
        m_Images2DToLoadAsync.emplace(file_name, std::make_shared<Fleur::ResourceHandle<Image2D>>(std::make_shared<Image2D>(file_name, ext))).first->second;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    thread_pool->Submit(
        [this](std::shared_ptr<Fleur::ResourceHandle<Image2D>> handle, bool flip_vertical)
        {
            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();
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
            Fleur::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(channels), 1, img_data};
            handle->Resource()->PostCreate(settings);

            auto image = m_Images2D.emplace(handle->Resource()->Name(), handle->Resource()).first->second;
            FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", image->Name());
            ++m_Images2DCount;
            handle->SetSuccess();

            auto it = m_Images2DToLoadAsync.find(handle->Resource()->Name().data());
            if (it != m_Images2DToLoadAsync.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_Images2DToLoadAsync.unsafe_erase(it);
            }

            stbi_image_free(img_data);
        },
        handle, flip_vertical);
    return handle;
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromMemory(std::string_view name, bool flip_vertical, unsigned char* data, uint32_t size_b)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (!data)
        return handle;

    std::string file_name = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, file_name, handle);
    if (loaded)
        return handle;

    int w, h, channels = 0;
    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flip_vertical));
    unsigned char* img_data = stbi_load_from_memory(data, size_b, &w, &h, &channels, 0);

    if (!img_data)
        return std::make_shared<Fleur::ResourceHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);

    auto img = m_Images2D.emplace(file_name, std::make_shared<Image2D>(file_name, ext, img_data, w, h, channels, 1)).first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    return std::make_shared<Fleur::ResourceHandle<Image2D>>(img, SUCCESS);

    stbi_image_free(img_data);
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromMemoryAsync(std::string_view name, bool flip_vertical, unsigned char* data, uint32_t size_b)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Image2D>> handle{nullptr};
    if (!data)
        return handle;

    std::string file_name = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, file_name, handle);
    if (loaded)
        return handle;

    auto it = m_Images2DToLoadAsync.find(file_name);
    if (it != m_Images2DToLoadAsync.end() && it->second->Status() != CORRUPTED)
        return it->second;

    handle =
        m_Images2DToLoadAsync.emplace(file_name, std::make_shared<Fleur::ResourceHandle<Image2D>>(std::make_shared<Image2D>(file_name, ext))).first->second;

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();
    thread_pool->Submit(
        [this](std::shared_ptr<Fleur::ResourceHandle<Image2D>> handle, bool flip_vertical, unsigned char* data, uint32_t size_b)
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

            auto it = m_Images2DToLoadAsync.find(handle->Resource()->Name().data());
            if (it != m_Images2DToLoadAsync.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_Images2DToLoadAsync.unsafe_erase(it);
            }

            Fleur::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(channels), 1, img_data};
            handle->Resource()->PostCreate(settings);
            auto image = m_Images2D.emplace(handle->Resource()->Name(), handle->Resource()).first->second;
            FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", image->Name());
            ++m_Images2DCount;
            handle->SetSuccess();

            {
                auto it = m_Images2DToLoadAsync.find(handle->Resource()->Name().data());
                if (it != m_Images2DToLoadAsync.end())
                {
                    std::mutex mtx;
                    std::lock_guard<std::mutex> lock(mtx);
                    m_Images2DToLoadAsync.unsafe_erase(it);
                }
            }

            stbi_image_free(img_data);
        },
        handle, flip_vertical, data, size_b);
    return handle;
}

CONST_SHARED_RES(Image2D)
Fleur::AssetsManager::LoadImage2DFromRawData(std::string_view name, unsigned char* data, uint32_t channels, uint32_t width, uint32_t height)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Image2D>> handle{nullptr};
    if (!data || name.empty())
        return handle;

    std::string file_name = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, file_name, handle);
    if (loaded)
        return handle;

    auto img = m_Images2D.emplace(file_name, std::make_shared<Image2D>(file_name, ext, data, width, height, channels, 1)).first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fleur::ResourceHandle<Image2D>>(img, SUCCESS);
    return handle;
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromColor(std::string_view name, Fleur::Graphics::Color color, uint32_t width, uint32_t height)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Image2D>> handle{nullptr};
    if (name.empty())
        return handle;

    uint32_t channels = Fleur::Graphics::Color::Channels(color);
    size_t size = width * height * channels;

    uint32_t color_data = color.Data();

    unsigned char* data = new unsigned char[size];
    for (size_t i = 0; i < width * height; ++i)
    {
        std::memcpy(data + i * channels, &color_data, channels);
    }
    auto img = m_Images2D.emplace(name, std::make_shared<Image2D>(name, "-", data, width, height, channels, 1)).first->second;

    return std::make_shared<Fleur::ResourceHandle<Image2D>>(img, ELoadingSts::SUCCESS);
}

// CubemapImage:
CONST_SHARED_RES(CubemapImage) Fleur::AssetsManager::load_cubemap_image(std::string_view path, bool flip_vertical)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    std::string file_name = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(m_CubemapImages, file_name, handle);
    if (loaded)
        return handle;

    SHARED_RES(Image2D) image2d = load_image2d(path, flip_vertical);
    Fleur::Graphics::Image2D cross_layout = image2d->Resource()->FromEquirectangularToCross();
    auto cubemap_img = m_CubemapImages.emplace(file_name, std::make_shared<CubemapImage>(cross_layout.FromCrossToCubemap())).first->second;
    ++m_CubemapImagesCount;
    FL_CORE_INFO("CubemapImage was emplaced: {0}", cubemap_img->Name());
}

CONST_SHARED_RES(CubemapImage) Fleur::AssetsManager::load_cubemap_image_async(std::string_view path, bool flip_vertical)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    SHARED_RES(Image2D) image_handle = load_image2d_async(path, flip_vertical);

    auto thread_pool = ServiceLocator::instance().GetService<ThreadPool>();

    handle = m_CubemapImagesToLoadAsync.emplace(path, std::make_shared<Fleur::ResourceHandle<CubemapImage>>()).first->second;

    thread_pool->Submit(
        [this](std::shared_ptr<Fleur::ResourceHandle<Image2D>> img_handle, std::shared_ptr<Fleur::ResourceHandle<CubemapImage>> cubemap_handle,
               bool flip_vertical)
        {
            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            while (img_handle->Status() != ELoadingSts::SUCCESS && img_handle->Status() != ELoadingSts::CORRUPTED)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (img_handle->Status() == ELoadingSts::CORRUPTED)
            {
                cubemap_handle->SetCorrupted(img_handle->FailureReason().value());
                return;
            }


            // Determine is it cross layout or equirectangular image:
            uint32_t image_ration = img_handle->Resource()->Width() / img_handle->Resource()->Height();
            if (image_ration == 2)
            {
                // equirectangular image
                auto cross_layout = m_Images2D
                                        .emplace(img_handle->Resource()->Name().data() + std::string("_cross_layout"),
                                                 std::make_shared<Image2D>(img_handle->Resource()->FromEquirectangularToCross()))
                                        .first->second;


                stbi_write_jpg("D:\\Engine\\GameEngine\\Sandbox\\Resources\\Images\\MyTestCross.jpg", cross_layout->Width(), cross_layout->Height(),
                               cross_layout->Channels(), cross_layout->Data(), 100 /* 1-100 */);

                FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", cross_layout->Name());
                ++m_Images2DCount;
                Fleur::Graphics::CubemapImage cubemap = cross_layout->FromCrossToCubemap();
                cubemap_handle->SetResource(std::make_shared<CubemapImage>(std::move(cubemap)));
                cubemap_handle->SetSuccess();
            }
            else
            {
                Fleur::Graphics::CubemapImage cubemap = img_handle->Resource()->FromCrossToCubemap();
                cubemap_handle->SetResource(std::make_shared<CubemapImage>(std::move(cubemap)));
                cubemap_handle->SetSuccess();
            }

            auto image = m_CubemapImages.emplace(cubemap_handle->Resource()->Name(), cubemap_handle->Resource());

            FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", cubemap_handle->Resource()->Name());
            ++m_CubemapImagesCount;

            auto it = m_CubemapImagesToLoadAsync.find(cubemap_handle->Resource()->Name().data());
            if (it != m_CubemapImagesToLoadAsync.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_CubemapImagesToLoadAsync.unsafe_erase(it);
            }
        },
        image_handle, handle, flip_vertical);
    return handle;
}

// Other:
uint16_t Fleur::AssetsManager::ImageChannels(std::string_view image2d_ext)
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
