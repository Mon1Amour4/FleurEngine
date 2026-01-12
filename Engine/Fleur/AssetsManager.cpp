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
using ImageAsyncOpShared = std::shared_ptr<ImageAsyncOp>;

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
ImageAsyncOpShared Fleur::AssetCache<ImageType>::LoadAsync(std::string_view path, Fleur::AssetID id)
{
    std::string fileName = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

    ImageRecord record = Add(fileName, id);
    if (record.alreadyExist)
    {
        ImageAsyncOpShared operation = std::make_shared<ImageAsyncOp>();
        operation->status = Fleur::ELoadingSts::SUCCESS;
        operation->asset = ImageAsset{record.asset.ID, record.asset.obj};
        return operation;
    };

    ImageAsset image2dAsset = record.asset;

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

            FL_CORE_INFO("[AssetsManager] Image ({0}, {1}, {2}, {3}) was added", handle->asset.ID, imagePtr->Name(), imagePtr->Width(), imagePtr->Height());

            handle->status = ELoadingSts::SUCCESS;
        },
        sharedOP, false);

    return sharedOP;
}

ImageAsset Fleur::AssetCache<ImageType>::Load(std::string_view path, Fleur::AssetID id)
{
    std::string fileName = std::filesystem::path(path).filename().stem().string();
    std::string ext = std::filesystem::path(path).extension().string();

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

    Fleur::Graphics::SFLImageView imageView = imagePtr->GetView();
    imageView.ID = image2dAsset.ID;

    // AddImageToUpload(imageView);

    FL_CORE_INFO("[AssetsManager] Image ({0}, {1}, {2}, {3}) was added", image2dAsset.ID, imagePtr->Name(), imagePtr->Width(), imagePtr->Height());

    stbi_image_free(imgData);

    return image2dAsset;
}
ImageRecord Fleur::AssetCache<ImageType>::Add(std::string_view name, AssetID id)
{
    ImageRecord record = Exist(name);
    if (record.alreadyExist)
        return record;

    stringMap.emplace(name, id);
    Fleur::Graphics::Image2D* image2DPtr = &map.emplace(id, Fleur::Graphics::Image2D(name, "")).first->second;
    m_size++;

    return {true, false, {id, image2DPtr}};
}
ImageRecord Fleur::AssetCache<ImageType>::Exist(std::string_view name)
{
    Fleur::AssetRecord<ImageType> record{false, false, {0, nullptr}};

    if (auto rec = stringMap.find(name.data()); rec != stringMap.end())
    {
        record.registered = true;
        record.alreadyExist = true;
        record.asset.ID = stringMap[name.data()];
        record.asset.obj = &map[record.asset.ID];
    }

    return record;
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
    // AddImageToUpload(imageView);

    FL_CORE_INFO("[AssetsManager] Image ({0}, {1}, {2}, {3}) was added", id, image2dAsset.obj->Name(), image2dAsset.obj->Width(), image2dAsset.obj->Height());

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
    // AddImageToUpload(imageView);

    FL_CORE_INFO("[AssetsManager] Image ({0}, {1}, {2}, {3}) was added", id, image2dAsset.obj->Name(), image2dAsset.obj->Width(), image2dAsset.obj->Height());

    return image2dAsset;
}
ImageAsset Fleur::AssetCache<ImageType>::Get(std::string_view name)
{
    return Exist(name).asset;
}
ImageAsset Fleur::AssetCache<ImageType>::Get(Fleur::AssetID id)
{
    ImageAsset image2dAsset{0, nullptr};
    if (auto rec = map.find(id); rec != map.end())
    {
        image2dAsset.ID = id;
        image2dAsset.obj = &rec->second;
    }

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
    Fleur::Graphics::Model::SFLPostCreateInfo createInfo = fabric.ProcessData();
    modelPtr->PostCreate(createInfo);

    cgltf_free(data);

    FL_CORE_INFO("[AssetsManager] Model ({0}, {1}) was added", modelAsset.ID, modelPtr->GetName());

    return modelAsset;
}
ModelRecord Fleur::AssetCache<ModelType>::Add(std::string_view name, Fleur::AssetID id)
{
    ModelRecord record = Exist(name);
    if (record.registered)
        return record;

    stringMap.emplace(name, id);
    Fleur::Graphics::Model* modelPtr = &map.emplace(id, Fleur::Graphics::Model(name)).first->second;
    m_size++;

    return {true, false, {id, modelPtr}};
}
ModelRecord Fleur::AssetCache<ModelType>::Exist(std::string_view name)
{
    Fleur::AssetRecord<ModelType> record{false, false, {0, nullptr}};

    if (auto rec = stringMap.find(name.data()); rec != stringMap.end())
    {
        record.registered = true;
        record.alreadyExist = true;
        record.asset.ID = stringMap[name.data()];
        record.asset.obj = &map[record.asset.ID];
    }

    return record;
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
