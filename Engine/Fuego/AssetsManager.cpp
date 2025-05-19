#include "AssetsManager.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

#include "External/stb_image/stb_image.h"
#include "FileSystem/FileSystem.h"
#include "Services/ServiceLocator.h"

#define ASSIMP_LOAD_FLAGS aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType

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

std::shared_ptr<Fuego::Graphics::Model> Fuego::AssetsManager::load_model(std::string_view path)
{
    if (path.empty())
        return std::shared_ptr<Fuego::Graphics::Model>();

    std::string file_name = std::filesystem::path(path.data()).filename().replace_extension().string();
    auto it = models.find(file_name);
    if (it != models.end())
        return it->second;

    Assimp::Importer importer{};
    const aiScene* scene = importer.ReadFile(ServiceLocator::instance().GetService<Fuego::FS::FileSystem>()->GetFullPathToFile(path.data()), ASSIMP_LOAD_FLAGS);
    if (!scene)
        return nullptr;
    std::lock_guard lock(models_async_operations);
    auto model = models.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Model>(scene)).first->second;
    ++models_count;
    FU_CORE_INFO("[AssetsManager] Model[{0}] was added: name: {1}, ", models.size(), model->GetName());
    return model;
}
std::shared_ptr<Fuego::Graphics::Image2D> Fuego::AssetsManager::load_image2d(std::string_view path, Fuego::Graphics::ImageFormat format)
{
    if (path.empty())
        return std::shared_ptr<Fuego::Graphics::Image2D>();

    std::string file_name = std::filesystem::path(path.data()).filename().replace_extension().string();
    auto image = images2d.find(file_name);
    if (image != images2d.end())
        return image->second;

    stbi_set_flip_vertically_on_load(1);
    int w, h, bpp = 0;
    unsigned char* data = stbi_load(ServiceLocator::instance().GetService<Fuego::FS::FileSystem>()->GetFullPathToFile(path.data()).c_str(), &w, &h, &bpp,
                                    Fuego::Graphics::Image2D::Channels(format));
    if (!data)
    {
        FU_CORE_ERROR("Can't load an image: {0} {1}", path, stbi_failure_reason());
        return nullptr;
    }
    auto img = images2d.emplace(std::move(file_name), std::make_shared<Fuego::Graphics::Image2D>(file_name, data, w, h, bpp, format)).first->second;
    ++images2d_count;
    FU_CORE_INFO("[AssetsManager] Image[{0}] was added: name: {1}, format: {2}, width: {3}, height: {4}", images2d.size(), img->Name(), img->PrintFormat(),
                 img->Width(), img->Height());
    return img;
}
