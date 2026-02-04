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
    PollMessages();

    bool imagesWereUploaded = false;

    if (m_ImagesToUpload.ReadyToUpload())
    {
        auto renderer = ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>();

        std::lock_guard<std::mutex> lc(m_ImagesToUpload.mx);

        Fleur::Graphics::SFLImageViewInfo info{};
        info.pData = m_ImagesToUpload.images.data();
        info.count = m_ImagesToUpload.images.size();
        renderer->SubmitImageViews(&info);

        imagesWereUploaded = true;
        m_ImagesToUpload.framesSinceLastUpload = 0;

        m_ImagesToUpload.Clear();
    }


    if (!imagesWereUploaded)
        m_ImagesToUpload.framesSinceLastUpload++;
}

void Fleur::AssetsManager::load_all_shaders()
{
    auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();
    auto path = fileSystem->GetFullPathToFolder("Shaders");

    std::vector<std::string> paths = fileSystem->GetAllFilesInFolder(path->c_str(), ".spv");

    for (const auto& path : paths)
    {
        AssetID id = m_GlobalId++;
        auto vec = fileSystem->ReadFileBinary(path);
        auto name = fileSystem->GetFileNameWithoutExtFromPath(path);

        m_ShaderMapString.emplace(std::move(name), id);
        m_ShaderMap.emplace(id, ShaderType(vec.data(), vec.size()));
    }
}

void Fleur::AssetsManager::PollMessages()
{
    std::lock_guard<std::mutex> lc(m_MessageMutex);

    while (!m_MessageQueue.empty())
    {
        const auto& message = m_MessageQueue.front();
        switch (message.type)
        {
        case EVENT_TYPE_MODEL_LOADED:
        {
            switch (message.loadingStatus)
            {
            case CORRUPTED:
            case LOADING_STATUS_TO_TERMINATE:
            {
                m_ModelCache.RemoveBrokenAsyncAsset(message.ID);
                break;
            }
            case LOADED:
            {
                Fleur::Graphics::Model* model = (Fleur::Graphics::Model*)message.pResource;
                auto renderer = Fleur::ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>();

                glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 100.f));
                glm::mat4 R = glm::mat4(1.f);
                glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(0.1f, 0.1f, 0.1f));
                glm::mat4 M = T * R * S;
                renderer->DrawModel(Fleur::Graphics::STATIC_GEOMETRY, model, M);
                m_ModelCache.RemoveFromAsyncOperations(model->GetName());
                break;
            }
            }
            break;
        }
        case EVENT_TYPE_IMAGE2D_LOADED:
        {
            switch (message.loadingStatus)
            {
            case CORRUPTED:
            case LOADING_STATUS_TO_TERMINATE:
            {
                m_Image2DCache.RemoveBrokenAsyncAsset(message.ID);
                break;
            }
            case LOADED:
            {
                const Fleur::Graphics::Image2D* loadedImage = reinterpret_cast<const Fleur::Graphics::Image2D*>(message.pResource);

                Fleur::Graphics::SFLImageView view = loadedImage->GetView();
                view.ID = message.ID;
                m_ImagesToUpload.images.push_back(view);
                m_Image2DCache.RemoveFromAsyncOperations(loadedImage->GetName());
                break;
            }
            }
        }
        }

        m_MessageQueue.pop_front();
    }
}

Fleur::AssetID Fleur::AssetsManager::GetNextID()
{
    return m_GlobalId++;
}


// ---------- Sync ----------
ModelAsset Fleur::AssetsManager::LoadModel(std::string_view path)
{
    return m_ModelCache.LoadAndRegister(path, Fleur::ResourceImportSettings{}, GetNextID());
}
ImageAsset Fleur::AssetsManager::LoadImage(std::string_view path, ImageImportSettings imageSettings)
{
    Fleur::ResourceImportSettings settings{};
    settings.imageImportSettings = imageSettings;
    ImageAsset imageAsset = m_Image2DCache.LoadAndRegister(path, settings, GetNextID());

    Fleur::Graphics::SFLImageView imageView = imageAsset.obj->GetView();
    imageView.ID = imageAsset.ID;
    m_ImagesToUpload.Add(imageView);

    return imageAsset;
}
CubemapAsset Fleur::AssetsManager::LoadCubemap(std::string_view path, CubemapImportSettings cubemapSettings)
{
    Fleur::ResourceImportSettings settings{};
    settings.cubemapSettings = cubemapSettings;
    CubemapAsset cubemapAsset = m_CubemapCache.LoadAndRegister(path, settings, GetNextID());

    Fleur::Graphics::SFLImageView imageView = cubemapAsset.obj->GetView();
    imageView.ID = cubemapAsset.ID;
    m_ImagesToUpload.Add(imageView);

    return cubemapAsset;
}


