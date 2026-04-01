/**
 * @file main_minimal.cpp
 * @brief 最小化主程序用于调试崩溃问题
 */

#include <iostream>
#include <memory>

// 最小化包含
#include "data/DatabaseModule.hpp"

using namespace PaperCrawler;

int main() {
    printf("Minimal test starting...\n");
    fflush(stdout);

    try {
        printf("Creating DatabaseModule...\n");
        fflush(stdout);

        auto dbModule = std::make_shared<DatabaseModule>();

        printf("DatabaseModule created successfully\n");
        fflush(stdout);

        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception: %s\n", e.what());
        return 1;
    }
}
