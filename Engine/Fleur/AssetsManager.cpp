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
    : m_ForceAlpha(true)
{
}
Fleur::AssetsManager::~AssetsManager()
{
}


// ---------- Service ----------
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
        for (auto item : m_ImagesToUpload.notifyMap)
        {
            *item = true;
        }
        m_ImagesToUpload.Clear();
        m_ImagesToUpload.notifyMap.clear();
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

                if (message.notifyOnGpuUpload)
                    m_ImagesToUpload.notifyMap.emplace_back(message.notifyOnGpuUpload);

                m_Image2DCache.RemoveFromAsyncOperations(loadedImage->GetName());
                break;
            }
            }
            break;
        }
        case EVENT_TYPE_CUBEMAP_LOADED:
        {
            switch (message.loadingStatus)
            {
            case CORRUPTED:
            case LOADING_STATUS_TO_TERMINATE:
            {
                m_CubemapCache.RemoveBrokenAsyncAsset(message.ID);
                break;
            }
            case LOADED:
            {
                const Fleur::Graphics::CubemapImage* loadedImage = reinterpret_cast<const Fleur::Graphics::CubemapImage*>(message.pResource);

                Fleur::Graphics::SFLImageView view = loadedImage->GetView();
                view.ID = message.ID;
                m_ImagesToUpload.images.push_back(view);

                if (message.notifyOnGpuUpload)
                    m_ImagesToUpload.notifyMap.emplace_back(message.notifyOnGpuUpload);

                m_CubemapCache.RemoveFromAsyncOperations(loadedImage->GetName());
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
    ModelRecord modelRecord = m_ModelCache.Register(path, GetNextID());
    if (modelRecord.alreadyExist)
        return ModelAsset{modelRecord.asset};

    LoadModelInternal(path, &modelRecord.asset);

    return modelRecord.asset;
}
ImageAsset Fleur::AssetsManager::LoadImage(std::string_view path, ImageImportSettings imageSettings)
{
    ImageRecord imageRecord = m_Image2DCache.Register(path, GetNextID());
    if (imageRecord.alreadyExist)
        return ImageAsset{imageRecord.asset};

    LoadImageInternal(path, &imageRecord.asset, imageSettings);

    Fleur::Graphics::SFLImageView imageView = imageRecord.asset.obj->GetView();
    imageView.ID = imageRecord.asset.ID;
    m_ImagesToUpload.Add(imageView, nullptr);

    return imageRecord.asset;
}
CubemapAsset Fleur::AssetsManager::LoadCubemap(std::string_view path, CubemapImportSettings cubemapSettings)
{
    CubemapRecord cubemapRecord = m_CubemapCache.Register(path, GetNextID());

    ImageAsset tmpImageAsset{0, new Fleur::Graphics::Image2D};

    ImageImportSettings imageSettings{.imageSource = IMAGE_SOURCE_DISK};
    LoadImageInternal(path, &tmpImageAsset, imageSettings);

    if (cubemapSettings.sourceLayout == CUBEMAP_SOURCE_LAYOUT_EQUIRECTANGULAR_IMAGE)
    {
        *cubemapRecord.asset.obj = std::move(Fleur::Graphics::CubemapImage::FromEquirectangular(*tmpImageAsset.obj));
    }
    else if (cubemapSettings.sourceLayout == CUBEMAP_SOURCE_LAYOUT_CROSS_LAYOUT)
    {
        *cubemapRecord.asset.obj = std::move(Fleur::Graphics::CubemapImage::FromCross(*tmpImageAsset.obj));
    }
    else if (cubemapSettings.sourceLayout == CUBEMAP_SOURCE_LAYOUT_6_IMAGES)
    {
        // TODO
        assert(false);
        //*cubemapRecord.asset.obj = std::move(Fleur::Graphics::CubemapImage::FromFaces(*tmpImageAsset.obj));
    }

    Fleur::Graphics::SFLImageView imageView = cubemapRecord.asset.obj->GetView();
    imageView.ID = cubemapRecord.asset.ID;
    m_ImagesToUpload.Add(imageView, nullptr);

    delete tmpImageAsset.obj;

    return cubemapRecord.asset;
}


// ---------- Async ----------
ModelAsyncOpShared Fleur::AssetsManager::LoadModelAsync(std::string_view path, std::function<void(ModelAsset&)> callback)
{
    ModelAsyncOpShared sharedOperation = m_ModelCache.RegisterAsync(path, GetNextID());
    if (sharedOperation->status.GetStatus() != REGISTERED)
        return sharedOperation;

    sharedOperation->status.SetStatus(TO_BE_LOADED);

    AssetLoadCallback internalCallback{};
    internalCallback.manager = this;
    internalCallback.callback = &Fleur::AssetsManager::OnAssetLoaded;

    LoadModelAsyncInternal(path, sharedOperation, internalCallback, callback);

    return sharedOperation;
}
ImageAsyncOpShared Fleur::AssetsManager::LoadImageAsync(std::string_view path, ImageImportSettings imageSettings = {.imageSource = IMAGE_SOURCE_DISK},
                                                        std::function<void(ImageAsset&)> callback, CallbackInvocationPoint callbackType)
{
    ImageAsyncOpShared sharedOperation = m_Image2DCache.RegisterAsync(path, GetNextID());
    if (sharedOperation->status.GetStatus() != REGISTERED)
        return sharedOperation;

    sharedOperation->status.SetStatus(TO_BE_LOADED);

    AssetLoadCallback internalCallback{};
    internalCallback.manager = this;
    internalCallback.callback = &Fleur::AssetsManager::OnAssetLoaded;

    LoadImageAsyncInternal(path, sharedOperation, imageSettings, internalCallback, callback, callbackType);

    return sharedOperation;
}
CubemapAsyncOpShared Fleur::AssetsManager::LoadCubemapAsync(std::string_view path, CubemapImportSettings cubemapSettings,
                                                            std::function<void(CubemapAsset&)> clientCallback, CallbackInvocationPoint callbackType)
{
    CubemapAsyncOpShared sharedOperation = m_CubemapCache.RegisterAsync(path, GetNextID());
    if (sharedOperation->status.GetStatus() != REGISTERED)
        return sharedOperation;

    sharedOperation->status.SetStatus(TO_BE_LOADED);

    AssetLoadCallback internalCallback{};
    internalCallback.manager = this;
    internalCallback.callback = &Fleur::AssetsManager::OnAssetLoaded;

    LoadCubemapAsyncInternal(path, sharedOperation, cubemapSettings, internalCallback, clientCallback, callbackType);

    return sharedOperation;
}


// ---------- Internal ----------
void Fleur::AssetsManager::LoadModelInternal(std::string_view path, ModelAsset* asset)
{
    ModelType* modelPtr = asset->obj;
    auto fileSystem = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fileSystem->GetFullPathToFile(path);
    if (!res)
    {
        FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, asset->ID, modelPtr->GetName());
        assert(false);
        return;
    }

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
    if (result != cgltf_result_success)
    {
        FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, asset->ID, modelPtr->GetName());
        assert(false);
        return;
    }

    result = cgltf_load_buffers(&options, data, res->c_str());
    if (result != cgltf_result_success)
    {
        FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, asset->ID, modelPtr->GetName());
        assert(false);
        return;
    }

    Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(res->c_str(), data);
    // fabric.load
    Fleur::Graphics::Model::SFLPostCreateInfo createInfo = fabric.ProcessData();
    // create images -> add to upload
    // fabric.release
    modelPtr->PostCreate(createInfo);

    cgltf_free(data);

    FL_CORE_INFO("[AssetsManager] Model (ID: {0}, {1}) has loaded", asset->ID, modelPtr->GetName());
}
void Fleur::AssetsManager::LoadImageInternal(std::string_view path, ImageAsset* asset, ImageImportSettings& imageSettings)
{
    ImageType* imagePtr = asset->obj;
    if (imageSettings.imageSource == IMAGE_SOURCE_DISK)
    {
        LoadImageFromDisk(path, asset, imageSettings);
    }
    else if (imageSettings.imageSource == IMAGE_SOURCE_COLOR)
    {
        LoadImageFromColor(asset, imageSettings);
    }
    else if (imageSettings.imageSource == IMAGE_SOURCE_MEMORY)
    {
        LoadImageFromMemory(asset, imageSettings);
    }
    else
    {
        assert(false);
    }

    FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) has loaded", asset->ID, imagePtr->GetName(), imagePtr->GetWidth(), imagePtr->GetHeight());
}

