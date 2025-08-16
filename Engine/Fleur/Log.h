#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"

namespace Fleur
{
class FLEUR_API Log
{
private:
    class LogImpl;

public:
    static void Init();
    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    static std::shared_ptr<spdlog::logger>& GetClientLogger();
};
}  // namespace Fleur

// clang-format off
#if defined(FLEUR_BUILD_LIB)
#define FL_CORE_TRACE(...) ::Fleur::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define FL_CORE_INFO(...) ::Fleur::Log::GetCoreLogger()->info(__VA_ARGS__)
#define FL_CORE_WARN(...) ::Fleur::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define FL_CORE_ERROR(...) ::Fleur::Log::GetCoreLogger()->error(__VA_ARGS__)
#define FL_CORE_CRITICAL(...) ::Fleur::Log::GetCoreLogger()->critical(__VA_ARGS__)
#endif

#if !defined(FLEUR_BUILD_LIB)
#define FL_TRACE(...) ::Fleur::Log::GetClientLogger()->trace(__VA_ARGS__)
#define FL_INFO(...) ::Fleur::Log::GetClientLogger()->info(__VA_ARGS__)
#define FL_WARN(...) ::Fleur::Log::GetClientLogger()->warn(__VA_ARGS__)
#define FL_ERROR(...) ::Fleur::Log::GetClientLogger()->error(__VA_ARGS__)
#define FL_CRITICAL(...) ::Fleur::Log::GetClientLogger()->critical(__VA_ARGS__)
#endif
// clang-format on

#if defined(FLEUR_ENABLE_ASSERTS)
#if defined(FLEUR_PLATFORM_WIN)
#define FL_ASSERT(x, ...)                                  \
    {                                                      \
        if (!(x))                                          \
        {                                                  \
            FL_ERROR("Assertion Failed {0}", __VA_ARGS__); \
            __debugbreak();                                \
        }                                                  \
    }
#define FL_CORE_ASSERT(x, ...)                                  \
    {                                                           \
        if (!(x))                                               \
        {                                                       \
            FL_CORE_ERROR("Assertion Failed {0}", __VA_ARGS__); \
            __debugbreak();                                     \
        }                                                       \
    }
#elif defined(FLEUR_PLATFORM_MACOS)
#define FL_ASSERT(x, ...)                                  \
    {                                                      \
        if (!(x))                                          \
        {                                                  \
            FL_ERROR("Assertion Failed {0}", __VA_ARGS__); \
            __builtin_debugtrap();                         \
        }                                                  \
    }
#define FL_CORE_ASSERT(x, ...)                                  \
    {                                                           \
        if (!(x))                                               \
        {                                                       \
            FL_CORE_ERROR("Assertion Failed {0}", __VA_ARGS__); \
            __builtin_debugtrap();                              \
        }                                                       \
    }
#endif
#else
#define FL_ASSERT(x, ...)
#define FL_CORE_ASSERT(x, ...)
#endif
