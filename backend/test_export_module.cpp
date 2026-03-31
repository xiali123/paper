/**
 * @file test_export_module.cpp
 * @brief 模拟ExportApiModule的导出，但不包含实际类
 */

#include <iostream>

// 导出函数，但不实际创建ExportApiModule实例
#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    std::cout << "createModule called (test version)" << std::endl;
    return (void*)0x12345678;
}

EXPORT void destroyModule(void* ptr) {
    std::cout << "destroyModule called (test version)" << std::endl;
}

EXPORT const char* getModuleVersion() {
    return "1.0.0-test";
}

}
