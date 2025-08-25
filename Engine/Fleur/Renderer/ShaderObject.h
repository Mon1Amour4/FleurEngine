#pragma once

#include <glm/glm.hpp>

template <typename T, typename U>
constexpr bool GetPureTypeAndCompare()
{
    return std::is_same_v<T, std::remove_cv_t<std::remove_reference_t<U>>>;
}

namespace Fleur::Graphics
{
class Shader;
class Material;
class Texture;

class ShaderObject
{
public:
    virtual ~ShaderObject() = default;

    template <typename T>
    bool Set(std::string_view name, const T& var)
    {
        if constexpr (GetPureTypeAndCompare<glm::vec3, T>())
        {
            return SetVec3fImpl(name, var);
        }
        else if constexpr (GetPureTypeAndCompare<glm::mat4, T>())
        {
            return SetMat4fImpl(name, var);
        }
        else if constexpr (GetPureTypeAndCompare<Texture, T>())
        {
            return SetText2dImpl(name, var);
        }
        // TODO
        else if constexpr (std::is_same_v<Texture, std::remove_cv_t<std::remove_pointer_t<T>>>)
        {
            return SetText2dImpl(name, var);
        }
        else
        {
            const char* typeName = typeid(T).name();
            FL_CORE_ASSERT(false, "t: {0}", typeid(T).name());
            return false;
        }
    }

    static ShaderObject* CreateShaderObject(std::string_view name, Shader* vs, Shader* px);

    virtual void Use() const = 0;

    virtual void BindMaterial(const Material* material) = 0;

    virtual void Release() = 0;

protected:
    ShaderObject(std::string_view name)
        : name(name)
    {
    }
    std::string name;
    virtual bool SetVec3fImpl(std::string_view name, const glm::vec3& vec) = 0;
    virtual bool SetMat4fImpl(std::string_view name, const glm::mat4& matrix) = 0;
    virtual bool SetText2dImpl(std::string_view name, const Texture& texture) = 0;
};
}  // namespace Fleur::Graphics
