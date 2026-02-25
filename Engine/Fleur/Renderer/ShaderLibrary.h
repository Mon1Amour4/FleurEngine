#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "AssetTypes.hpp"
#include "Shader.h"

namespace Fleur::Graphics
{

struct ShaderProgram
{
    Fleur::AssetHandle vertexShader{};
    Fleur::AssetHandle fragmentShader{};
};

class ShaderLibrary
{
public:
    /// Loads a shader binary by path, returns existing record for the same logical shader name.
    Fleur::Asset<Fleur::Graphics::Shader> Load(std::string_view path);
    /// Returns a shader by logical name.
    Fleur::Asset<Fleur::Graphics::Shader> Get(std::string_view name);
    /// Returns a shader by stable handle.
    Fleur::Asset<Fleur::Graphics::Shader> Get(const Fleur::AssetHandle& handle);
    /// Creates (or returns existing) shader program from two shader stage handles.
    Fleur::Asset<Fleur::Graphics::ShaderProgram> CreateProgram(std::string_view name, const Fleur::AssetHandle& vertexShader,
                                                               const Fleur::AssetHandle& fragmentShader);
    /// Returns shader program by logical name.
    Fleur::Asset<Fleur::Graphics::ShaderProgram> GetProgram(std::string_view name);
    /// Returns shader program by stable handle.
    Fleur::Asset<Fleur::Graphics::ShaderProgram> GetProgram(const Fleur::AssetHandle& handle);
    /// Loads built-in shaders required by renderer startup.
    void LoadDefaults();
    /// Clears shader records.
    void Clear();

private:
    struct ShaderRecord
    {
        Fleur::AssetHandle handle{};
        std::string name;
        Fleur::Graphics::Shader shader;
    };
    struct ShaderProgramRecord
    {
        Fleur::AssetHandle handle{};
        std::string name;
        Fleur::Graphics::ShaderProgram program;
    };

    Fleur::Asset<Fleur::Graphics::Shader> GetUnlocked(const Fleur::AssetHandle& handle);
    Fleur::Asset<Fleur::Graphics::ShaderProgram> GetProgramUnlocked(const Fleur::AssetHandle& handle);

    std::mutex m_Mutex;
    std::unordered_map<std::string, Fleur::AssetHandle> m_NameToHandle;
    std::unordered_map<Fleur::AssetID, uint32_t> m_GenerationByID;
    std::unordered_map<Fleur::AssetID, ShaderRecord> m_Records;
    std::atomic<uint32_t> m_NextID{1};
    std::unordered_map<std::string, Fleur::AssetHandle> m_ProgramNameToHandle;
    std::unordered_map<Fleur::AssetID, uint32_t> m_ProgramGenerationByID;
    std::unordered_map<Fleur::AssetID, ShaderProgramRecord> m_Programs;
    std::atomic<uint32_t> m_NextProgramID{1};
};

}  // namespace Fleur::Graphics
