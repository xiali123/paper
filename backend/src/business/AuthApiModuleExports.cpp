#include "business/AuthApiModule.hpp"
#include "core/ModuleExports.hpp"

using namespace PaperCrawler;

extern "C" PAPERCRAWLER_MODULE_EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

extern "C" PAPERCRAWLER_MODULE_EXPORT void* createModule() {
    return static_cast<void*>(new AuthApiModule());
}

extern "C" PAPERCRAWLER_MODULE_EXPORT void destroyModule(void* modulePtr) {
    delete static_cast<AuthApiModule*>(modulePtr);
}
