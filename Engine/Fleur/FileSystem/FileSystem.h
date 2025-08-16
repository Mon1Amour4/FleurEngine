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

    std::optional<std::string> OpenFile(const std::string& file, std::fstream::ios_base::openmode mode = std::fstream::ios_base::in);
    std::optional<std::string> GetFullPathToFile(std::string_view file_name) const;
    std::optional<std::string> GetFullPathToFolder(std::string_view folder_name) const;

    bool FUCreateFile(const std::string& file_name, std::string_view folder) const;
    void WriteToFile(std::string_view file_name, const char* buffer);

    friend class Application;
    FileSystem();
    ~FileSystem() = default;
    FLEUR_INTERFACE(FileSystem);

private:
    void OnInit();
    void OnShutdown();
};
}  // namespace Fleur::FS
