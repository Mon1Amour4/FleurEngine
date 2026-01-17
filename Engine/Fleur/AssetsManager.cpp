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

using ImageType = Fleur::Graphics::Image2D;
using ShaderType = Fleur::Graphics::Shader;
using ModelType = Fleur::Graphics::Model;

using ImageRecord = Fleur::AssetRecord<Fleur::Graphics::Image2D>;
using ModelRecord = Fleur::AssetRecord<Fleur::Graphics::Model>;

using ImageAsset = Fleur::Asset<ImageType>;
using ModelAsset = Fleur::Asset<ModelType>;

using ImageAsyncOp = Fleur::AsyncOperation<ImageType>;
using ModelAsyncOp = Fleur::AsyncOperation<ModelType>;

using ImageAsyncOpShared = std::shared_ptr<ImageAsyncOp>;
using ModelAsyncOpShared = std::shared_ptr<ModelAsyncOp>;

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

        Fleur::Graphics::SFLImageViewInfo info{};
        info.pData = m_ImagesToUpload.images.data();
        info.count = m_ImagesToUpload.images.size();
        renderer->SubmitImageViews(&info);

        imagesWereUploaded = true;
        m_ImagesToUpload.framesSinceLastUpload = 0;
        m_ImagesToUpload.images.clear();
    }


    if (!imagesWereUploaded)
        m_ImagesToUpload.framesSinceLastUpload++;
}


//======================================================================
// Image2D
ImageAsyncOpShared Fleur::AssetCache<ImageType>::LoadAsync(std::string_view path, AssetID id, AssetLoadCallback onLoaded)
{
    std::string fileName = std::filesystem::path(path).filename().string();

    ImageRecord record = Add(fileName, id);
    if (record.alreadyExist)
    {
        if (auto operation = asyncMap.find(record.asset.ID); operation != asyncMap.end())
        {
            // In progress
            return operation->second;
        }
        else
        {
            // Done
            return {std::make_shared<ImageAsyncOp>(record.asset, ELoadingSts::SUCCESS)};
        }
    };

    ImageAsset image2dAsset = record.asset;

    ImageAsyncOpShared asyncOperation = asyncMap.emplace(id, std::make_shared<ImageAsyncOp>(image2dAsset, ELoadingSts::TO_BE_LOADED)).first->second;

    auto threadPool = ServiceLocator::instance().GetService<ThreadPool>();

    threadPool->Submit(
        [this](ImageAsyncOpShared handle, bool flipVertical, AssetLoadCallback callback)
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

            std::filesystem::path fullPath = imagePtr->Name();

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

            FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) was added", handle->asset.ID, imagePtr->Name(), imagePtr->Width(), imagePtr->Height());

            handle->status.SetStatus(ELoadingSts::SUCCESS);
            callback.result.loadingStatus = handle->status.GetStatus();
            callback.operator()();
        },
        asyncOperation, false, onLoaded);

    return asyncOperation;
}

ImageAsset Fleur::AssetCache<ImageType>::Load(std::string_view path, Fleur::AssetID id)
{
    std::string fileName = std::filesystem::path(path).filename().string();

    ImageRecord image2dRecord = Add(fileName, id);
    if (image2dRecord.alreadyExist)
        return image2dRecord.asset;

    ImageAsset image2dAsset = image2dRecord.asset;
    ImageType* imagePtr = image2dRecord.asset.obj;

    auto fs = ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();

    auto res = fs->GetFullPathToFile(path);
    if (!res)
        return ImageAsset({0, nullptr});

    int width = 0;
    int height = 0;
    int channels = 0;
    uint32_t desiredChannels = 4;

    stbi_set_flip_vertically_on_load_thread(static_cast<int>(false));
    unsigned char* imgData = stbi_load(res.value().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!imgData)
    {
        FL_CORE_ERROR("Can't load an image: {0} {1}", fileName, stbi_failure_reason());
        return ImageAsset({0, nullptr});
    }

    Fleur::Graphics::ImagePostCreation settings{(uint32_t)(width), (uint32_t)(height), (uint16_t)(desiredChannels), 1, imgData};
    imagePtr->PostCreate(settings);

    FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) was added", image2dAsset.ID, imagePtr->Name(), imagePtr->Width(), imagePtr->Height());

    stbi_image_free(imgData);

    return image2dAsset;
}
ImageAsset Fleur::AssetsManager::FromColor(std::string_view name, Fleur::Graphics::Color color)
{
    using value_type = Fleur::Graphics::Image2D;
    AssetID id = m_GlobalId++;

    ImageRecord image2dRecord = m_Image2DCache.Add(name, id);
    if (image2dRecord.alreadyExist)
        return image2dRecord.asset;

    ImageAsset image2dAsset = image2dRecord.asset;

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
    {
        m_ImagesToUpload.images.push_back(imageView);
    }

    FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) was added", id, image2dAsset.obj->Name(), image2dAsset.obj->Width(),
                 image2dAsset.obj->Height());

    return image2dAsset;
}
ImageAsset Fleur::AssetsManager::LoadImageFromMemory(std::string_view name, unsigned char* pData, size_t size)
{
    assert(pData && size > 0);

    using value_type = Fleur::Graphics::Image2D;
    AssetID id = m_GlobalId++;

    ImageRecord image2dRecord = m_Image2DCache.Add(name, id);
    if (image2dRecord.alreadyExist)
        return image2dRecord.asset;

    ImageAsset image2dAsset = image2dRecord.asset;

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
    {
        m_ImagesToUpload.images.push_back(imageView);
    }

    FL_CORE_INFO("[AssetsManager] Image (ID: {0}, {1}, {2}, {3}) was added", id, image2dAsset.obj->Name(), image2dAsset.obj->Width(),
                 image2dAsset.obj->Height());

    return image2dAsset;
}


