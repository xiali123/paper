#include <windows.h>
#include <iostream>

int main() {
    const char* dllPath = "./Release/libtest_export_module.dll";

    std::cout << "Testing: " << dllPath << std::endl;

    HMODULE hModule = LoadLibraryA(dllPath);
    if (!hModule) {
        std::cerr << "Failed to load!" << std::endl;
        return 1;
    }

    const char* symbols[] = {"createModule", "destroyModule", "getModuleVersion"};
    int found = 0;

    for (auto symbol : symbols) {
        if (GetProcAddress(hModule, symbol)) {
            std::cout << "  FOUND: " << symbol << std::endl;
            found++;
        } else {
            std::cout << "  NOT FOUND: " << symbol << std::endl;
        }
    }

    FreeLibrary(hModule);
    return found == 3 ? 0 : 1;
}
