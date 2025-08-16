#pragma once

#if defined(FLEUR_PLATFORM_WIN)
#define EXPORT __declspec(dllexport)
#define FLEUR_IMPORT __declspec(dllimport)
#elif defined(FLEUR_PLATFORM_MACOS)
#define EXPORT __attribute__((visibility("default")))
#define FLEUR_IMPORT __attribute__((visibility("default")))
#else
#error Unsupported platform!
#endif

// clang-format off
#if defined(FLEUR_DLL_LIB)
    #if defined(FLEUR_BUILD_LIB)
        #define FLEUR_API EXPORT
    #else
        #define FLEUR_API FLEUR_IMPORT
    #endif
#else
    #define FLEUR_API
#endif
// clang-format on

#define FLEUR_NON_COPYABLE_NON_MOVABLE(ClassName)    \
    ClassName(const ClassName&) = delete;            \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete;                 \
    ClassName& operator=(ClassName&&) = delete;

#define FLEUR_NON_COPYABLE(ClassName)     \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;

// TODO: Rewrite to use a custom unique pointer
#define FLEUR_INTERFACE(ClassName) \
private:                           \
    class ClassName##Impl;         \
    ClassName##Impl* d;

#define BIT(x) (1 << x)

#define UNUSED(x) (void)(x)
