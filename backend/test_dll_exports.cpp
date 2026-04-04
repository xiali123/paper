/**
 * @file test_dll_exports.cpp
 * @brief 测试DLL符号导出
 */

#include <windows.h>
#include <iostream>
#include <vector>

struct SymbolInfo {
    std::string name;
    void* address;
};

int main() {
    const char* dllPath = "./Release/libExportApiModule.dll";

    std::cout << "=== DLL Symbol Export Test ===" << std::endl;
    std::cout << "Loading DLL: " << dllPath << std::endl;

    HMODULE hModule = LoadLibraryA(dllPath);
    if (!hModule) {
        std::cerr << "Failed to load DLL!" << std::endl;
        std::cerr << "Error code: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "DLL loaded successfully!" << std::endl;
    std::cout << "Module handle: 0x" << hModule << std::endl;
    std::cout << std::endl;

    // 测试符号列表 - 使用统一命名规范
    std::vector<std::string> testSymbols = {
        // 新的统一命名（带模块名前缀）
        "ExportApiModule_createModule",
        "ExportApiModule_destroyModule",
        "ExportApiModule_getModuleVersion",
        // 向后兼容的短名称
        "createModule",
        "destroyModule",
        "getModuleVersion",
        // 可能的前缀/修饰
        "_ExportApiModule_createModule",
        "_ExportApiModule_createModule@0",
        "_createModule",
        "_createModule@0",
    };

    std::cout << "Testing " << testSymbols.size() << " possible symbol names:" << std::endl;
    std::cout << std::endl;

    std::vector<SymbolInfo> foundSymbols;

    for (const auto& symbol : testSymbols) {
        void* addr = GetProcAddress(hModule, symbol.c_str());
        if (addr) {
            std::cout << "  ✓ FOUND: \"" << symbol << "\" at 0x" << addr << std::endl;
            foundSymbols.push_back({symbol, addr});
        } else {
            std::cout << "  ✗ NOT FOUND: \"" << symbol << "\"" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "=== Summary ===" << std::endl;
    std::cout << "Found " << foundSymbols.size() << " out of " << testSymbols.size() << " symbols" << std::endl;

    if (foundSymbols.empty()) {
        std::cout << std::endl;
        std::cout << "ERROR: No expected symbols found!" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible issues:" << std::endl;
        std::cout << "1. Functions are not being exported from the DLL" << std::endl;
        std::cout << "2. Symbol names are decorated differently" << std::endl;
        std::cout << "3. Wrong calling convention" << std::endl;
        std::cout << "4. DLL was built with different compiler settings" << std::endl;

        // 尝试枚举所有导出的符号
        std::cout << std::endl;
        std::cout << "Note: To see all exported symbols, you can use:" << std::endl;
        std::cout << "  dumpbin /EXPORTS " << dllPath << std::endl;
        std::cout << "  or" << std::endl;
        std::cout << "  objdump -T " << dllPath << std::endl;
    }

    FreeLibrary(hModule);
    return foundSymbols.empty() ? 1 : 0;
}
