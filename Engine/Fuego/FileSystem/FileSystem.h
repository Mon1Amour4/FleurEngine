#pragma once

#include <filesystem>
#include <fstream>

namespace Fuego::Renderer
{
class Model;
}

namespace Fuego::FS
{

class Application;

class FUEGO_API FileSystem
{
public:
    FUEGO_NON_COPYABLE_NON_MOVABLE(FileSystem)

    std::string OpenFile(const std::string& file, std::fstream::ios_base::openmode mode = std::fstream::ios_base::in);
    bool Load_Image(IN const std::string& file, IN int& bits_per_pixel, OUT unsigned char*& data, OUT int& x, OUT int& y, int image_channels = 0);
    const std::string GetFullPathToFile(std::string_view file_name) const;
    std::string GetFullPathToFolder(std::string_view folder_name) const;

    void FUCreateFile(const std::string& file_name, std::string_view folder) const;
    void WriteToFile(std::string_view file_name, const char* buffer);


    friend class Application;
    FileSystem();
    ~FileSystem() = default;
    FUEGO_INTERFACE(FileSystem);
};
}  // namespace Fuego::FS
