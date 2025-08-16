#include "FileSystem.h"

#if defined(FLEUR_PLATFORM_WIN)
#include "FileSystemPathsWin.h"
#endif

#if defined(FLEUR_PLATFORM_MACOS)
#include <limits.h>
#include <mach-o/dyld.h>

#include "FileSystemPathsMacOS.h"
#endif

namespace Fleur::FS
{
class FileSystem::FileSystemImpl
{
    friend class FileSystem;
    std::string GetExecutablePath();
    const std::string resource_path = GetExecutablePath() + resource;
    const std::string shaders_path = shaders_win_path;
    const std::string images_path = "Images";
    const std::string models_path = "Models";
    const std::string scenes_path = "Scenes";
    const std::vector<std::string_view> _searchPaths = {shaders_path.data(), images_path.data(), models_path.data(), scenes_path.data()};
};

FileSystem::FileSystem()
    : d(new FileSystemImpl())
{
}

void FileSystem::OnInit()
{
    // TODO
}

void FileSystem::OnShutdown()
{
    // TODO
}

std::optional<std::string> FileSystem::OpenFile(const std::string& file, std::fstream::ios_base::openmode mode)
{
    auto res = GetFullPathToFile(file);
    if (!res)
        return std::nullopt;

    std::fstream f(res.value(), mode);
    FL_CORE_ASSERT(f.is_open(), "[FS] failed to open a file");

    std::stringstream buffer;
    buffer << f.rdbuf();
    f.close();

    return std::optional<std::string>(buffer.str());
}

std::string FileSystem::FileSystemImpl::GetExecutablePath()
{
#if defined(FLEUR_PLATFORM_WIN)
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
#elif defined(FLEUR_PLATFORM_MACOS)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0)
    {
        throw std::runtime_error("Buffer too small for executable path");
    }
#endif
    return std::filesystem::path(path).parent_path().string();
}

std::optional<std::string> FileSystem::GetFullPathToFile(std::string_view fileName) const
{
    if (fileName.empty())
        return std::nullopt;

    std::filesystem::path file(fileName);
    std::string extension = file.extension().string();
    std::string out_name;
    extension = file.extension().string();
    if (extension.compare(".png") == 0 || extension.compare(".jpg") == 0)
    {
        std::filesystem::path filePath = d->resource_path / std::filesystem::path(d->images_path) / fileName;
        if (std::filesystem::exists(filePath))
        {
            out_name = filePath.lexically_normal().string();
        }
    }
    else if (extension.compare(".glsl") == 0)
    {
        std::filesystem::path filePath = d->resource_path / std::filesystem::path(d->shaders_path) / fileName;
        if (std::filesystem::exists(filePath))
        {
            out_name = filePath.lexically_normal().string();
        }
    }
    else if (extension.compare(".obj") == 0)
    {
        std::filesystem::path filePath = d->resource_path / std::filesystem::path(d->models_path) / fileName;
        if (std::filesystem::exists(filePath))
        {
            out_name = filePath.lexically_normal().string();
        }
    }

    for (const auto& path : d->_searchPaths)
    {
        std::filesystem::path filePath = d->resource_path / std::filesystem::path(path) / fileName;

        if (std::filesystem::exists(filePath))
        {
            out_name = filePath.lexically_normal().string();
        }
    }
    if (out_name.empty())
    {
        FL_CORE_ERROR("[FileSystem] GetFullPathToFile-> Can't find {0}", fileName);
        return std::nullopt;
    }
    return std::optional<std::string>(out_name);
}

std::optional<std::string> FileSystem::GetFullPathToFolder(std::string_view folder_name) const
{
    for (const auto& path : d->_searchPaths)
    {
        std::filesystem::path folder_path = ((d->resource_path / std::filesystem::path(path)).lexically_normal());
        if (std::filesystem::exists(folder_path) && std::filesystem::is_directory(folder_path))
        {
            std::string folder = folder_path.parent_path().filename().string();
            if (folder.compare(folder_name.data()) == 0)
            {
                return std::optional<std::string>(folder_path.string());
            }
        }
    }
    return std::nullopt;
}
bool FileSystem::FUCreateFile(const std::string& file_name, std::string_view folder) const
{
    auto res = GetFullPathToFolder(folder);
    if (!res)
        return false;

    std::ofstream file(std::filesystem::path(res.value()) / file_name);
    if (file)
    {
        return true;
        file.close();
    }
    else
        return false;
}
void FileSystem::WriteToFile(std::string_view file_name, const char* buffer)
{
    if (!buffer || file_name.empty())
        return;

    auto res = GetFullPathToFile(file_name);
    if (!res)
        return;

    std::ofstream file(file_name.data(), std::ios::out | std::ios::trunc);
    if (file)
    {
        file << buffer << std::endl;
        file.close();
    }
}

}  // namespace Fleur::FS
