#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <map>
#include <vector>
#include <functional>

namespace PaperCrawler {

/**
 * @brief HTTP方法
 */
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    TRACE
};

/**
 * @brief 参数类型
 */
enum class ParameterType {
    QUERY,      // 查询参数
    PATH,       // 路径参数
    HEADER,     // 请求头
    BODY,       // 请求体
    FORM,       // 表单
    COOKIE      // Cookie
};

/**
 * @brief 数据类型
 */
enum class DataType {
    STRING,
    INTEGER,
    NUMBER,
    BOOLEAN,
    ARRAY,
    OBJECT,
    FILE,
    BINARY
};

/**
 * @brief API参数
 */
struct APIParameter {
    std::string name;
    ParameterType in;
    DataType type;
    bool required{false};
    std::string description;
    std::string defaultValue;  // 默认值
    std::vector<std::string> enumValues;  // 枚举值
    std::map<std::string, std::string> examples;  // 示例
};

/**
 * @brief API响应
 */
struct APIResponse {
    int statusCode{200};
    std::string description;
    std::string contentType{"application/json"};
    std::string example;  // 响应示例
    std::map<std::string, std::string> headers;  // 响应头
};

/**
 * @brief API端点信息
 */
struct APIEndpoint {
    std::string path;
    HTTPMethod method;
    std::string summary;
    std::string description;
    std::vector<std::string> tags;
    std::string operationId;

    // 请求参数
    std::vector<APIParameter> parameters;
    std::string requestBodySchema;  // 请求体Schema
    std::string requestBodyExample;  // 请求体示例

    // 响应
    std::map<int, APIResponse> responses;

    // 安全
    std::vector<std::string> security;  // 安全要求（如：BearerAuth）

    // 速率限制
    int rateLimit{0};  // 每分钟请求限制

    // 已废弃
    bool deprecated{false};

    // 元数据
    std::string moduleName;  // 所属模块
    std::string version;     // API版本
};

/**
 * @brief API标签
 */
struct APITag {
    std::string name;
    std::string description;
    std::string displayName;  // 显示名称
    std::vector<std::string> endpoints;  // 该标签下的端点
};

/**
 * @brief 安全方案
 */
struct SecurityScheme {
    std::string name;        // 方案名称（如：BearerAuth）
    std::string type;        // 类型（apiKey, http, oauth2, openIdConnect）
    std::string scheme;      // HTTP方案（bearer, basic）
    std::string bearerFormat;  // Bearer格式（JWT）
    std::string in;          // apiKey位置（header, query）
    std::string description;  // 描述
};

/**
 * @brief OpenAPI规范信息
 */
struct OpenAPIInfo {
    std::string title{"PaperCrawler API"};
    std::string version{"1.0.0"};
    std::string description{"PaperCrawler Backend API Documentation"};
    std::string termsOfService;
    std::string contactName;
    std::string contactEmail;
    std::string contactUrl;
    std::string licenseName{"MIT"};
    std::string licenseUrl;
};

/**
 * @brief 文档配置
 */
struct DocumentationConfig {
    bool enableSwaggerUI{true};            // 启用Swagger UI
    bool enableReDoc{false};               // 启用ReDoc
    std::string docsPath{"/api/docs"};     // 文档路径
    bool autoGenerate{true};               // 自动生成规范
    bool includeExamples{true};            // 包含示例
    bool enableTryItOut{true};             // 启用"Try it out"
    std::string uiTheme{"light"};          // UI主题（light, dark）
    std::vector<std::string> servers;      // 服务器列表
};

/**
 * @brief API文档模块
 *
 * 功能：
 * 1. 自动生成OpenAPI规范
 * 2. Swagger UI集成
 * 3. API示例代码生成
 * 4. API版本管理
 * 5. 文档导出（JSON/YAML）
 * 6. 交互式文档
 *
 * 特性：
 * - 自动化：从代码注释自动生成文档
 * - 标准化：符合OpenAPI 3.0规范
 * - 可视化：Swagger UI交互式文档
 * - 多语言：支持多语言文档
 * - 版本管理：支持API版本控制
 */
class APIDocumentationModule : public IModule {
public:
    APIDocumentationModule();
    ~APIDocumentationModule() override;

    std::string getName() const override { return "APIDocumentation"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Auto-generate OpenAPI specs and Swagger UI";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 注册API端点
     */
    void registerEndpoint(const APIEndpoint& endpoint);

    /**
     * @brief 批量注册端点
     */
    void registerEndpoints(const std::vector<APIEndpoint>& endpoints);

    /**
     * @brief 注册标签
     */
    void registerTag(const APITag& tag);

    /**
     * @brief 注册安全方案
     */
    void registerSecurityScheme(const SecurityScheme& scheme);

    /**
     * @brief 生成OpenAPI规范（JSON格式）
     */
    std::string generateOpenAPIJSON();

    /**
     * @brief 生成OpenAPI规范（YAML格式）
     */
    std::string generateOpenAPIYAML();

    /**
     * @brief 获取Swagger UI HTML
     */
    std::string getSwaggerUI();

    /**
     * @brief 获取ReDoc HTML
     */
    std::string getReDoc();

    /**
     * @brief 导出文档到文件
     */
    bool exportToFile(const std::string& filepath, bool asJSON = true);

    /**
     * @brief 从文件导入文档
     */
    bool importFromFile(const std::string& filepath);

    /**
     * @brief 获取所有端点
     */
    std::vector<APIEndpoint> getAllEndpoints() const;

    /**
     * @brief 根据路径获取端点
     */
    std::vector<APIEndpoint> getEndpointsByPath(const std::string& path) const;

    /**
     * @brief 根据标签获取端点
     */
    std::vector<APIEndpoint> getEndpointsByTag(const std::string& tag) const;

    /**
     * @brief 根据模块获取端点
     */
    std::vector<APIEndpoint> getEndpointsByModule(const std::string& moduleName) const;

    /**
     * @brief 删除端点
     */
    bool removeEndpoint(const std::string& path, HTTPMethod method);

    /**
     * @brief 清除所有端点
     */
    void clearAllEndpoints();

    /**
     * @brief 设置OpenAPI信息
     */
    void setOpenAPIInfo(const OpenAPIInfo& info);

    /**
     * @brief 设置配置
     */
    void setConfig(const DocumentationConfig& config);

    /**
     * @brief 添加服务器
     */
    void addServer(const std::string& url, const std::string& description = "");

    /**
     * @brief 文档统计
     */
    struct DocumentationStats {
        size_t totalEndpoints;
        size_t totalTags;
        size_t totalSecuritySchemes;
        std::map<std::string, size_t> endpointsByTag;
        std::map<std::string, size_t> endpointsByModule;
        std::map<HTTPMethod, size_t> endpointsByMethod;
    };
    DocumentationStats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::string httpMethodToString(HTTPMethod method) const;
    std::string parameterTypeToString(ParameterType type) const;
    std::string dataTypeToString(DataType type) const;

    DocumentationConfig config_;
    OpenAPIInfo openAPIInfo_;
    std::vector<APIEndpoint> endpoints_;
    std::vector<APITag> tags_;
    std::vector<SecurityScheme> securitySchemes_;
};

} // namespace PaperCrawler
