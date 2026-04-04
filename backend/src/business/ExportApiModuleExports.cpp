/**
 * @file ExportApiModuleExports.cpp
 * @brief ExportApiModule导出函数
 * 使用与test_minimal完全相同的模式
 */

#include "business/ExportApiModule.hpp"

// 与test_minimal完全一致的导出方式
#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::ExportApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::ExportApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}
