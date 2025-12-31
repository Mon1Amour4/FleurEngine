#pragma once

#include <filesystem>
#include <fstream>

#include "Services/ServiceInterfaces.hpp"

namespace Fleur::Graphics
{
class Model;
}

namespace Fleur::FS
{

class Application;

class FLEUR_API FileSystem : public Service<FileSystem>
{
public:
    friend struct Service<FileSystem>;

    FileSystem(FileSystem&&) noexcept = default;
    FileSystem& operator=(FileSystem&&) noexcept = default;

    [[nodiscard]] std::optional<std::string> OpenFile(const std::string& file, std::fstream::ios_base::openmode mode = std::fstream::ios_base::in);
    std::optional<std::string> GetFullPathToFile(std::string_view fileName) const;
    std::optional<std::string> GetFullPathToFolder(std::string_view folderName) const;

    [[nodiscard]] bool FUCreateFile(const std::string& fileName, std::string_view folder) const;
    void WriteToFile(std::string_view fileName, const char* buffer);

    std::vector<std::string> GetAllFilesInFolder(std::string_view fullPathToFolder, const char* extensions = ".*");
    std::string GetFileNameWithoutExtFromPath(std::string_view path);

    friend class Application;
    FileSystem();
    ~FileSystem() = default;
    FLEUR_INTERFACE(FileSystem);

private:
    void OnInit();
    void OnShutdown();
};
}  // namespace Fleur::FS