// ---------- Async ----------
ModelAsyncOpShared Fleur::AssetsManager::LoadModelAsync(std::string_view path)
{
    AssetLoadCallback callback{};
    callback.manager = this;
    callback.callback = &Fleur::AssetsManager::OnAssetLoaded;

    return m_ModelCache.LoadAndRegisterAsync(path, Fleur::ResourceImportSettings{}, GetNextID(), callback);
}
ImageAsyncOpShared Fleur::AssetsManager::LoadImageAsync(std::string_view path)
{
    return ImageAsyncOpShared();
}
CubemapAsyncOpShared Fleur::AssetsManager::LoadCubemapAsync(std::string_view path, CubemapImportSettings settings)
{
    return CubemapAsyncOpShared();
}


// ---------- AssetCache ----------
ModelAsset Fleur::AssetCache<ModelType>::Load(std::string_view path, ModelRecord modelRecord, ResourceImportSettings importSettings)
{
    ModelAsset modelAsset = modelRecord.asset;
    ModelType* modelPtr = modelRecord.asset.obj;
    auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fileSystem->GetFullPathToFile(path);
    if (!res)
        return ModelAsset{0, nullptr};

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
    if (result != cgltf_result_success)
        return ModelAsset{0, nullptr};

    result = cgltf_load_buffers(&options, data, res->c_str());
    if (result != cgltf_result_success)
        return ModelAsset{0, nullptr};

    Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(res->c_str(), data);
    // fabric.load
    Fleur::Graphics::Model::SFLPostCreateInfo createInfo = fabric.ProcessData();
    // create images -> add to upload
    // fabric.release
    modelPtr->PostCreate(createInfo);

    cgltf_free(data);

    FL_CORE_INFO("[AssetsManager] Model (ID: {0}, {1}) has loaded", modelAsset.ID, modelPtr->GetName());

    return modelAsset;
}
ImageAsset Fleur::AssetCache<ImageType>::Load(std::string_view path, ImageRecord imageRecord, ResourceImportSettings importSettings)
{
    ImageAsset image2dAsset = imageRecord.asset;
    ImageType* imagePtr = imageRecord.asset.obj;
    if (importSettings.imageImportSettings.imageSource == IMAGE_SOURCE_DISK)
    {
        auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

        auto res = fs->GetFullPathToFile(path);
        if (!res)
            return ImageAsset({0, nullptr});

        int width = 0;
        int height = 0;
        int channels = 0;
        uint32_t desiredChannels = 4;

        stbi_set_flip_vertically_on_load_thread(importSettings.imageImportSettings.flip);
        unsigned char* imgData = stbi_load(res.value().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!imgData)
        {
            FL_CORE_ERROR("Can't load an image: {0} {1}", res.value(), stbi_failure_reason());
            return ImageAsset({0, nullptr});
        }

        Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
        imagePtr->PostCreate(settings);

        stbi_image_free(imgData);
    }
    else if (importSettings.imageImportSettings.imageSource == IMAGE_SOURCE_COLOR)
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t channels = Fleur::Graphics::Color::Channels(importSettings.imageImportSettings.color);

        size_t size = width * height * channels;

        uint32_t colorData = importSettings.imageImportSettings.color.Data();

        Fleur::Memory::FleurAllocator<unsigned char> alloc;
        unsigned char* pData = alloc.allocate(size);
        for (size_t i = 0; i < size; i += channels)
        {
            std::memcpy(pData + i, &colorData + i, channels);
        }

        Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, channels, 1, pData};
        image2dAsset.obj->PostCreate(info);

        alloc.deallocate(pData, size);
    }
    else if (importSettings.imageImportSettings.imageSource == IMAGE_SOURCE_MEMORY)
    {
        assert(importSettings.imageImportSettings.pMemoryData);
        assert(importSettings.imageImportSettings.sizeInMemory > 0);

        int width = 0;
        int height = 0;
        int channels = 0;
        int desiredChannels = 4;

        stbi_set_flip_vertically_on_load_thread(static_cast<int>(false));
        unsigned char* imgData =
            stbi_load_from_memory(importSettings.imageImportSettings.pMemoryData, static_cast<int>(importSettings.imageImportSettings.sizeInMemory), &width,
                                  &height, &channels, STBI_rgb_alpha);

        Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, desiredChannels, 1, imgData};

        image2dAsset.obj->PostCreate(info);
    }
    else
    {
        assert(false);
    }

    FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) has loaded", image2dAsset.ID, imagePtr->GetName(), imagePtr->GetWidth(),
                 imagePtr->GetHeight());

    return image2dAsset;
}
CubemapAsset Fleur::AssetCache<CubemapType>::Load(std::string_view path, CubemapRecord imageRecord, ResourceImportSettings importSettings)
{
    return CubemapAsset{};
}

