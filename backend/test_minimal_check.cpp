#include <windows.h>
#include <iostream>

int main() {
    const char* dllPath = "./Release/libtest_minimal.dll";

    std::cout << "Testing minimal DLL: " << dllPath << std::endl;

    HMODULE hModule = LoadLibraryA(dllPath);
    if (!hModule) {
        std::cerr << "Failed to load DLL!" << std::endl;
        return 1;
    }

    const char* symbols[] = {"add", "getVersion", "createTest"};

    int found = 0;
    for (auto symbol : symbols) {
        void* addr = GetProcAddress(hModule, symbol);
        if (addr) {
            std::cout << "  FOUND: " << symbol << " at 0x" << addr << std::endl;
            found++;
        } else {
            std::cout << "  NOT FOUND: " << symbol << std::endl;
        }
    }

    FreeLibrary(hModule);

    if (found == 3) {
        std::cout << "\n*** SUCCESS: All symbols exported! ***" << std::endl;
        return 0;
    } else {
        std::cout << "\n*** FAILURE: Only " << found << "/3 symbols found ***" << std::endl;
        return 1;
    }
}
