#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <optional>
#include <any>
#include <limits>

namespace PaperCrawler {

// 前向声明
struct HttpRequest;

/**
 * @brief 验证规则类型
 */
enum class ValidationRuleType {
    REQUIRED,       // 必填
    TYPE,           // 类型检查
    RANGE,          // 范围检查
    LENGTH,         // 长度检查
    PATTERN,        // 正则表达式
    EMAIL,          // 邮箱
    URL,            // URL
    CUSTOM,         // 自定义
    ARRAY,          // 数组
    OBJECT,         // 对象
    ENUM,           // 枚举
    DATE,           // 日期
    BOOLEAN,        // 布尔值
    NUMBER          // 数字
};

/**
 * @brief 验证规则基类
 */
class IValidationRule {
public:
    virtual ~IValidationRule() = default;

    /**
     * @brief 验证值
     * @return pair<是否通过, 错误消息>
     */
    virtual std::pair<bool, std::string> validate(const std::string& fieldName,
                                                  const std::any& value) const = 0;

    /**
     * @brief 获取规则类型
     */
    virtual ValidationRuleType getType() const = 0;

    /**
     * @brief 获取规则名称
     */
    virtual std::string getName() const = 0;
};

/**
 * @brief 验证结果
 */
struct ValidationResult {
    bool valid{true};
    std::string fieldName;
    std::string errorMessage;
    ValidationRuleType failedRule;
    std::any actualValue;
};

/**
 * @brief 验证上下文
 */
struct ValidationContext {
    std::map<std::string, std::string> pathVariables;  // 路径变量
    std::map<std::string, std::string> queryParameters; // 查询参数
    std::map<std::string, std::string> headers;        // 请求头
    std::string body;                                  // 请求体
    std::any parsedBody;                              // 解析后的请求体（JSON等）
};

/**
 * @brief 字段验证规则集合
 */
struct FieldValidation {
    std::string fieldName;
    std::vector<std::shared_ptr<IValidationRule>> rules;
    bool required{false};

    /**
     * @brief 添加规则
     */
    void addRule(std::shared_ptr<IValidationRule> rule) {
        rules.push_back(rule);
    }
};

/**
 * @brief JSON Schema验证配置
 */
struct SchemaValidationConfig {
    bool enableOpenAPISchema{true};       // 启用OpenAPI Schema验证
    bool enableStrictMode{false};         // 严格模式（不允许额外字段）
    bool enableCustomValidators{true};    // 启用自定义验证器
    std::string customValidatorsPath{"./config/validators"};
};

/**
 * @brief 验证模块统计
 */
struct ValidationStats {
    uint64_t totalRequestsValidated{0};
    uint64_t validationFailures{0};
    uint64_t validationSuccesses{0};
    double failureRate{0.0};
    std::map<std::string, uint64_t> failuresByField;
    std::map<ValidationRuleType, uint64_t> failuresByRuleType;
};

/**
 * @brief 验证模块
 *
 * 功能：
 * 1. 请求参数验证
 * 2. JSON Schema验证
 * 3. 自定义验证规则
 * 4. OpenAPI规范验证
 * 5. 类型安全验证
 * 6. 业务规则验证
 *
 * 特性：
 * - 声明式：配置驱动的验证规则
 * - 可扩展：支持自定义验证器
 * - 高性能：编译时优化
 * - 详细错误：清晰的错误消息
 * - 国际化：支持多语言错误消息
 */
class ValidationModule : public IModule {
public:
    ValidationModule();
    ~ValidationModule() override;

