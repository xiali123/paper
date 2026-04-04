#pragma once

#include <string>
#include <memory>

// Platform-specific module handle types
// Use void* for all platforms to avoid header pollution
// Type conversion will be handled in .cpp files
#ifdef _WIN32
    using ModuleHandle = void*;
#else
    // Linux/macOS use void*
    using ModuleHandle = void*;
#endif

namespace PaperCrawler {

/**
 * @brief Module type enumeration
 */
enum class ModuleType {
    SERVER,     // Infrastructure module
    BUSINESS    // Business API module
};

/**
 * @brief Module lifecycle state
 */
enum class ModuleState {
    UNLOADED,    // Module not loaded
    LOADED,      // Module loaded
    INITIALIZED, // Module initialized
    STARTED,     // Module started
    STOPPED,     // Module stopped
    FAILED       // Error state
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
    #define BUSINESS_API __declspec(dllexport)
    #define PAPERCRAWLER_API __declspec(dllexport)
#else
    #define PAPERCRAWLER_MODULE_EXPORT __attribute__((visibility("default")))
    #define BUSINESS_API __attribute__((visibility("default")))
    #define PAPERCRAWLER_API __attribute__((visibility("default")))
#endif
