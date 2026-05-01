#pragma once

#include <string>
#include <map>
#include <vector>
#include <regex>
#include <spdlog/spdlog.h>
#include "core/HttpTypes.hpp"

namespace PaperCrawler {

struct ApiVersion {
    std::string version;       // e.g., "v1", "v2"
    bool isActive = true;
    bool isDeprecated = false;
    std::string sunsetDate;
    std::string deprecationNotice;

    static ApiVersion create(const std::string& v) {
        return {v, true, false, "", ""};
    }

    static ApiVersion createDeprecated(const std::string& v, const std::string& sunset) {
        return {v, true, true, sunset, "This API version will be removed on " + sunset};
    }
};

class ApiVersionManager {
public:
    ApiVersionManager();
    ~ApiVersionManager() = default;

    void registerVersion(const ApiVersion& version);
    void setDefaultVersion(const std::string& version);
    std::string extractVersion(const std::string& path) const;
    bool isVersionValid(const std::string& version) const;
    bool isDeprecated(const std::string& version) const;
    std::string getDeprecationNotice(const std::string& version) const;
    std::string stripVersion(const std::string& path) const;
    void addVersionRoute(const std::string& version,
                          const std::string& versionedPath,
                          const std::string& internalPath);
    std::vector<ApiVersion> getVersions() const;
    std::string getDefaultVersion() const { return defaultVersion_; }
    void addVersionHeaders(HttpResponse& response,
                            const std::string& requestedVersion) const;

private:
    std::map<std::string, ApiVersion> versions_;
    std::string defaultVersion_{"v1"};
    std::map<std::string, std::string> routeAliases_;
    static const std::regex versionPattern_;
};

} // namespace PaperCrawler
