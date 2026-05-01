#include "core/ApiVersionManager.hpp"
#include "core/HttpTypes.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

const std::regex ApiVersionManager::versionPattern_(R"(/api/(v\d+)/)");

ApiVersionManager::ApiVersionManager() {
    // 默认注册v1
    registerVersion(ApiVersion::create("v1"));
}

void ApiVersionManager::registerVersion(const ApiVersion& version) {
    versions_[version.version] = version;
    spdlog::info("[ApiVersion] Registered version: {} (active={}, deprecated={})",
                 version.version, version.isActive, version.isDeprecated);
}

void ApiVersionManager::setDefaultVersion(const std::string& version) {
    if (versions_.count(version)) {
        defaultVersion_ = version;
        spdlog::info("[ApiVersion] Default version set to: {}", version);
    }
}

std::string ApiVersionManager::extractVersion(const std::string& path) const {
    std::smatch match;
    if (std::regex_search(path, match, versionPattern_) && match.size() > 1) {
        return match[1].str();
    }
    return defaultVersion_;
}

bool ApiVersionManager::isVersionValid(const std::string& version) const {
    auto it = versions_.find(version);
    return it != versions_.end() && it->second.isActive;
}

bool ApiVersionManager::isDeprecated(const std::string& version) const {
    auto it = versions_.find(version);
    return it != versions_.end() && it->second.isDeprecated;
}

std::string ApiVersionManager::getDeprecationNotice(const std::string& version) const {
    auto it = versions_.find(version);
    if (it != versions_.end() && it->second.isDeprecated) {
        return it->second.deprecationNotice;
    }
    return "";
}

std::string ApiVersionManager::stripVersion(const std::string& path) const {
    // /api/v1/users → /api/users
    std::smatch match;
    if (std::regex_search(path, match, versionPattern_)) {
        std::string result = path;
        std::string versionPart = "/api/" + match[1].str() + "/";
        size_t pos = result.find(versionPart);
        if (pos != std::string::npos) {
            result.replace(pos, versionPart.length(), "/api/");
        }
        return result;
    }
    return path;
}

void ApiVersionManager::addVersionRoute(const std::string& version,
                                          const std::string& versionedPath,
                                          const std::string& internalPath) {
    routeAliases_[version + ":" + versionedPath] = internalPath;
}

std::vector<ApiVersion> ApiVersionManager::getVersions() const {
    std::vector<ApiVersion> result;
    for (const auto& [_, v] : versions_) {
        result.push_back(v);
    }
    return result;
}

void ApiVersionManager::addVersionHeaders(HttpResponse& response,
                                            const std::string& requestedVersion) const {
    response.headers["X-API-Version"] = requestedVersion;

    if (requestedVersion != defaultVersion_) {
        response.headers["X-API-Current-Version"] = defaultVersion_;
    }

    if (isDeprecated(requestedVersion)) {
        response.headers["Deprecation"] = "true";
        response.headers["Sunset"] = getDeprecationNotice(requestedVersion);
        std::string notice = getDeprecationNotice(requestedVersion);
        if (!notice.empty()) {
            response.headers["Link"] = "</api/" + defaultVersion_ + "/>; rel=\"successor-version\"";
        }
    }
}

} // namespace PaperCrawler