void Fleur::AssetsManager::LoadModelAsyncInternal(std::string_view path, ModelAsyncOpShared sharedOperation, AssetLoadCallback& internalCallback,
                                                  std::function<void(ModelAsset&)> clientCallback)
{
    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();
    threadPool->Submit(
        [this](ModelAsyncOpShared handle, std::string_view path, AssetLoadCallback internalCallback, std::function<void(ModelAsset&)> clientCallback)
        {
            handle->status.SetStatus(EAsyncOperationStatus::LOADING);

            ModelType* modelPtr = handle->asset.obj;

            internalCallback.result.pResource = modelPtr;
            internalCallback.result.type = EVENT_TYPE_MODEL_LOADED;
            internalCallback.result.ID = handle->asset.ID;

            auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

            std::filesystem::path fullPath = modelPtr->GetName();

            auto res = fs->GetFullPathToFile(path);
            if (!res)
            {
                FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, handle->asset.ID, modelPtr->GetName());
                assert(false);
                handle->status.SetStatus(EAsyncOperationStatus::CORRUPTED);
                internalCallback.result.loadingStatus = handle->status.GetStatus();
                internalCallback.operator()();
                return;
            }

            cgltf_options options = {};
            cgltf_data* data = NULL;
            cgltf_result result = cgltf_parse_file(&options, res->c_str(), &data);
            if (result != cgltf_result_success)
            {
                FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, handle->asset.ID, modelPtr->GetName());
                assert(false);
                handle->status.SetStatus(EAsyncOperationStatus::CORRUPTED);
                internalCallback.result.loadingStatus = handle->status.GetStatus();
                internalCallback.operator()();
                return;
            }

            result = cgltf_load_buffers(&options, data, res->c_str());
            if (result != cgltf_result_success)
            {
                FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, handle->asset.ID, modelPtr->GetName());
                assert(false);
                handle->status.SetStatus(EAsyncOperationStatus::CORRUPTED);
                internalCallback.result.loadingStatus = handle->status.GetStatus();
                internalCallback.operator()();
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

            handle->status.SetStatus(EAsyncOperationStatus::LOADED);

            internalCallback.result.loadingStatus = handle->status.GetStatus();
            internalCallback.operator()();

            if (clientCallback)
                clientCallback.operator()(handle->asset);
        },
        sharedOperation, path, internalCallback, clientCallback);
}

