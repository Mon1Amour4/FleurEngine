#pragma once

#include <type_traits>

namespace Fleur::Concepts
{

template <typename T>
concept IsDefaultConstructible = std::is_default_constructible_v<T>;
}
