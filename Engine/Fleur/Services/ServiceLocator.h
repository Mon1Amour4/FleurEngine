#pragma once
#include <typeindex>
#include <unordered_map>

#include "AssetsManager.h"
#include "FileSystem/FileSystem.h"
#include "Renderer.h"
#include "ServiceInterfaces.hpp"
#include "ThreadPool.h"
#include "singleton.hpp"

namespace Fleur
{

#pragma region Templates
using service_variant = std::variant<std::shared_ptr<Fleur::Graphics::Renderer>, std::shared_ptr<Fleur::FS::FileSystem>, std::shared_ptr<Fleur::ThreadPool>,
                                     std::shared_ptr<Fleur::AssetsManager>>;

#pragma endregion

class ServiceLocator : public singleton<ServiceLocator>
{
    friend class singleton<ServiceLocator>;

public:
    template <typename T, typename... Args>
    std::optional<std::shared_ptr<T>> Register(Args&&... args)
    {
        static_assert(std::is_constructible_v<T, Args...>, "Type T is not constructible with provided arguments.");
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        service_variant variant = ptr;
        auto [it, inserted] = services.try_emplace(std::type_index(typeid(T)), std::move(variant));
        if (inserted)
            return ptr;

        if (auto pval = std::get_if<std::shared_ptr<T>>(&it->second))
            return *pval;

        return std::nullopt;
    };

    template <class T>
    std::shared_ptr<T> GetService()
    {
        auto it = services.find(std::type_index(typeid(T)));
        if (it == services.end())
            return nullptr;

        if (auto ptr = std::get_if<std::shared_ptr<T>>(&it->second))
            return *ptr;

        return nullptr;
    }

private:
    ServiceLocator() = default;
    std::unordered_map<std::type_index, service_variant> services;
};
}  // namespace Fleur
