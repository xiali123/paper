/**
 * @file test_minimal_dll.cpp
 * @brief 最小的DLL测试 - 验证符号导出是否工作
 */

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT int add(int a, int b) {
    return a + b;
}

EXPORT const char* getVersion() {
    return "1.0.0";
}

EXPORT void* createTest() {
    return (void*)0x12345678;
}

}