//======================================================================
// Model
ModelAsset Fleur::AssetCache<ModelType>::Load(std::string_view path, Fleur::AssetID id)
{
    if (path.empty())
        return ModelAsset{0, nullptr};

    std::string fileName = std::filesystem::path(path).stem().string();

    ModelRecord modelRecord = Add(fileName, id);
    if (modelRecord.alreadyExist)
        return modelRecord.asset;

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

    Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(fileName, data);
    // fabric.load
    Fleur::Graphics::Model::SFLPostCreateInfo createInfo = fabric.ProcessData();
    // create images -> add to upload
    // fabric.release
    modelPtr->PostCreate(createInfo);

    cgltf_free(data);

    FL_CORE_INFO("[AssetsManager] Model (ID: {0}, {1}) was added", modelAsset.ID, modelPtr->Name());

    return modelAsset;
}
ModelAsyncOpShared Fleur::AssetCache<ModelType>::LoadAsync(std::string_view path, AssetID id, AssetLoadCallback onLoaded)
{
    std::string fileName = std::filesystem::path(path).filename().string();

    ModelRecord record = Add(fileName, id);
    if (record.alreadyExist)
    {
        if (auto operation = asyncMap.find(record.asset.ID); operation != asyncMap.end())
        {
            // In progress
            return operation->second;
        }
        else
        {
            // Done
            return {std::make_shared<ModelAsyncOp>(record.asset, ELoadingSts::SUCCESS)};
        }
    };

    ModelAsset modelAsset = record.asset;

    ModelAsyncOpShared asyncOperation = asyncMap.emplace(id, std::make_shared<ModelAsyncOp>(modelAsset, ELoadingSts::TO_BE_LOADED)).first->second;

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

            std::filesystem::path fullPath = modelPtr->Name();

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

            Fleur::Graphics::CGLTFModelFabric fabric = Fleur::Graphics::CGLTFModelFabric(modelPtr->Name(), data);
            // fabric.load
            Fleur::Graphics::Model::SFLPostCreateInfo createInfo = fabric.ProcessData();
            modelPtr->PostCreate(createInfo);
            // load async images
            // fabric.release

            cgltf_free(data);  // move to release

            FL_CORE_INFO("[AssetsManager] Model (ID: {0}, {1}) was added", handle->asset.ID, modelPtr->Name());

            handle->status.SetStatus(ELoadingSts::SUCCESS);

            callback.result.loadingStatus = handle->status.GetStatus();
            callback.operator()();
        },
        asyncOperation, path, onLoaded);

    return asyncOperation;
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
            case SUCCESS:
            {
                auto renderer = Fleur::ServiceLocator::instance().GetService<Fleur::Graphics::Renderer>();
                glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 100.f));
                glm::mat4 R = glm::mat4(1.f);
                glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(0.1f, 0.1f, 0.1f));
                glm::mat4 M = T * R * S;
                renderer->DrawModel(Fleur::Graphics::STATIC_GEOMETRY, (Fleur::Graphics::Model*)message.pResource, M);
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
            case SUCCESS:
            {
                const Fleur::Graphics::Image2D* loadedImage = reinterpret_cast<const Fleur::Graphics::Image2D*>(message.pResource);

                Fleur::Graphics::SFLImageView view = loadedImage->GetView();
                view.ID = message.ID;
                m_ImagesToUpload.images.push_back(view);
                break;
            }
            }
        }
        }

        m_MessageQueue.pop_front();
    }
}
