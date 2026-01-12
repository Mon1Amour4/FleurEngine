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
{
}
Fleur::AssetsManager::~AssetsManager()
{
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
    /*if (needToUploadResources)
    {
        auto renderer = ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>();

        Fleur::Graphics::SFLImageViewInfo info{};
        info.pData = m_ImagesToUpload.data();
        info.count = m_ImagesToUpload.size();
        renderer->SubmitImageViews(&info);

        m_ImagesToUpload.clear();
    }*/
}


//======================================================================
// Image2D
std::shared_ptr<Fleur::AsyncOperation<Fleur::Graphics::Image2D>> Fleur::AssetCache<Fleur::Graphics::Image2D>::LoadAsync(std::string_view path, Fleur::AssetID id)
{
    using ImageType = Fleur::Graphics::Image2D;
    using ImageAsyncOp = Fleur::AsyncOperation<ImageType>;
    using ImageAsyncOpShared = std::shared_ptr<ImageAsyncOp>;

    std::string fileName = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    Asset<ImageType> image2dAsset = Add(fileName, id);

    ImageAsyncOpShared sharedOP = std::make_shared<ImageAsyncOp>(image2dAsset, ELoadingSts::TO_BE_LOADED);

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](ImageAsyncOpShared handle, bool flipVertical)
        {
            ImageType* imagePtr = handle->asset.obj;

            handle->status = ELoadingSts::LOADING;

            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            std::filesystem::path full_path = imagePtr->Name();
            full_path.replace_extension(imagePtr->Ext());

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

            Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
            imagePtr->PostCreate(settings);

            Fleur::Graphics::SFLImageView imageView = imagePtr->GetView();
            imageView.ID = handle->asset.ID;

            // AddImageToUpload(imageView);

            stbi_image_free(imgData);

            FL_CORE_INFO("[AssetsManager] Image ({1}, {2}, {3}, {4}) was added", handle->asset.ID, imagePtr->Name(), imagePtr->Width(), imagePtr->Height());
            handle->status = ELoadingSts::SUCCESS;
        },
        sharedOP, false);

    return sharedOP;
}
Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetCache<Fleur::Graphics::Image2D>::Load(std::string_view path, Fleur::AssetID id)
{
}
    Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetCache<Fleur::Graphics::Image2D>::Add(std::string_view name, AssetID id)
{
    stringMap.emplace(name, id);
    Fleur::Graphics::Image2D* image2D = &map.emplace(Fleur::Graphics::Image2D(name, "")).first->second;
    m_size++;

    return {id, image2D};
}
Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetsManager::FromColor(std::string_view name, Fleur::Graphics::Color color)
{
    using value_type = Fleur::Graphics::Image2D;
    AssetID id = m_GlobalId++;

    Asset<value_type> image2dAsset = m_Image2DCache.Add(name, id);

    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t channels = Fleur::Graphics::Color::Channels(color);

    size_t size = width * height * channels;

    uint32_t colorData = color.Data();

    Fleur::Memory::FleurAllocator<unsigned char> alloc;
    unsigned char* pData = alloc.allocate(size);
    for (size_t i = 0; i < width * height * channels; ++i)
    {
        std::memcpy(pData + i, &colorData, channels);
    }

    Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, channels, 1, pData};
    image2dAsset.obj->PostCreate(info);

    alloc.deallocate(pData, size);

    Fleur::Graphics::SFLImageView imageView = image2dAsset.obj->GetView();
    imageView.ID = id;
    //AddImageToUpload(imageView);

    FL_CORE_INFO("[AssetsManager] Image ({1}, {2}, {3}, {4}) was added", id, image2dAsset.obj->Name(), image2dAsset.obj->Width(), image2dAsset.obj->Height());

    return image2dAsset;
}
Fleur::Asset<Fleur::Graphics::Image2D> Fleur::AssetsManager::LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size)
{
    assert(pData && size > 0);

    using value_type = Fleur::Graphics::Image2D;
    AssetID id = m_GlobalId++;

    Asset<value_type> image2dAsset = m_Image2DCache.Add(name, id);

    int width = 0;
    int height = 0;
    int channels = 0;
    int desiredChannels = 4;

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(false));
    unsigned char* imgData = stbi_load_from_memory(pData, static_cast<int>(size), &width, &height, &channels, STBI_rgb_alpha);

    Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, desiredChannels, 1, imgData};

    image2dAsset.obj->PostCreate(info);

    Fleur::Graphics::SFLImageView imageView = image2dAsset.obj->GetView();
    imageView.ID = id;
    //AddImageToUpload(imageView);

    FL_CORE_INFO("[AssetsManager] Image ({1}, {2}, {3}, {4}) was added", id, image2dAsset.obj->Name(), image2dAsset.obj->Width(), image2dAsset.obj->Height());

    return image2dAsset;
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
        m_ShaderMap.emplace(fileSystem->GetFileNameWithoutExtFromPath(path), Fleur::Graphics::Shader(vec.data(), vec.size()));
    }
}



