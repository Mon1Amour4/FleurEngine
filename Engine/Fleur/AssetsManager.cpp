#include "AssetsManager.h"

#if !defined(CGLTF_IMPLEMENTATION)
#define CGLTF_IMPLEMENTATION
#include "External/cgltf/cgltf.h"
#endif

#if !defined(STB_IMAGE_WRITE_IMPLEMENTATION)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "External/stb_image/stb_image_write.h"
#endif

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

    std::string fileName = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(m_Models, fileName, handle);
    if (loaded)
        return handle;

    auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    handle = std::make_shared<Fleur::ResourceHandle<Model>>(std::make_shared<Model>(fileName));

    auto res = fileSystem->GetFullPathToFile(path);
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
    handle = std::make_shared<Fleur::ResourceHandle<Model>>(std::make_shared<Model>(fileName, data));
    handle->SetSuccess();
    m_Models.emplace(std::move(fileName), handle->Resource());
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

    std::string fileName = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(m_Models, fileName, handle);
    if (loaded)
        return handle;
    {
        const auto it = m_ModelsToLoadAsync.find(fileName);
        if (it != m_ModelsToLoadAsync.end() && it->second->Status() != CORRUPTED)
            return it->second;
    }

    handle = m_ModelsToLoadAsync.emplace(fileName, std::make_shared<Fleur::ResourceHandle<Model>>(nullptr)).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](std::string_view path, std::string_view fileName, std::shared_ptr<Fleur::ResourceHandle<Model>> handle)
        {
            handle->SetStatus(ELoadingSts::LOADING);

            auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            auto res = fileSystem->GetFullPathToFile(path);
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

            handle->SetResource(std::make_shared<Model>(fileName, data));
            handle->SetSuccess();
            m_Models.emplace(fileName, handle->Resource());
            FL_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", m_Models.size(), fileName);
            ++m_ModelsCount;

            auto it = m_ModelsToLoadAsync.find(fileName.data());
            if (it != m_ModelsToLoadAsync.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_ModelsToLoadAsync.unsafe_erase(it);
            }
            cgltf_free(data);
        },
        path, fileName, handle);

    return handle;
}

// Image:
CONST_SHARED_RES(Image2D) Fleur::AssetsManager::load_image2d(std::string_view path, bool flipVertical)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (path.empty())
        return handle;

    std::string fileName = std::filesystem::path(path).stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;

    auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fs->GetFullPathToFile(path);
    if (!res)
        return std::make_shared<Fleur::ResourceHandle<Image2D>>(nullptr, CORRUPTED, WRONG_PATH);

    int w, h, channels = 0;
    unsigned char* imgData = stbi_load(res.value().c_str(), &w, &h, &channels, 0);
    if (!imgData)
    {
        FL_CORE_ERROR("Can't load an image: {0} {1}", path, stbi_failure_reason());
        return std::make_shared<Fleur::ResourceHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);
    }

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));

    auto img = m_Images2D.emplace(fileName, std::make_shared<Image2D>(fileName, ext, imgData, w, h, channels, 1)).first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fleur::ResourceHandle<Image2D>>(img, SUCCESS);
    stbi_image_free(imgData);
    return handle;
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::load_image2d_async(std::string_view path, bool flipVertical)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (path.empty())
        return handle;

    std::string fileName = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;
    {
        auto it = m_Images2DToLoadAsync.find(fileName);
        if (it != m_Images2DToLoadAsync.end() && it->second->Status() != CORRUPTED)
            return it->second;
    }

    handle = m_Images2DToLoadAsync.emplace(fileName, std::make_shared<Fleur::ResourceHandle<Image2D>>(std::make_shared<Image2D>(fileName, ext))).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](std::shared_ptr<Fleur::ResourceHandle<Image2D>> handle, bool flipVertical)
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

            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
            unsigned char* imgData = stbi_load(res.value().c_str(), &w, &h, &channels, 0);

            if (!imgData)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }
            Fleur::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(channels), 1, imgData};
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

            stbi_image_free(imgData);
        },
        handle, flipVertical);
    return handle;
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromMemory(std::string_view name, bool flipVertical, unsigned char* data, uint32_t sizeBytes)
{
    SHARED_RES(Image2D) handle{nullptr};
    if (!data)
        return handle;

    std::string fileName = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;

    int w, h, channels = 0;
    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
    unsigned char* imgData = stbi_load_from_memory(data, sizeBytes, &w, &h, &channels, 0);

    if (!imgData)
        return std::make_shared<Fleur::ResourceHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);

    auto img = m_Images2D.emplace(fileName, std::make_shared<Image2D>(fileName, ext, imgData, w, h, channels, 1)).first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    return std::make_shared<Fleur::ResourceHandle<Image2D>>(img, SUCCESS);

    stbi_image_free(imgData);
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromMemoryAsync(std::string_view name, bool flipVertical, unsigned char* data, uint32_t sizeBytes)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Image2D>> handle{nullptr};
    if (!data)
        return handle;

    std::string fileName = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;

    auto it = m_Images2DToLoadAsync.find(fileName);
    if (it != m_Images2DToLoadAsync.end() && it->second->Status() != CORRUPTED)
        return it->second;

    handle = m_Images2DToLoadAsync.emplace(fileName, std::make_shared<Fleur::ResourceHandle<Image2D>>(std::make_shared<Image2D>(fileName, ext))).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();
    threadPool->Submit(
        [this](std::shared_ptr<Fleur::ResourceHandle<Image2D>> handle, bool flipVertical, unsigned char* data, uint32_t sizeBytes)
        {
            if (!data)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }

            int w, h, channels = 0;
            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
            unsigned char* imgData = stbi_load_from_memory(data, sizeBytes, &w, &h, &channels, 0);

            if (!imgData)
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

            Fleur::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(channels), 1, imgData};
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

            stbi_image_free(imgData);
        },
        handle, flipVertical, data, sizeBytes);
    return handle;
}

