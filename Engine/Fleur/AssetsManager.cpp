#include "AssetsManager.h"

#include "FleurAllocator.hpp"
#include "Renderer/ModelFabric.h"

#if !defined(CGLTF_IMPLEMENTATION)
#define CGLTF_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4996)
#include <External/cgltf/cgltf.h>
#endif

#if !defined(STB_IMAGE_WRITE_IMPLEMENTATION)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <External/stb_image/stb_image_write.h>
#endif

#include <External/stb_image/stb_image.h>

#pragma warning(pop)

#include "FileSystem/FileSystem.h"
#include "Services/ServiceLocator.h"

using Model = Fleur::Graphics::Model;
using Texture = Fleur::Graphics::Texture;
using Image2D = Fleur::Graphics::Image2D;
using CubemapImage = Fleur::Graphics::CubemapImage;


Fleur::AssetsManager::AssetsManager()
    : m_ModelsCount(0)
    , m_Images2DCount(0)
    , needToUploadResources(false)
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
    // bool loaded = is_already_loaded(m_Models, fileName, handle);
    // if (loaded)
    //     return handle;

    auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    handle = std::make_shared<Fleur::AsyncOperationHandle<Model>>(std::make_shared<Model>(fileName));

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
    if (result != cgltf_result_success)
    {
        handle->SetCorrupted(NO_DATA);
        return handle;
    }

    Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(fileName, data);
    Fleur::Graphics::Model* model = fabric.ProcessModel();
    handle->SetSuccess();
    handle->SetResource(std::make_shared<Model>(std::move(*model)));
    // m_Models.emplace(std::move(fileName), handle->Resource());
    ++m_ModelsCount;

    // FL_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", m_Models.size(), handle->Resource()->GetName());

    cgltf_free(data);

    needToUploadResources = true;

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

    /*bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;*/

    auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fs->GetFullPathToFile(path);
    if (!res)
        return std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(nullptr, CORRUPTED, WRONG_PATH);

    int w, h, channels = 0;
    uint32_t desiredChannels = 4;
    unsigned char* imgData = stbi_load(res.value().c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!imgData)
    {
        FL_CORE_ERROR("Can't load an image: {0} {1}", path, stbi_failure_reason());
        return std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);
    }

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));

    auto img =
        m_Images2D.emplace(fileName, std::make_shared<Image2D>(fileName, ext, imgData, w, h, static_cast<uint16_t>(desiredChannels), static_cast<uint16_t>(1)))
            .first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(img, SUCCESS);
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

    handle =
        m_Images2DToLoadAsync.emplace(fileName, std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(std::make_shared<Image2D>(fileName, ext))).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](std::shared_ptr<Fleur::AsyncOperationHandle<Image2D>> handle, bool flipVertical)
        {
            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();
            auto img = handle->Resource();

            int w, h, channels = 0;
            uint32_t desiredChannels = 4;
            std::filesystem::path full_path = img->Name();
            full_path.replace_extension(img->Ext());

            auto res = fs->GetFullPathToFile(full_path.string());
            if (!res)
            {
                handle->SetCorrupted(WRONG_PATH);
                return;
            }

            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
            unsigned char* imgData = stbi_load(res.value().c_str(), &w, &h, &channels, STBI_rgb_alpha);

            if (!imgData)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }
            Fleur::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(desiredChannels), 1, imgData};
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

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromMemory(std::string_view name, bool flipVertical, unsigned char* data, size_t sizeBytes)
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
    uint32_t desiredChannels = 4;
    stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
    unsigned char* imgData = stbi_load_from_memory(data, static_cast<int>(sizeBytes), &w, &h, &channels, STBI_rgb_alpha);

    if (!imgData)
        return std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(nullptr, CORRUPTED, NO_DATA);

    auto img =
        m_Images2D.emplace(fileName, std::make_shared<Image2D>(fileName, ext, imgData, w, h, static_cast<uint16_t>(desiredChannels), static_cast<uint16_t>(1)))
            .first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    return std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(img, SUCCESS);

    stbi_image_free(imgData);
}

