#include "features/ConfigModule.hpp"
#include <iostream>

namespace PaperCrawler {

// 基础实现
class ConfigModule::Impl {
public:
    // TODO: 实现细节
};

ConfigModule::ConfigModule()
    : impl_(std::make_unique<Impl>()) {}

ConfigModule::~ConfigModule() = default;

bool ConfigModule::initialize() {
    std::cout << "ConfigModule::initialize" << std::endl;
    return true;
}

bool ConfigModule::start() {
    std::cout << "ConfigModule started" << std::endl;
    return true;
}

bool ConfigModule::stop() {
    std::cout << "ConfigModule stopped" << std::endl;
    return true;
}

void ConfigModule::cleanup() {
    // 清理资源
}

} // namespace PaperCrawler
