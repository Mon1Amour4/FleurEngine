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
    const std::string m_ResourcePath = GetExecutablePath() + s_Resource;
    const std::string m_ShadersPath = s_ShadersWinPath;
    const std::string m_ImagesPath = "Images";
    const std::string m_ModelsPath = "Models";
    const std::string m_ScenesPath = "Scenes";
    const std::vector<std::string_view> m_SearchPaths = {m_ShadersPath.data(), m_ImagesPath.data(), m_ModelsPath.data(), m_ScenesPath.data()};
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
    std::string outName;
    extension = file.extension().string();
    if (extension.compare(".png") == 0 || extension.compare(".jpg") == 0)
    {
        std::filesystem::path filePath = d->m_ResourcePath / std::filesystem::path(d->m_ImagesPath) / fileName;
        if (std::filesystem::exists(filePath))
        {
            outName = filePath.lexically_normal().string();
        }
    }
    else if (extension.compare(".glsl") == 0)
    {
        std::filesystem::path filePath = d->m_ResourcePath / std::filesystem::path(d->m_ShadersPath) / fileName;
        if (std::filesystem::exists(filePath))
        {
            outName = filePath.lexically_normal().string();
        }
    }
    else if (extension.compare(".obj") == 0)
    {
        std::filesystem::path filePath = d->m_ResourcePath / std::filesystem::path(d->m_ModelsPath) / fileName;
        if (std::filesystem::exists(filePath))
        {
            outName = filePath.lexically_normal().string();
        }
    }

    for (const auto& path : d->m_SearchPaths)
    {
        std::filesystem::path filePath = d->m_ResourcePath / std::filesystem::path(path) / fileName;

        if (std::filesystem::exists(filePath))
        {
            outName = filePath.lexically_normal().string();
        }
    }
    if (outName.empty())
    {
        FL_CORE_ERROR("[FileSystem] GetFullPathToFile-> Can't find {0}", fileName);
        return std::nullopt;
    }
    return std::optional<std::string>(outName);
}

std::optional<std::string> FileSystem::GetFullPathToFolder(std::string_view folderName) const
{
    for (const auto& path : d->m_SearchPaths)
    {
        std::filesystem::path folderPath = ((d->m_ResourcePath / std::filesystem::path(path)).lexically_normal());
        if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath))
        {
            std::string folder = folderPath.parent_path().filename().string();
            if (folder.compare(folderName.data()) == 0)
            {
                return std::optional<std::string>(folderPath.string());
            }
        }
    }
    return std::nullopt;
}
bool FileSystem::FUCreateFile(const std::string& fileName, std::string_view folder) const
{
    auto res = GetFullPathToFolder(folder);
    if (!res)
        return false;

    std::ofstream file(std::filesystem::path(res.value()) / fileName);
    if (file)
    {
        return true;
        file.close();
    }
    else
        return false;
}
void FileSystem::WriteToFile(std::string_view fileName, const char* buffer)
{
    if (!buffer || fileName.empty())
        return;

    auto res = GetFullPathToFile(fileName);
    if (!res)
        return;

    std::ofstream file(fileName.data(), std::ios::out | std::ios::trunc);
    if (file)
    {
        file << buffer << std::endl;
        file.close();
    }
}

}  // namespace Fleur::FS