void Fleur::AssetsManager::LoadImageAsyncInternal(std::string_view path, ImageAsyncOpShared sharedOperation, ImageImportSettings& imageSettings,
                                                  AssetLoadCallback& internalCallback, std::function<void(ImageAsset&)> clientCallback,
                                                  CallbackInvocationPoint callbackType)
{
    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](std::string_view path, ImageAsyncOpShared handle, ImageImportSettings imageSettings, AssetLoadCallback internalCallback,
               std::function<void(ImageAsset&)> clientCallback, CallbackInvocationPoint callbackType)
        {
            ImageType* imagePtr = handle->asset.obj;

            if (callbackType == AFTER_GPU_UPLOAD)
                internalCallback.result.notifyOnGpuUpload = &handle->isGpuUploaded;

            if (!handle->status.SetStatus(EAsyncOperationStatus::LOADING))
            {
                FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, handle->asset.ID, imagePtr->GetName());
                assert(false);
                internalCallback.result.loadingStatus = handle->status.GetStatus();
                internalCallback.operator()();
                return;
            }


            internalCallback.result.pResource = imagePtr;
            internalCallback.result.type = EVENT_TYPE_IMAGE2D_LOADED;
            internalCallback.result.ID = handle->asset.ID;

            internalCallback.result.loadingStatus = handle->status.GetStatus();

            if (imageSettings.imageSource == IMAGE_SOURCE_DISK)
            {
                auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

                std::filesystem::path fullPath = path;

                auto res = fs->GetFullPathToFile(fullPath.string());
                if (!res)
                {
                    FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, handle->asset.ID, imagePtr->GetName());
                    assert(false);
                    handle->status.SetStatus(EAsyncOperationStatus::CORRUPTED);
                    internalCallback.result.loadingStatus = handle->status.GetStatus();
                    internalCallback();
                    return;
                }

                int width = 0;
                int height = 0;
                int channels = 0;
                int desiredChannels = STBI_default;
                if (m_ForceAlpha)
                    desiredChannels = STBI_rgb_alpha;

                stbi_set_flip_vertically_on_load_thread(static_cast<int>(imageSettings.flip));
                unsigned char* imgData = stbi_load(res.value().c_str(), &width, &height, &channels, desiredChannels);

                if (!imgData)
                {
                    FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, handle->asset.ID, imagePtr->GetName());
                    assert(false);
                    handle->status.SetStatus(EAsyncOperationStatus::CORRUPTED);
                    internalCallback.result.loadingStatus = handle->status.GetStatus();
                    internalCallback();
                    return;
                }

                Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
                imagePtr->PostCreate(settings);

                stbi_image_free(imgData);
            }
            else if (imageSettings.imageSource == IMAGE_SOURCE_COLOR)
            {
                uint32_t width = 1;
                uint32_t height = 1;
                uint32_t channels = Fleur::Graphics::Color::Channels(imageSettings.color);

                size_t size = width * height * channels;

                uint32_t colorData = imageSettings.color.Data();

                Fleur::Memory::FleurAllocator<unsigned char> alloc;
                unsigned char* pData = alloc.allocate(size);
                for (size_t i = 0; i < size; i += channels)
                {
                    std::memcpy(pData + i, &colorData + i, channels);
                }

                Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, channels, 1, pData};
                imagePtr->PostCreate(info);

                alloc.deallocate(pData, size);
            }
            else
            {
                assert(false);
            }
            FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) was added", handle->asset.ID, imagePtr->GetName(), imagePtr->GetWidth(),
                         imagePtr->GetHeight());

            handle->status.SetStatus(EAsyncOperationStatus::LOADED);
            internalCallback.result.loadingStatus = handle->status.GetStatus();
            internalCallback();

            if (clientCallback)
            {
                if (callbackType == AFTER_CPU_LOAD)
                {
                    clientCallback(handle->asset);
                    return;
                }
                else if (callbackType == AFTER_GPU_UPLOAD)
                {
                    while (!handle->isGpuUploaded)
                    {
                    };
                    clientCallback(handle->asset);
                }
                else
                {
                    assert(false);
                }
            }
        },
        path, sharedOperation, imageSettings, internalCallback, clientCallback, callbackType);
}
// clang-format off
void Fleur::AssetsManager::LoadCubemapAsyncInternal(std::string_view path,
                                                    CubemapAsyncOpShared sharedOperation, 
                                                    CubemapImportSettings& cubemapSettings,
                                                    AssetLoadCallback& internalCallback,
                                                    std::function<void(CubemapAsset&)> clientCallback, 
                                                    CallbackInvocationPoint callbackType)
{
    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](std::string_view path, CubemapAsyncOpShared asyncOperation, CubemapImportSettings cubemapSettings, AssetLoadCallback internalCallback,
               std::function<void(CubemapAsset&)> clientCallback, CallbackInvocationPoint callbackType)
        {
            asyncOperation->status.SetStatus(EAsyncOperationStatus::LOADING);
            if (callbackType == AFTER_GPU_UPLOAD)
                internalCallback.result.notifyOnGpuUpload = &asyncOperation->isGpuUploaded;

            Fleur::Graphics::Image2D* image2d = new Fleur::Graphics::Image2D(path);
            ImageAsset tmpImageAsset{asyncOperation->asset.ID, image2d};

            Fleur::ImageImportSettings imageSettings{.imageSource = IMAGE_SOURCE_DISK};
            LoadImageInternal(path, &tmpImageAsset, imageSettings);


            if (cubemapSettings.sourceLayout == CUBEMAP_SOURCE_LAYOUT_EQUIRECTANGULAR_IMAGE)
            {
                *asyncOperation->asset.obj = Fleur::Graphics::CubemapImage::FromEquirectangular(*image2d);
            }
            else if (cubemapSettings.sourceLayout == CUBEMAP_SOURCE_LAYOUT_CROSS_LAYOUT)
            {
                *asyncOperation->asset.obj = std::move(Fleur::Graphics::CubemapImage::FromCross(*image2d));
            }
            else if (cubemapSettings.sourceLayout == CUBEMAP_SOURCE_LAYOUT_6_IMAGES)
            {
                assert(false);
            }

            delete image2d;

            internalCallback.result.pResource = asyncOperation->asset.obj;
            internalCallback.result.type = EVENT_TYPE_CUBEMAP_LOADED;
            internalCallback.result.ID = asyncOperation->asset.ID;
            asyncOperation->status.SetStatus(EAsyncOperationStatus::LOADED);
            internalCallback.result.loadingStatus = asyncOperation->status.GetStatus();
            internalCallback();
            
            if (clientCallback)
            {
                if (callbackType == AFTER_CPU_LOAD)
                {
                    clientCallback(asyncOperation->asset);
                    return;
                }
                else if (callbackType == AFTER_GPU_UPLOAD)
                {
                    while(!asyncOperation->isGpuUploaded){};
                    clientCallback(asyncOperation->asset);
                }
                else
                {
                    assert(false);
                }
            }
        },
        path, sharedOperation, cubemapSettings, internalCallback, clientCallback, callbackType);
}
// clang-format on

