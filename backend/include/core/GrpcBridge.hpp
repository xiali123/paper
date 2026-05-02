#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <spdlog/spdlog.h>
#include "core/HttpTypes.hpp"

namespace PaperCrawler {

// gRPC服务方法描述
struct GrpcMethod {
    std::string service;    // e.g., "SearchService"
    std::string method;     // e.g., "Search"
    std::string inputType;  // e.g., "SearchRequest"
    std::string outputType; // e.g., "SearchResponse"
    bool serverStreaming = false;
    bool clientStreaming = false;
};

// gRPC-REST桥接器
// 将proto定义的gRPC服务映射到REST-over-HTTP端点
class GrpcBridge {
public:
    using Handler = std::function<std::string(const std::string& jsonRequest)>;

    static GrpcBridge& instance() {
        static GrpcBridge inst;
        return inst;
    }

    void registerService(const std::string& serviceName) {
        spdlog::info("[GrpcBridge] Registering service: {}", serviceName);
    }

    void registerMethod(const std::string& serviceName, const std::string& methodName,
                        Handler handler, bool serverStreaming = false) {
        std::string key = serviceName + "/" + methodName;
        handlers_[key] = std::move(handler);
        GrpcMethod m;
        m.service = serviceName;
        m.method = methodName;
        m.serverStreaming = serverStreaming;
        methods_[key] = m;
        spdlog::info("[GrpcBridge] Registered method: {}/{}", serviceName, methodName);
    }

    std::string call(const std::string& serviceName, const std::string& methodName,
                     const std::string& jsonRequest) {
        std::string key = serviceName + "/" + methodName;
        auto it = handlers_.find(key);
        if (it == handlers_.end()) {
            return R"({"error":"Method not found: )" + key + R"("})";
        }
        return it->second(jsonRequest);
    }

    std::vector<GrpcMethod> listMethods() const {
        std::vector<GrpcMethod> result;
        for (const auto& [_, m] : methods_) {
            result.push_back(m);
        }
        return result;
    }

    bool hasMethod(const std::string& serviceName, const std::string& methodName) const {
        return methods_.count(serviceName + "/" + methodName) > 0;
    }

private:
    GrpcBridge() = default;
    std::map<std::string, Handler> handlers_;
    std::map<std::string, GrpcMethod> methods_;
};

} // namespace PaperCrawler