CONST_SHARED_RES(Image2D)
Fleur::AssetsManager::LoadImage2DFromRawData(std::string_view name, unsigned char* data, uint32_t channels, uint32_t width, uint32_t height)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::Image2D>> handle{nullptr};
    if (!data || name.empty())
        return handle;

    std::string fileName = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;

    auto img = m_Images2D.emplace(fileName, std::make_shared<Image2D>(fileName, ext, data, width, height, channels, 1)).first->second;
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

    uint32_t colorData = color.Data();

    unsigned char* data = new unsigned char[size];
    for (size_t i = 0; i < width * height; ++i)
    {
        std::memcpy(data + i * channels, &colorData, channels);
    }
    auto img = m_Images2D.emplace(name, std::make_shared<Image2D>(name, "-", data, width, height, channels, 1)).first->second;

    return std::make_shared<Fleur::ResourceHandle<Image2D>>(img, ELoadingSts::SUCCESS);
}

// CubemapImage:
CONST_SHARED_RES(CubemapImage) Fleur::AssetsManager::load_cubemap_image(std::string_view path, bool flipVertical)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    std::string fileName = std::filesystem::path(path).stem().string();
    bool loaded = is_already_loaded(m_CubemapImages, fileName, handle);
    if (loaded)
        return handle;

    SHARED_RES(Image2D) image2d = load_image2d(path, flipVertical);
    Fleur::Graphics::Image2D crossLayout = image2d->Resource()->FromEquirectangularToCross();
    auto cubemapImg = m_CubemapImages.emplace(fileName, std::make_shared<CubemapImage>(crossLayout.FromCrossToCubemap())).first->second;
    ++m_CubemapImagesCount;
    FL_CORE_INFO("CubemapImage was emplaced: {0}", cubemapImg->Name());
}

CONST_SHARED_RES(CubemapImage) Fleur::AssetsManager::load_cubemap_image_async(std::string_view path, bool flipVertical)
{
    std::shared_ptr<Fleur::ResourceHandle<Fleur::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    SHARED_RES(Image2D) imageHandle = load_image2d_async(path, flipVertical);

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    handle = m_CubemapImagesToLoadAsync.emplace(path, std::make_shared<Fleur::ResourceHandle<CubemapImage>>()).first->second;

    threadPool->Submit(
        [this](std::shared_ptr<Fleur::ResourceHandle<Image2D>> imgHandle, std::shared_ptr<Fleur::ResourceHandle<CubemapImage>> cubemapHandle, bool flipVertical)
        {
            auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            while (imgHandle->Status() != ELoadingSts::SUCCESS && imgHandle->Status() != ELoadingSts::CORRUPTED)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (imgHandle->Status() == ELoadingSts::CORRUPTED)
            {
                cubemapHandle->SetCorrupted(imgHandle->FailureReason().value());
                return;
            }


            // Determine is it cross layout or equirectangular image:
            uint32_t imageRation = imgHandle->Resource()->Width() / imgHandle->Resource()->Height();
            if (imageRation == 2)
            {
                // equirectangular image
                auto crossLayout = m_Images2D
                                       .emplace(imgHandle->Resource()->Name().data() + std::string("_cross_layout"),
                                                std::make_shared<Image2D>(imgHandle->Resource()->FromEquirectangularToCross()))
                                       .first->second;


                stbi_write_jpg("D:\\Engine\\GameEngine\\Sandbox\\Resources\\Images\\MyTestCross.jpg", crossLayout->Width(), crossLayout->Height(),
                               crossLayout->Channels(), crossLayout->Data(), 100 /* 1-100 */);

                FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", crossLayout->Name());
                ++m_Images2DCount;
                Fleur::Graphics::CubemapImage cubemap = crossLayout->FromCrossToCubemap();
                cubemapHandle->SetResource(std::make_shared<CubemapImage>(std::move(cubemap)));
                cubemapHandle->SetSuccess();
            }
            else
            {
                Fleur::Graphics::CubemapImage cubemap = imgHandle->Resource()->FromCrossToCubemap();
                cubemapHandle->SetResource(std::make_shared<CubemapImage>(std::move(cubemap)));
                cubemapHandle->SetSuccess();
            }

            auto image = m_CubemapImages.emplace(cubemapHandle->Resource()->Name(), cubemapHandle->Resource());

            FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", cubemapHandle->Resource()->Name());
            ++m_CubemapImagesCount;

            auto it = m_CubemapImagesToLoadAsync.find(cubemapHandle->Resource()->Name().data());
            if (it != m_CubemapImagesToLoadAsync.end())
            {
                std::mutex mtx;
                std::lock_guard<std::mutex> lock(mtx);
                m_CubemapImagesToLoadAsync.unsafe_erase(it);
            }
        },
        imageHandle, handle, flipVertical);
    return handle;
}

// Other:
uint16_t Fleur::AssetsManager::ImageChannels(std::string_view image2DExt)
{
    if (image2DExt.empty() || image2DExt.size() > 3)
        return static_cast<uint16_t>(3);

    if (image2DExt.compare("jpg"))
    {
        return static_cast<uint16_t>(3);
    }
    else if (image2DExt.compare("png"))
    {
        return static_cast<uint16_t>(4);
    }
    else
    {
        return static_cast<uint16_t>(3);
    }
}
