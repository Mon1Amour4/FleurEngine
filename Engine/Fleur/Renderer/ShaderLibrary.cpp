#include "ShaderLibrary.h"

#include <array>

#include "FileSystem/FileSystem.h"
#include "Services/ServiceLocator.h"

namespace Fleur::Graphics
{

Fleur::Asset<Fleur::Graphics::Shader> ShaderLibrary::GetUnlocked(const Fleur::AssetHandle& handle)
{
    if (!handle.IsValid())
        return {{}, nullptr};

    if (auto gen = m_GenerationByID.find(handle.id); gen != m_GenerationByID.end() && gen->second == handle.generation)
    {
        if (auto rec = m_Records.find(handle.id); rec != m_Records.end())
            return {handle, &rec->second.shader};
    }

    return {{}, nullptr};
}

Fleur::Asset<Fleur::Graphics::ShaderProgram> ShaderLibrary::GetProgramUnlocked(const Fleur::AssetHandle& handle)
{
    if (!handle.IsValid())
        return {{}, nullptr};

    if (auto gen = m_ProgramGenerationByID.find(handle.id); gen != m_ProgramGenerationByID.end() && gen->second == handle.generation)
    {
        if (auto rec = m_Programs.find(handle.id); rec != m_Programs.end())
            return {handle, &rec->second.program};
    }

    return {{}, nullptr};
}

Fleur::Asset<Fleur::Graphics::Shader> ShaderLibrary::Load(std::string_view path)
{
    if (path.empty())
        return {{}, nullptr};

    auto fs = Fleur::ServiceLocator::instance().GetService<Fleur::FS::FileSystem>();
    if (!fs)
        return {{}, nullptr};

    const std::string name = fs->GetFileNameWithoutExtFromPath(path);
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (auto byName = m_NameToHandle.find(name); byName != m_NameToHandle.end())
            return GetUnlocked(byName->second);
    }

    std::vector<char> code = fs->ReadFileBinary(path);
    if (code.empty())
        return {{}, nullptr};

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (auto byName = m_NameToHandle.find(name); byName != m_NameToHandle.end())
        return GetUnlocked(byName->second);

    const Fleur::AssetID id = m_NextID.fetch_add(1, std::memory_order_acq_rel);
    const Fleur::AssetHandle handle{id, 1};

    ShaderRecord record{};
    record.handle = handle;
    record.name = name;
    record.shader = Fleur::Graphics::Shader(code.data(), code.size());

    auto inserted = m_Records.emplace(id, std::move(record)).first;
    m_NameToHandle.emplace(name, handle);
    m_GenerationByID.emplace(id, 1);

    return {handle, &inserted->second.shader};
}

Fleur::Asset<Fleur::Graphics::Shader> ShaderLibrary::Get(std::string_view name)
{
    if (name.empty())
        return {{}, nullptr};

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (auto byName = m_NameToHandle.find(std::string(name)); byName != m_NameToHandle.end())
        return GetUnlocked(byName->second);

    return {{}, nullptr};
}

Fleur::Asset<Fleur::Graphics::Shader> ShaderLibrary::Get(const Fleur::AssetHandle& handle)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetUnlocked(handle);
}

Fleur::Asset<Fleur::Graphics::ShaderProgram> ShaderLibrary::CreateProgram(std::string_view name, const Fleur::AssetHandle& vertexShader,
                                                                           const Fleur::AssetHandle& fragmentShader)
{
    if (name.empty() || !vertexShader.IsValid() || !fragmentShader.IsValid())
        return {{}, nullptr};

    std::lock_guard<std::mutex> lock(m_Mutex);
    const std::string key{name};
    if (auto byName = m_ProgramNameToHandle.find(key); byName != m_ProgramNameToHandle.end())
        return GetProgramUnlocked(byName->second);

    if (!GetUnlocked(vertexShader).obj || !GetUnlocked(fragmentShader).obj)
        return {{}, nullptr};

    const Fleur::AssetID id = m_NextProgramID.fetch_add(1, std::memory_order_acq_rel);
    const Fleur::AssetHandle handle{id, 1};

    ShaderProgramRecord record{};
    record.handle = handle;
    record.name = key;
    record.program.vertexShader = vertexShader;
    record.program.fragmentShader = fragmentShader;

    auto inserted = m_Programs.emplace(id, std::move(record)).first;
    m_ProgramNameToHandle.emplace(key, handle);
    m_ProgramGenerationByID.emplace(id, 1);
    return {handle, &inserted->second.program};
}

Fleur::Asset<Fleur::Graphics::ShaderProgram> ShaderLibrary::GetProgram(std::string_view name)
{
    if (name.empty())
        return {{}, nullptr};

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (auto byName = m_ProgramNameToHandle.find(std::string(name)); byName != m_ProgramNameToHandle.end())
        return GetProgramUnlocked(byName->second);
    return {{}, nullptr};
}

Fleur::Asset<Fleur::Graphics::ShaderProgram> ShaderLibrary::GetProgram(const Fleur::AssetHandle& handle)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetProgramUnlocked(handle);
}

void ShaderLibrary::LoadDefaults()
{
    static constexpr std::array<std::string_view, 4> kDefaultShaders = {"vertex.spv", "opaque.spv", "skyboxVertex.spv", "skyboxFragment.spv"};
    for (std::string_view shader : kDefaultShaders)
    {
        Load(shader);
    }
}

void ShaderLibrary::Clear()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_NameToHandle.clear();
    m_GenerationByID.clear();
    m_Records.clear();
    m_ProgramNameToHandle.clear();
    m_ProgramGenerationByID.clear();
    m_Programs.clear();
}

}  // namespace Fleur::Graphics
