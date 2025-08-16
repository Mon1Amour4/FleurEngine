#pragma once

namespace Fleur
{
template <typename T>
class singleton
{
public:
    static T& instance();

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;
    singleton(singleton&&) = delete;
    singleton& operator=(singleton&&) = delete;

protected:
    singleton() = default;
    virtual ~singleton() = default;
};
}  // namespace Fleur
