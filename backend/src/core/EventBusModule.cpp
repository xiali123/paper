#include "$(echo $file | sed 's|src/|include/|' | sed 's|\.cpp|\.hpp|')"
#include <iostream>

namespace PaperCrawler {

// 基础实现
class $(basename $(echo $file | sed 's|Module.cpp||')))::Impl {
public:
    // TODO: 实现细节
};

$(basename $(echo $file | sed 's|Module.cpp||'))::$(basename $(echo $file | sed 's|Module.cpp||'))()
    : impl_(std::make_unique<Impl>()) {}

$(basename $(echo $file | sed 's|Module.cpp||'))::~$(basename $(echo $file | sed 's|Module.cpp||'))() = default;

bool $(basename $(echo $file | sed 's|Module.cpp||'))::initialize() {
    std::cout << "$(basename $(echo $file | sed 's|Module.cpp||'))::initialize" << std::endl;
    return true;
}

bool $(basename $(echo $file | sed 's|Module.cpp||'))::start() {
    std::cout << "$(basename $(echo $file | sed 's|Module.cpp||')) started" << std::endl;
    return true;
}

bool $(basename $(echo $file | sed 's|Module.cpp||'))::stop() {
    std::cout << "$(basename $(echo $file | sed 's|Module.cpp||')) stopped" << std::endl;
    return true;
}

void $(basename $(echo $file | sed 's|Module.cpp||'))::cleanup() {
    // 清理资源
}

} // namespace PaperCrawler
