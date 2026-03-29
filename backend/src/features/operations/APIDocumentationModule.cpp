#include "features/operations/APIDocumentationModule.hpp"
#include <iostream>
#include <sstream>
#include <mutex>

namespace PaperCrawler {

class APIDocumentationModule::Impl {
public:
    std::map<std::string, APIEndpoint> endpoints_;
    mutable std::mutex mutex_;
};

APIDocumentationModule::APIDocumentationModule()
    : impl_(std::make_unique<Impl>()) {}

APIDocumentationModule::~APIDocumentationModule() = default;

bool APIDocumentationModule::initialize() {
    std::cout << "APIDocumentationModule::initialize" << std::endl;
    return true;
}

bool APIDocumentationModule::start() {
    std::cout << "APIDocumentationModule started" << std::endl;
    return true;
}

bool APIDocumentationModule::stop() {
    std::cout << "APIDocumentationModule stopped" << std::endl;
    return true;
}

void APIDocumentationModule::cleanup() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->endpoints_.clear();
}

void APIDocumentationModule::registerEndpoint(const APIEndpoint& endpoint) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::string key = std::to_string(static_cast<int>(endpoint.method)) + ":" + endpoint.path;
    impl_->endpoints_[key] = endpoint;
}

std::string APIDocumentationModule::generateOpenAPIJSON() {
    std::ostringstream json;
    json << "{\n";
    json << "  \"openapi\": \"3.0.0\",\n";
    json << "  \"info\": {\n";
    json << "    \"title\": \"PaperCrawler API\",\n";
    json << "    \"version\": \"1.0.0\"\n";
    json << "  },\n";
    json << "  \"paths\": {\n";

    bool first = true;
    for (const auto& pair : impl_->endpoints_) {
        if (!first) json << ",\n";
        first = false;

        const auto& info = pair.second;
        json << "    \"" << info.path << "\": {\n";
        json << "      \"" << static_cast<int>(info.method) << "\": {\n";
        json << "        \"summary\": \"" << info.description << "\",\n";
        json << "        \"responses\": {\n";
        json << "          \"200\": {\n";
        json << "            \"description\": \"Success\"\n";
        json << "          }\n";
        json << "        }\n";
        json << "      }\n";
        json << "    }\n";
    }

    json << "  }\n";
    json << "}";
    return json.str();
}

std::string APIDocumentationModule::getSwaggerUI() {
    // TODO: 返回Swagger UI HTML
    return "<html>Swagger UI Placeholder</html>";
}

std::string APIDocumentationModule::generateOpenAPIYAML() {
    std::ostringstream yaml;
    yaml << "openapi: 3.0.0\n";
    yaml << "info:\n";
    yaml << "  title: PaperCrawler API\n";
    yaml << "  version: 1.0.0\n";
    yaml << "paths:\n";

    for (const auto& pair : impl_->endpoints_) {
        const auto& info = pair.second;
        yaml << "  " << info.path << ":\n";
        yaml << "    " << static_cast<int>(info.method) << ":\n";
        yaml << "      summary: " << info.description << "\n";
        yaml << "      responses:\n";
        yaml << "        '200':\n";
        yaml << "          description: Success\n";
    }

    return yaml.str();
}


} // namespace PaperCrawler
