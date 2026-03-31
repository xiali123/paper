#include <windows.h>
#include <iostream>
#include <vector>

int main() {
    const char* dllPath = "./modules/dynamic/libExportApiModule.dll";

    std::cout << "=== DLL Export Test ===" << std::endl;
    std::cout << "DLL: " << dllPath << std::endl;

    HMODULE hModule = LoadLibraryA(dllPath);
    if (!hModule) {
        std::cerr << "Failed to load DLL!" << std::endl;
        return 1;
    }

    std::cout << "DLL loaded at: 0x" << hModule << std::endl;

    // 测试所有可能的符号名称
    std::vector<std::string> testSymbols = {
        "ExportApiModule_createModule",
        "ExportApiModule_destroyModule",
        "ExportApiModule_getModuleVersion",
        "createModule",
        "destroyModule",
        "getModuleVersion",
        "_ExportApiModule_createModule",
        "_ExportApiModule_createModule@0",
        "_createModule",
        "_createModule@0",
        "createModule@0",
    };

    int foundCount = 0;
    std::cout << "\nTesting symbols:" << std::endl;

    for (const auto& symbol : testSymbols) {
        void* addr = GetProcAddress(hModule, symbol.c_str());
        if (addr) {
            std::cout << "  FOUND: \"" << symbol << "\" at 0x" << addr << std::endl;
            foundCount++;
        }
    }

    std::cout << "\nTotal found: " << foundCount << " symbols" << std::endl;

    if (foundCount == 0) {
        std::cout << "\n*** NO SYMBOLS EXPORTED ***" << std::endl;
        std::cout << "The DLL compiles but exports no symbols!" << std::endl;
    }

    FreeLibrary(hModule);
    return foundCount > 0 ? 0 : 1;
}