void Fleur::AssetCache<ModelType>::LoadAsync(std::string_view path, ModelAsyncOpShared asyncOperation, ResourceImportSettings settings, AssetID id,
                                             AssetLoadCallback callback)
{
    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](ModelAsyncOpShared handle, std::string_view path, AssetLoadCallback callback)
        {
            handle->status.SetStatus(ELoadingSts::LOADING);

            ModelType* modelPtr = handle->asset.obj;

            callback.result.pResource = modelPtr;
            callback.result.type = EVENT_TYPE_MODEL_LOADED;
            callback.result.ID = handle->asset.ID;

            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            std::filesystem::path fullPath = modelPtr->GetName();

            auto res = fs->GetFullPathToFile(path);
            if (!res)
            {
                handle->status.SetStatus(ELoadingSts::CORRUPTED);
                callback.result.loadingStatus = handle->status.GetStatus();
                callback.operator()();
                return;
            }

            cgltf_options options = {};
            cgltf_data* data = NULL;
            cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
            if (result != cgltf_result_success)
            {
                handle->status.SetStatus(ELoadingSts::CORRUPTED);
                callback.result.loadingStatus = handle->status.GetStatus();
                callback.operator()();
                return;
            }

            result = cgltf_load_buffers(&options, data, res->c_str());
            if (result != cgltf_result_success)
            {
                handle->status.SetStatus(ELoadingSts::CORRUPTED);
                callback.result.loadingStatus = handle->status.GetStatus();
                callback.operator()();
                return;
            }

            Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(modelPtr->GetName(), data);
            // fabric.load
            Fleur::Graphics::Model::SFLPostCreateInfo createInfo = fabric.ProcessData();
            modelPtr->PostCreate(createInfo);
            // load async images
            // fabric.release

            cgltf_free(data);  // move to release

            FL_CORE_INFO("[AssetsManager] Model (ID: {0}, {1}) was added", handle->asset.ID, modelPtr->GetName());

            handle->status.SetStatus(ELoadingSts::LOADED);

            callback.result.loadingStatus = handle->status.GetStatus();
            callback.operator()();
        },
        asyncOperation, path, callback);
}

void Fleur::AssetCache<ImageType>::LoadAsync(std::string_view path, ImageAsyncOpShared asyncOperation, ResourceImportSettings settings, AssetID id,
                                             AssetLoadCallback callback)
{
    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](std::string_view path, ImageAsyncOpShared handle, bool flipVertical, AssetLoadCallback callback)
        {
            if (!handle->status.SetStatus(ELoadingSts::LOADING))
            {
                callback.result.loadingStatus = handle->status.GetStatus();
                callback.operator()();
                return;
            }

            ImageType* imagePtr = handle->asset.obj;

            callback.result.pResource = imagePtr;
            callback.result.type = EVENT_TYPE_IMAGE2D_LOADED;
            callback.result.ID = handle->asset.ID;

            callback.result.loadingStatus = handle->status.GetStatus();

            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            std::filesystem::path fullPath = path;

            auto res = fs->GetFullPathToFile(fullPath.string());
            if (!res)
            {
                handle->status.SetStatus(ELoadingSts::CORRUPTED);
                callback.result.loadingStatus = handle->status.GetStatus();
                callback.operator()();
                return;
            }

            int width = 0;
            int height = 0;
            int channels = 0;
            uint32_t desiredChannels = 4;

            stbi_set_flip_vertically_on_load_thread(static_cast<int>(flipVertical));
            unsigned char* imgData = stbi_load(res.value().c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!imgData)
            {
                handle->status.SetStatus(ELoadingSts::CORRUPTED);
                callback.result.loadingStatus = handle->status.GetStatus();
                callback.operator()();
                return;
            }

            Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
            imagePtr->PostCreate(settings);

            stbi_image_free(imgData);

            FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) was added", handle->asset.ID, imagePtr->GetName(), imagePtr->GetWidth(),
                         imagePtr->GetHeight());

            handle->status.SetStatus(ELoadingSts::LOADED);
            callback.result.loadingStatus = handle->status.GetStatus();
            callback.operator()();
        },
        path, asyncOperation, false, callback);
}