CONST_SHARED_RES(Image2D) Fleur::AssetsManager::LoadImage2DFromMemoryAsync(std::string_view name, bool flipVertical, unsigned char* data, size_t sizeBytes)
{
    std::shared_ptr<Fleur::AsyncOperationHandle<Fleur::Graphics::Image2D>> handle{nullptr};
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

    handle =
        m_Images2DToLoadAsync.emplace(fileName, std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(std::make_shared<Image2D>(fileName, ext))).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();
    threadPool->Submit(
        [this](std::shared_ptr<Fleur::AsyncOperationHandle<Image2D>> handle, bool flipVertical, unsigned char* data, size_t sizeBytes)
        {
            if (!data)
            {
                handle->SetCorrupted(NO_DATA);
                return;
            }

            int w, h, channels = 0;
            uint32_t desiredChannels = 4;
            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
            unsigned char* imgData = stbi_load_from_memory(data, static_cast<int>(sizeBytes), &w, &h, &channels, STBI_rgb_alpha);

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

            Fleur::Graphics::ImagePostCreation settings{static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint16_t>(desiredChannels), 1, imgData};
            handle->Resource()->PostCreate(settings);
            auto image = m_Images2D.emplace(handle->Resource()->Name(), handle->Resource()).first->second;
            FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", image->Name());
            ++m_Images2DCount;
            handle->SetSuccess();

            {
                auto it2 = m_Images2DToLoadAsync.find(handle->Resource()->Name().data());
                if (it2 != m_Images2DToLoadAsync.end())
                {
                    std::mutex mtx;
                    std::lock_guard<std::mutex> lock(mtx);
                    m_Images2DToLoadAsync.unsafe_erase(it2);
                }
            }

            stbi_image_free(imgData);
        },
        handle, flipVertical, data, sizeBytes);
    return handle;
}

CONST_SHARED_RES(Image2D)
Fleur::AssetsManager::LoadImage2DFromRawData(std::string_view name, unsigned char* data, uint16_t channels, uint32_t width, uint32_t height)
{
    std::shared_ptr<Fleur::AsyncOperationHandle<Fleur::Graphics::Image2D>> handle{nullptr};
    if (!data || name.empty())
        return handle;

    std::string fileName = std::filesystem::path(name.data()).stem().string();
    std::string ext = std::filesystem::path(name.data()).extension().string();

    bool loaded = is_already_loaded(m_Images2D, fileName, handle);
    if (loaded)
        return handle;

    auto img = m_Images2D.emplace(fileName, std::make_shared<Image2D>(fileName, ext, data, width, height, channels, static_cast<uint16_t>(1))).first->second;
    FL_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, width: {2}, height: {3}", ++m_Images2DCount, img->Name(), img->Width(), img->Height());
    handle = std::make_shared<Fleur::AsyncOperationHandle<Image2D>>(img, SUCCESS);
    return handle;
}