void Fleur::AssetsManager::LoadImageFromDisk(std::string_view path, ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings)
{
    ImageType* imagePtr = imageAsset->obj;
    AssetID imageID = imageAsset->ID;

    auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fs->GetFullPathToFile(path);
    if (!res)
    {
        FL_CORE_ERROR("{0} ID: {1}, Name: {2}", CORRUPTED_ASSET_ERROR_MESSAGE, imageID, imagePtr->GetName());
        assert(false);
        return;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    int desiredChannels = STBI_default;
    if (m_ForceAlpha)
        desiredChannels = STBI_rgb_alpha;

    stbi_set_flip_vertically_on_load_thread(imageSettings.flip);
    unsigned char* imgData = stbi_load(res.value().c_str(), &width, &height, &channels, desiredChannels);
    if (!imgData)
    {
        FL_CORE_ERROR("{0} ID: {1}, Name: {2}, Reason: {3}", CORRUPTED_ASSET_ERROR_MESSAGE, imageID, imagePtr->GetName(), stbi_failure_reason());
        assert(false);
        return;
    }

    Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
    imagePtr->PostCreate(settings);

    stbi_image_free(imgData);
}
void Fleur::AssetsManager::LoadImageFromColor(ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings)
{
    ImageType* imagePtr = imageAsset->obj;
    AssetID imageID = imageAsset->ID;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t channels = Fleur::Graphics::Color::Channels(imageSettings.color);

    size_t size = width * height * channels;

    uint32_t colorData = imageSettings.color.Data();

    Fleur::Memory::FleurAllocator<unsigned char> alloc;
    unsigned char* pData = alloc.allocate(size);
    for (size_t i = 0; i < size; i += channels)
    {
        std::memcpy(pData + i, &colorData + i, channels);
    }

    Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, channels, 1, pData};
    imagePtr->PostCreate(info);

    alloc.deallocate(pData, size);
}
void Fleur::AssetsManager::LoadImageFromMemory(ImageAsset* imageAsset, Fleur::ImageImportSettings& imageSettings)
{
    ImageType* imagePtr = imageAsset->obj;
    AssetID imageID = imageAsset->ID;
    assert(imageSettings.pMemoryData);
    assert(imageSettings.sizeInMemory > 0);

    int width = 0;
    int height = 0;
    int channels = 0;
    int desiredChannels = STBI_default;
    if (m_ForceAlpha)
        desiredChannels = STBI_rgb_alpha;


    stbi_set_flip_vertically_on_load_thread(static_cast<int>(false));
    unsigned char* imgData =
        stbi_load_from_memory(imageSettings.pMemoryData, static_cast<int>(imageSettings.sizeInMemory), &width, &height, &channels, desiredChannels);

    Fleur::Graphics::ImagePostCreation info{(uint32_t)width, (uint32_t)height, desiredChannels, 1, imgData};

    imagePtr->PostCreate(info);
}