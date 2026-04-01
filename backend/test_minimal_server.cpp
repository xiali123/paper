/**
 * @file test_minimal_server.cpp
 * @brief 最小服务器测试 - 用于调试启动崩溃问题
 */

#include <iostream>
#include <csignal>

// 捕获信号用于调试
void signalHandler(int signum) {
    std::cout << "Signal (" << signum << ") caught" << std::endl;
    exit(signum);
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);

    std::cout << "======================================" << std::endl;
    std::cout << "Minimal Server Test Starting..." << std::endl;
    std::cout << "======================================" << std::endl;

    try {
        std::cout << "Step 1: Basic output - OK" << std::endl;

        std::cout << "Step 2: Testing std::string..." << std::endl;
        std::string test = "Hello, World!";
        std::cout << "  String test: " << test << " - OK" << std::endl;

        std::cout << "Step 3: Testing std::vector..." << std::endl;
        std::vector<int> numbers = {1, 2, 3, 4, 5};
        std::cout << "  Vector size: " << numbers.size() << " - OK" << std::endl;

        std::cout << "Step 4: Testing std::map..." << std::endl;
        std::map<std::string, int> config;
        config["port"] = 8080;
        std::cout << "  Config port: " << config["port"] << " - OK" << std::endl;

        std::cout << "Step 5: Testing dynamic memory..." << std::endl;
        auto ptr = std::make_unique<int>(42);
        std::cout << "  Dynamic value: " << *ptr << " - OK" << std::endl;

        std::cout << "Step 6: Testing exceptions..." << std::endl;
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            std::cout << "  Caught exception: " << e.what() << " - OK" << std::endl;
        }

        std::cout << "======================================" << std::endl;
        std::cout << "All tests passed successfully!" << std::endl;
        std::cout << "Basic C++ runtime is working correctly." << std::endl;
        std::cout << "======================================" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
