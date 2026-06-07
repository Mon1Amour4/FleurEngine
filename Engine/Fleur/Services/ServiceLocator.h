#pragma once
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>

#include "AssetsManager.h"
#include "FileSystem/FileSystem.h"
#include "ServiceInterfaces.hpp"
#include "ThreadPool.h"
#include "singleton.hpp"

namespace Fleur
{

// Services are stored type-erased as shared_ptr<void> keyed by type_index, so a new
// service type needs no central list edit. GetService<T>/Register<T> cast back at the
// call site (where T is complete). (Headers above are kept only for transitive
// convenience of existing includers; ServiceLocator itself no longer needs them.)
class ServiceLocator : public singleton<ServiceLocator>
{
    friend class singleton<ServiceLocator>;

public:
    template <typename T, typename... Args>
    std::optional<std::shared_ptr<T>> Register(Args&&... args)
    {
        static_assert(std::is_constructible_v<T, Args...>, "Type T is not constructible with provided arguments.");
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        auto [it, inserted] = services.try_emplace(std::type_index(typeid(T)), ptr);
        if (inserted)
            return ptr;

        return std::static_pointer_cast<T>(it->second);
    };

    template <class T>
    std::shared_ptr<T> GetService()
    {
        auto it = services.find(std::type_index(typeid(T)));
        if (it == services.end())
            return nullptr;

        return std::static_pointer_cast<T>(it->second);
    }

private:
    ServiceLocator() = default;
    std::unordered_map<std::type_index, std::shared_ptr<void>> services;
};
}  // namespace Fleur
