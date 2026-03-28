#pragma once

#include <string>
#include <memory>

// Windows platform forward declarations (avoid windows.h macro pollution)
#ifdef _WIN32
    // Forward declare Windows types
    struct HMODULE__;
    struct HINSTANCE__;

    // Use incomplete type aliases
    using ModuleHandle = HMODULE__*;
#else
    // Linux/macOS use void*
    using ModuleHandle = void*;
#endif

namespace PaperCrawler {

/**
 * @brief Module type enumeration
 */
enum class ModuleType {
    SERVER,     // Server module (infrastructure)
    BUSINESS    // Business module (API handling)
};

/**
 * @brief Module lifecycle state
 */
enum class ModuleState {
    UNLOADED,   // Not loaded
    LOADED,     // Loaded
    STARTED,    // Started
    STOPPED,    // Stopped
    ERROR       // Error state
};

/**
 * @brief Module export function types
 */

// Get module version
using GetModuleVersionFunc = const char* (*)();

// Create module instance
using CreateModuleFunc = void* (*)();

// Destroy module instance
using DestroyModuleFunc = void (*)(void*);

} // namespace PaperCrawler

// Export macro definitions
#ifdef _WIN32
    #define PAPERCRAWLER_MODULE_EXPORT __declspec(dllexport)
#else
    #define PAPERCRAWLER_MODULE_EXPORT __attribute__((visibility("default")))
#endif