// CubemapImage:
CONST_SHARED_RES(CubemapImage) Fleur::AssetsManager::load_cubemap_image(std::string_view path, bool flipVertical)
{
    std::shared_ptr<Fleur::AsyncOperationHandle<Fleur::Graphics::CubemapImage>> handle{nullptr};
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
    std::shared_ptr<Fleur::AsyncOperationHandle<Fleur::Graphics::CubemapImage>> handle{nullptr};
    if (path.empty())
        return handle;

    SHARED_RES(Image2D) imageHandle = load_image2d_async(path, flipVertical);

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    handle = m_CubemapImagesToLoadAsync.emplace(path, std::make_shared<Fleur::AsyncOperationHandle<CubemapImage>>()).first->second;

    threadPool->Submit(
        [this](std::shared_ptr<Fleur::AsyncOperationHandle<Image2D>> imgHandle, std::shared_ptr<Fleur::AsyncOperationHandle<CubemapImage>> cubemapHandle,
               bool flipVertical)
        {
            UNUSED(flipVertical);

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


//======================================================================
// Service
void Fleur::AssetsManager::OnInit()
{
    load_all_shaders();
}
void Fleur::AssetsManager::OnShutdown()
{
}
void Fleur::AssetsManager::OnUpdate(float dtTime)
{
    if (needToUploadResources)
    {
        auto renderer = ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>();

        Fleur::Graphics::SFLImageViewInfo info{};
        info.pData = m_ImagesToUpload.data();
        info.count = m_ImagesToUpload.size();
        renderer->SubmitImageViews(&info);

        m_ImagesToUpload.clear();
    }
}


//======================================================================
// Shader
void Fleur::AssetsManager::load_all_shaders()
{
    auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();
    auto path = fileSystem->GetFullPathToFolder("Shaders");

    std::vector<std::string> paths = fileSystem->GetAllFilesInFolder(path->c_str(), ".spv");

    for (const auto& path : paths)
    {
        auto vec = fileSystem->ReadFileBinary(path);
        m_ShaderMap.emplace(fileSystem->GetFileNameWithoutExtFromPath(path),
                            std::make_shared<Fleur::Graphics::Shader>(Fleur::Graphics::Shader(vec.data(), vec.size())));
    }
}

//======================================================================
// Image2D
const Fleur::AsyncOperation<Fleur::Graphics::Image2D>* Fleur::AssetsManager::load_async_image2D(std::string_view path)
{
    using value_type = Fleur::Graphics::Image2D;
    AssetID ID = m_AssetIDCounter++;

    std::string fileName = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    m_StringToIDMap.emplace(fileName, ID);
    auto pair = m_Image2DMap.emplace(ID, value_type(fileName, ext)).first;
    Fleur::Asset<value_type> asset{pair->first, &pair->second};
    Fleur::AsyncOperation<value_type> op{std::move(asset), ELoadingSts::TO_BE_LOADED};

    Fleur::AsyncOperation<value_type>* asyncOperation = &m_Image2DAsyncOperationsMap.emplace(pair->first, std::move(op)).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](Fleur::AsyncOperation<value_type>* handle, bool flipVertical)
        {
            value_type* image2D = handle->asset.obj;

            handle->status = ELoadingSts::LOADING;

            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            std::filesystem::path full_path = image2D->Name();
            full_path.replace_extension(image2D->Ext());

            auto res = fs->GetFullPathToFile(full_path.string());
            if (!res)
            {
                handle->status = ELoadingSts::CORRUPTED;
                return;
            }

            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));

            int width = 0;
            int height = 0;
            int channels = 0;
            uint32_t desiredChannels = 4;

            unsigned char* imgData = stbi_load(res.value().c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!imgData)
            {
                handle->status = ELoadingSts::CORRUPTED;
                return;
            }
            handle->status = ELoadingSts::SUCCESS;
            Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
            image2D->PostCreate(settings);

            FL_CORE_INFO("[AssetsManager] Image ({1}, {2}, {3}, {4}) was added", handle->asset.ID, image2D->Name(), image2D->Width(), image2D->Height());

            Fleur::Graphics::SFLImageView imageView = image2D->GetView();
            imageView.ID = handle->asset.ID;

            AddImageToUpload(imageView);

            stbi_image_free(imgData);
        },
        asyncOperation, false);

    return asyncOperation;
}
Fleur::AssetID Fleur::AssetsManager::LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size)
{
    assert(pData && size > 0);

    using value_type = Fleur::Graphics::Image2D;

    AssetID ID = m_AssetIDCounter++;

    m_StringToIDMap.emplace(name, ID);
    value_type* image2D = &m_Image2DMap.emplace(ID, value_type(name, "")).first->second;

    int width = 0;
    int height = 0;
    int channels = 0;
    int desiredChannels = 4;

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(false));
    unsigned char* imgData = stbi_load_from_memory(pData, static_cast<int>(size), &width, &height, &channels, STBI_rgb_alpha);

    Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, desiredChannels, 1, imgData};
    image2D->PostCreate(info);

    FL_CORE_INFO("[AssetsManager] Image ({1}, {2}, {3}, {4}) was added", ID, image2D->Name(), image2D->Width(), image2D->Height());

    Fleur::Graphics::SFLImageView imageView = image2D->GetView();
    imageView.ID = ID;
    AddImageToUpload(imageView);

    return ID;
}
Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetsManager::FromColor(std::string_view name, Fleur::Graphics::Color color)
{
    using value_type = Fleur::Graphics::Image2D;
    AssetID ID = m_AssetIDCounter++;

    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t channels = Fleur::Graphics::Color::Channels(color);

    size_t size = width * height * channels;

    uint32_t colorData = color.Data();

    Fleur::Memory::FleurAllocator<unsigned char> alloc;
    unsigned char* data = alloc.allocate(size);
    for (size_t i = 0; i < width * height * channels; ++i)
    {
        std::memcpy(data + i, &colorData, channels);
    }

    m_StringToIDMap.emplace(name, ID);
    value_type* image2D = &m_Image2DMap.emplace(ID, Fleur::Graphics::Image2D(name, "", data, width, height, channels, 1)).first->second;

    alloc.deallocate(data, size);

    Fleur::Graphics::SFLImageView imageView = image2D->GetView();
    imageView.ID = ID;
    AddImageToUpload(imageView);

    FL_CORE_INFO("[AssetsManager] Image ({1}, {2}, {3}, {4}) was added", ID, image2D->Name(), image2D->Width(), image2D->Height());

    return Fleur::Asset<value_type>{ID, image2D};
}
void Fleur::AssetsManager::AddImageToUpload(Fleur::Graphics::SFLImageView imageView)
{
    m_ImagesToUpload.push_back(imageView);
    needToUploadResources = true;
}


