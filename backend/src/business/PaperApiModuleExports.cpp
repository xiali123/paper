#include "business/PaperApiModule.hpp"
#include "core/ModuleExports.hpp"

using namespace PaperCrawler;

/**
 * @brief 导出模块版本信息
 */
extern "C" PAPERCRAWLER_MODULE_EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

/**
 * @brief 导出模块创建函数
 *
 * PluginManager调用此函数创建模块实例
 */
extern "C" PAPERCRAWLER_MODULE_EXPORT void* createModule() {
    return static_cast<void*>(new PaperApiModule());
}

/**
 * @brief 导出模块销毁函数
 *
 * PluginManager调用此函数销毁模块实例
 */
extern "C" PAPERCRAWLER_MODULE_EXPORT void destroyModule(void* modulePtr) {
    delete static_cast<PaperApiModule*>(modulePtr);
}
