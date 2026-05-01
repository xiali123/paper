#pragma once

#include "core/HttpTypes.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <optional>

namespace PaperCrawler {

/**
 * @brief HTTP响应构建器
 *
 * 提供统一的API响应格式，减少重复代码
 *
 * 标准响应格式：
 * {
 *   "success": true/false,
 *   "message": "description",
 *   "data": {...},
 *   "error": "error message" (仅失败时)
 * }
 */
class ResponseBuilder {
public:
    /**
     * @brief 构建成功响应（仅消息）
     */
    static HttpResponse success(const std::string& message = "Operation successful") {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = json{
            {"success", true},
            {"message", message}
        }.dump();
        return response;
    }

    /**
     * @brief 构建成功响应（带数据）
     */
    static HttpResponse success(const std::string& message, const nlohmann::json& data) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = json{
            {"success", true},
            {"message", message},
            {"data", data}
        }.dump();
        return response;
    }

    /**
     * @brief 构建成功响应（带数据和元数据）
     */
    static HttpResponse success(const std::string& message,
                                const nlohmann::json& data,
                                const nlohmann::json& meta) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = json{
            {"success", true},
            {"message", message},
            {"data", data},
            {"meta", meta}
        }.dump();
        return response;
    }

    /**
     * @brief 构建创建成功响应（HTTP 201）
     */
    static HttpResponse created(const std::string& message = "Resource created",
                                const nlohmann::json& data = {}) {
        HttpResponse response;
        response.statusCode = 201;
        response.headers["Content-Type"] = "application/json";
        response.body = json{
            {"success", true},
            {"message", message},
            {"data", data}
        }.dump();
        return response;
    }

    /**
     * @brief 构建错误响应（HTTP 400）
     */
    static HttpResponse badRequest(const std::string& message) {
        return error(400, message);
    }

    /**
     * @brief 构建未授权响应（HTTP 401）
     */
    static HttpResponse unauthorized(const std::string& message = "Unauthorized") {
        return error(401, message);
    }

    /**
     * @brief 构建禁止访问响应（HTTP 403）
     */
    static HttpResponse forbidden(const std::string& message = "Forbidden") {
        return error(403, message);
    }

    /**
     * @brief 构建未找到响应（HTTP 404）
     */
    static HttpResponse notFound(const std::string& message = "Resource not found") {
        return error(404, message);
    }

    /**
     * @brief 构建方法不允许响应（HTTP 405）
     */
    static HttpResponse methodNotAllowed(const std::string& message = "Method not allowed") {
        return error(405, message);
    }

    /**
     * @brief 构建冲突响应（HTTP 409）
     */
    static HttpResponse conflict(const std::string& message = "Resource conflict") {
        return error(409, message);
    }

    /**
     * @brief 构建未验证实体响应（HTTP 422）
     */
    static HttpResponse unprocessableEntity(const std::string& message,
                                             const nlohmann::json& errors = {}) {
        HttpResponse response;
        response.statusCode = 422;
        response.headers["Content-Type"] = "application/json";
        json body = {
            {"success", false},
            {"error", message}
        };
        if (!errors.empty() && errors.is_object()) {
            body["errors"] = errors;
        }
        response.body = body.dump();
        return response;
    }

    /**
     * @brief 构建服务器错误响应（HTTP 500）
     */
    static HttpResponse internalError(const std::string& message = "Internal server error") {
        return error(500, message);
    }

    /**
     * @brief 构建服务不可用响应（HTTP 503）
     */
    static HttpResponse serviceUnavailable(const std::string& message = "Service unavailable") {
        return error(503, message);
    }

    /**
     * @brief 构建分页响应
     */
    static HttpResponse paginated(const nlohmann::json& items,
                                  int page,
                                  int limit,
                                  int total,
                                  const std::string& message = "Success") {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = json{
            {"success", true},
            {"message", message},
            {"data", items},
            {"pagination", {
                {"page", page},
                {"limit", limit},
                {"total", total},
                {"totalPages", (total + limit - 1) / limit}
            }}
        }.dump();
        return response;
    }

    /**
     * @brief 构建自定义状态码响应
     */
    static HttpResponse error(int statusCode, const std::string& message) {
        HttpResponse response;
        response.statusCode = statusCode;
        response.headers["Content-Type"] = "application/json";
        response.body = json{
            {"success", false},
            {"error", message}
        }.dump();
        return response;
    }

    /**
     * @brief 从异常构建错误响应
     */
    static HttpResponse fromException(const std::exception& e, int statusCode = 500) {
        return error(statusCode, e.what());
    }

    /**
     * @brief 构建JSON响应（自定义）
     */
    static HttpResponse json(const nlohmann::json& body, int statusCode = 200) {
        HttpResponse response;
        response.statusCode = statusCode;
        response.headers["Content-Type"] = "application/json";
        response.body = body.dump();
        return response;
    }

    /**
     * @brief 构建纯文本响应
     */
    static HttpResponse text(const std::string& body, int statusCode = 200) {
        HttpResponse response;
        response.statusCode = statusCode;
        response.headers["Content-Type"] = "text/plain";
        response.body = body;
        return response;
    }

    /**
     * @brief 验证JSON请求体
     * @return optional包含解析后的JSON，错误时返回nullopt
     */
    static std::optional<nlohmann::json> parseJson(const std::string& body) {
        try {
            return nlohmann::json::parse(body);
        } catch (const nlohmann::json::exception& e) {
            return std::nullopt;
        }
    }

    /**
     * @brief 验证必填字段
     * @param json 要验证的JSON对象
     * @param fields 必填字段列表
     * @return 错误响应（如果有缺失字段），否则返回空optional
     */
    static std::optional<HttpResponse> validateRequiredFields(
        const nlohmann::json& json,
        const std::vector<std::string>& fields) {

        nlohmann::json missingFields;
        for (const auto& field : fields) {
            if (!json.contains(field) || json[field].is_null()) {
                missingFields.push_back(field);
            }
        }

        if (!missingFields.empty()) {
            return unprocessableEntity("Missing required fields",
                {{"missing", missingFields}});
        }

        return std::nullopt;
    }

    /**
     * @brief 验证字段类型
     * @param json 要验证的JSON对象
     * @param fieldTypes 字段到类型的映射 {field: "type"}
     * @return 错误响应（如果有类型不匹配），否则返回空optional
     */
    static std::optional<HttpResponse> validateFieldTypes(
        const nlohmann::json& json,
        const std::map<std::string, std::string>& fieldTypes) {

        nlohmann::json invalidFields;
        for (const auto& [field, expectedType] : fieldTypes) {
            if (!json.contains(field)) continue;

            bool valid = false;
            const auto& value = json[field];

            if (expectedType == "string") {
                valid = value.is_string();
            } else if (expectedType == "number") {
                valid = value.is_number();
            } else if (expectedType == "integer") {
                valid = value.is_number_integer();
            } else if (expectedType == "boolean") {
                valid = value.is_boolean();
            } else if (expectedType == "array") {
                valid = value.is_array();
            } else if (expectedType == "object") {
                valid = value.is_object();
            }

            if (!valid) {
                invalidFields[field] = expectedType;
            }
        }

        if (!invalidFields.empty()) {
            return unprocessableEntity("Invalid field types",
                {{"invalid", invalidFields}});
        }

        return std::nullopt;
    }
};

} // namespace PaperCrawler