//======================================================================
// Model
const Fleur::AsyncOperation<Fleur::Graphics::Model>* Fleur::AssetsManager::load_async_model(std::string_view path)
{
    using value_type = Fleur::Graphics::Model;

    std::string fileName = std::filesystem::path(path).filename().stem().string();

    AssetID ID = m_AssetIDCounter++;
    m_StringToIDMap.emplace(fileName, ID);

    auto pair = m_ModelMap.emplace(ID, value_type(fileName)).first;
    Fleur::Asset<value_type> asset{ID, nullptr};
    Fleur::AsyncOperation<value_type> op{std::move(asset), ELoadingSts::TO_BE_LOADED};

    Fleur::AsyncOperation<value_type>* asyncOperation = &m_ModelAsyncOperationsMap.emplace(ID, std::move(op)).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](Fleur::AsyncOperation<value_type>* handle, std::string_view path)
        {
            handle->status = ELoadingSts::LOADING;

            auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            auto res = fileSystem->GetFullPathToFile(path);
            if (!res)
            {
                handle->status = ELoadingSts::CORRUPTED;
                handle->asset.obj = nullptr;
                return handle;
            }

            cgltf_options options = {};
            cgltf_data* data = NULL;
            cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
            if (result != cgltf_result_success)
            {
                handle->status = ELoadingSts::CORRUPTED;
                handle->asset.obj = nullptr;
                return handle;
            }
            result = cgltf_load_buffers(&options, data, res->c_str());
            if (result != cgltf_result_success)
            {
                handle->status = ELoadingSts::CORRUPTED;
                handle->asset.obj = nullptr;
                return handle;
            }

            Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(handle->asset.obj->GetName(), data);
            Fleur::Graphics::Model::SFLPostCreateInfo info = fabric.ProcessData();

            handle->asset.obj->PostCreate(info);

            FL_CORE_INFO("[AssetsManager] Image was added: name: {0}, ", handle->asset.obj->GetName());

            cgltf_free(data);
        },
        asyncOperation, path);

    return asyncOperation;
}