    std::string getName() const override { return "Validation"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Request validation with JSON Schema and custom rules";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 验证HTTP请求
     * @return 验证结果（valid=true表示通过）
     */
    std::vector<ValidationResult> validateRequest(const HttpRequest& request);

    /**
     * @brief 验证上下文
     */
    std::vector<ValidationResult> validateContext(const ValidationContext& context);

    /**
     * @brief 验证单个字段
     */
    ValidationResult validateField(const std::string& fieldName,
                                   const std::any& value,
                                   const std::vector<std::shared_ptr<IValidationRule>>& rules);

    /**
     * @brief 添加字段验证规则
     */
    void addFieldValidation(const std::string& fieldName, const FieldValidation& validation);

    /**
     * @brief 添加验证规则
     */
    void addRule(const std::string& fieldName, std::shared_ptr<IValidationRule> rule);

    /**
     * @brief 移除字段验证
     */
    void removeFieldValidation(const std::string& fieldName);

    /**
     * @brief 验证JSON Schema
     */
    std::vector<ValidationResult> validateJSONSchema(const std::string& json,
                                                     const std::string& schema);

    /**
     * @brief 加载JSON Schema
     */
    bool loadSchema(const std::string& schemaName, const std::string& schemaFilePath);

    /**
     * @brief 验证OpenAPI规范
     */
    std::vector<ValidationResult> validateOpenAPISpec(const std::string& endpoint,
                                                      const std::string& method,
                                                      const std::any& requestBody);

    /**
     * @brief 加载OpenAPI规范
     */
    bool loadOpenAPISpec(const std::string& specFilePath);

    /**
     * @brief 添加自定义验证器
     */
    void addCustomValidator(const std::string& name,
                           std::function<std::pair<bool, std::string>(const std::any&)> validator);

    /**
     * @brief 移除自定义验证器
     */
    void removeCustomValidator(const std::string& name);

    /**
     * @brief 获取验证统计
     */
    ValidationStats getStats() const;

    /**
     * @brief 设置配置
     */
    void setConfig(const SchemaValidationConfig& config);

    /**
     * @brief 设置严格模式
     */
    void setStrictMode(bool enabled);

    /**
     * @brief 清除所有验证规则
     */
    void clearAllRules();

    /**
     * @brief 清除所有Schema
     */
    void clearAllSchemas();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    SchemaValidationConfig config_;
    std::map<std::string, FieldValidation> fieldValidations_;
    std::map<std::string, std::string> schemas_;  // schemaName -> schema JSON
    std::map<std::string, std::function<std::pair<bool, std::string>(const std::any&)>> customValidators_;

    ValidationStats stats_;
};

// ============================================================================
// 内置验证规则
// ============================================================================

/**
 * @brief 必填规则
 */
class RequiredRule : public IValidationRule {
public:
    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::REQUIRED; }
    std::string getName() const override { return "required"; }
};

/**
 * @brief 类型规则
 */
class TypeRule : public IValidationRule {
public:
    enum class ValueType { STRING, NUMBER, INTEGER, BOOLEAN, ARRAY, OBJECT };

    TypeRule(ValueType expectedType) : expectedType_(expectedType) {}

    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::TYPE; }
    std::string getName() const override;

private:
    ValueType expectedType_;
};

/**
 * @brief 范围规则
 */
class RangeRule : public IValidationRule {
public:
    RangeRule(double min, double max, bool inclusive = true)
        : min_(min), max_(max), inclusive_(inclusive) {}

    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::RANGE; }
    std::string getName() const override { return "range"; }

private:
    double min_;
    double max_;
    bool inclusive_;
};

/**
 * @brief 长度规则
 */
class LengthRule : public IValidationRule {
public:
    LengthRule(size_t min, size_t max = std::numeric_limits<size_t>::max())
        : min_(min), max_(max) {}

    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::LENGTH; }
    std::string getName() const override { return "length"; }

private:
    size_t min_;
    size_t max_;
};

/**
 * @brief 正则表达式规则
 */
class PatternRule : public IValidationRule {
public:
    PatternRule(const std::string& pattern) : pattern_(pattern) {}

    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::PATTERN; }
    std::string getName() const override { return "pattern"; }

private:
    std::string pattern_;
};

/**
 * @brief 邮箱规则
 */
class EmailRule : public IValidationRule {
public:
    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::EMAIL; }
    std::string getName() const override { return "email"; }
};

/**
 * @brief URL规则
 */
class URLRule : public IValidationRule {
public:
    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::URL; }
    std::string getName() const override { return "url"; }
};

/**
 * @brief 枚举规则
 */
class EnumRule : public IValidationRule {
public:
    EnumRule(const std::vector<std::string>& allowedValues)
        : allowedValues_(allowedValues) {}

    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override;
    ValidationRuleType getType() const override { return ValidationRuleType::ENUM; }
    std::string getName() const override { return "enum"; }

private:
    std::vector<std::string> allowedValues_;
};

/**
 * @brief 自定义规则
 */
class CustomRule : public IValidationRule {
public:
    using CustomValidator = std::function<std::pair<bool, std::string>(const std::string&, const std::any&)>;

    CustomRule(const std::string& name, CustomValidator validator)
        : name_(name), validator_(validator) {}

    std::pair<bool, std::string> validate(const std::string& fieldName,
                                         const std::any& value) const override {
        return validator_(fieldName, value);
    }
    ValidationRuleType getType() const override { return ValidationRuleType::CUSTOM; }
    std::string getName() const override { return name_; }

private:
    std::string name_;
    CustomValidator validator_;
};

} // namespace PaperCrawler
