/**
 * AI Prompt Templates
 * AI提示词模板库 - 头文件
 *
 * 文件位置: backend/include/prompts/AIPromptTemplates.hpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace PaperCrawler {
namespace AI {

// ============================================================================
// 审稿人Prompt上下文
// ============================================================================

/**
 * @brief 审稿人Prompt上下文
 */
struct ReviewPromptContext {
    // 论文信息
    std::string paperId;
    std::string paperTitle;
    std::string paperAuthors;
    std::string paperAbstract;
    std::string paperContent;
    std::string researchField;
    std::string targetJournal;

    // 审稿选项
    bool includeComparison{true};      // 是否包含对比分析
    bool includeMethodology{true};     // 是否包含方法论评估
    bool includeReferences{true};      // 是否包含参考文献检查
    int maxSuggestions{5};             // 最大建议数量
};

/**
 * @brief 审稿风格
 */
enum class ReviewPromptStyle {
    Strict,       // 严格型（critical but fair）
    Balanced,     // 平衡型（objective and balanced）
    Encouraging   // 鼓励型（supportive and constructive）
};

// ============================================================================
// 文献综述Prompt上下文
// ============================================================================

/**
 * @brief 文献综述Prompt上下文
 */
struct LiteratureReviewContext {
    // 基本信息
    std::string userId;
    std::string title;
    std::string researchField;
    std::vector<int> paperIds;
    std::string reviewType{"narrative"}; // systematic, meta_analysis, narrative

    // 内容选项
    int maxLength{5000};             // 最大字数
    bool includeGaps{true};          // 包含研究空白
    bool includeTrends{true};        // 包含研究趋势
    bool includeMethodology{true};   // 包含方法论总结
    bool includeKeyFindings{true};   // 包含主要发现
    bool includeFutureDirections{true}; // 包含未来方向

    // 格式选项
    std::string outputFormat{"json"}; // json, markdown, html
    std::string language{"en"};       // en, zh
    int themeCount{5};                // 主题数量
};

// ============================================================================
// 研究计划Prompt上下文
// ============================================================================

/**
 * @brief 研究计划Prompt上下文
 */
struct ResearchPlanContext {
    // 基本信息
    std::string userId;
    std::string title;
    std::string researchQuestion;
    std::string researchField;
    std::vector<std::string> keywords;

    // 项目参数
    int durationMonths{12};           // 项目周期（月）
    std::string budgetLevel{"medium"}; // low, medium, high
    std::string fundingAgency{""};     // 资助机构

    // 内容选项
    bool includeTimeline{true};       // 包含时间安排
    bool includeBudget{true};         // 包含预算估算
    bool includeRisks{true};          // 包含风险评估
    bool includeTeam{true};           // 包含团队配置
    bool includeEthics{true};         // 包含伦理考量

    // 格式选项
    std::string outputFormat{"json"}; // json, markdown, pdf
    std::string language{"en"};       // en, zh
};

// ============================================================================
// AI Prompt模板类
// ============================================================================

/**
 * @brief AI提示词模板类
 *
 * 功能：
 * 1. 生成AI审稿人Prompt
 * 2. 生成文献综述Prompt
 * 3. 生成研究计划Prompt
 * 4. 多语言支持（中英文）
 * 5. 动态参数替换
 */
class AIPromptTemplates {
public:
    AIPromptTemplates();
    ~AIPromptTemplates();

    // ========================================================================
    // 1. AI审稿人Prompt生成器
    // ========================================================================

    /**
     * @brief 生成AI审稿人Prompt
     * @param context 审稿上下文
     * @param style 审稿风格
     * @return 完整的Prompt字符串
     */
    static std::string generateReviewPrompt(
        const ReviewPromptContext& context,
        ReviewPromptStyle style = ReviewPromptStyle::Balanced
    );

    /**
     * @brief 生成中文版AI审稿人Prompt
     * @param context 审稿上下文
     * @param style 审稿风格
     * @return 中文Prompt字符串
     */
    static std::string generateReviewPromptChinese(
        const ReviewPromptContext& context,
        ReviewPromptStyle style = ReviewPromptStyle::Balanced
    );

    // ========================================================================
    // 2. 文献综述Prompt生成器
    // ========================================================================

    /**
     * @brief 生成文献综述Prompt
     * @param context 综述上下文
     * @return 完整的Prompt字符串
     */
    static std::string generateLiteratureReviewPrompt(
        const LiteratureReviewContext& context
    );

    /**
     * @brief 生成中文版文献综述Prompt
     * @param context 综述上下文
     * @return 中文Prompt字符串
     */
    static std::string generateLiteratureReviewPromptChinese(
        const LiteratureReviewContext& context
    );

    // ========================================================================
    // 3. 研究计划Prompt生成器
    // ========================================================================

    /**
     * @brief 生成研究计划Prompt
     * @param context 计划上下文
     * @return 完整的Prompt字符串
     */
    static std::string generateResearchPlanPrompt(
        const ResearchPlanContext& context
    );

    /**
     * @brief 生成中文版研究计划Prompt
     * @param context 计划上下文
     * @return 中文Prompt字符串
     */
    static std::string generateResearchPlanPromptChinese(
        const ResearchPlanContext& context
    );

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 替换模板变量
     * @param templateStr 模板字符串
     * @param variables 变量映射表
     * @return 替换后的字符串
     *
     * 示例：
     * templateStr = "Hello {{name}}, your score is {{score}}"
     * variables = {{"name", "Alice"}, {"score", "95"}}
     * result = "Hello Alice, your score is 95"
     */
    static std::string replaceVariables(
        const std::string& templateStr,
        const std::map<std::string, std::string>& variables
    );

    /**
     * @brief 获取模板版本号
     * @return 版本号字符串
     */
    std::string getVersion() const { return impl_->version_; }

    /**
     * @brief 设置默认Token限制
     * @param maxTokens 最大Token数
     */
    void setDefaultMaxTokens(int maxTokens) { impl_->defaultMaxTokens_ = maxTokens; }

    /**
     * @brief 设置默认温度参数
     * @param temperature 温度值（0-1）
     */
    void setDefaultTemperature(double temperature) { impl_->defaultTemperature_ = temperature; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 内部辅助方法
    static std::string generateRoleSetting(const std::string& role, const std::string& context);
    static std::string generateReviewCriteria(const ReviewPromptContext& context);
    static std::string generateReviewOutputFormat(bool includeComparison);
    static std::string generateDomainGuidelines(const std::string& researchField);
};

// ============================================================================
// Prompt构建器（Builder模式）
// ============================================================================

/**
 * @brief Prompt构建器
 *
 * 提供流式API构建复杂的Prompt
 */
class PromptBuilder {
public:
    PromptBuilder& addRole(const std::string& role) {
        prompt_ += role + "\n\n";
        return *this;
    }

    PromptBuilder& addTask(const std::string& task) {
        prompt_ += "## Task\n\n" + task + "\n\n";
        return *this;
    }

    PromptBuilder& addContext(const std::string& key, const std::string& value) {
        context_[key] = value;
        return *this;
    }

    PromptBuilder& addCriteria(const std::vector<std::string>& criteria) {
        prompt_ += "## Criteria\n\n";
        for (size_t i = 0; i < criteria.size(); ++i) {
            prompt_ += std::to_string(i + 1) + ". " + criteria[i] + "\n";
        }
        prompt_ += "\n";
        return *this;
    }

    PromptBuilder& addOutputFormat(const std::string& format) {
        prompt_ += "## Output Format\n\n" + format + "\n\n";
        return *this;
    }

    PromptBuilder& addInstructions(const std::string& instructions) {
        prompt_ += "## Instructions\n\n" + instructions + "\n\n";
        return *this;
    }

    std::string build() const {
        return prompt_;
    }

    void reset() {
        prompt_.clear();
        context_.clear();
    }

private:
    std::string prompt_;
    std::map<std::string, std::string> context_;
};

// ============================================================================
// Prompt验证器
// ============================================================================

/**
 * @brief Prompt验证结果
 */
struct PromptValidationResult {
    bool valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    int estimatedTokens;
    std::map<std::string, std::string> metadata;
};

/**
 * @brief Prompt验证器
 */
class PromptValidator {
public:
    /**
     * @brief 验证Prompt
     * @param prompt Prompt字符串
     * @return 验证结果
     */
    static PromptValidationResult validate(const std::string& prompt);

    /**
     * @brief 估算Token数量
     * @param prompt Prompt字符串
     * @return 估算的Token数
     */
    static int estimateTokens(const std::string& prompt);

    /**
     * @brief 检查Prompt长度
     * @param prompt Prompt字符串
     * @param maxTokens 最大Token限制
     * @return 是否超出限制
     */
    static bool exceedsTokenLimit(const std::string& prompt, int maxTokens);

    /**
     * @brief 清理Prompt
     * @param prompt 原始Prompt
     * @return 清理后的Prompt
     */
    static std::string sanitize(const std::string& prompt);
};

// ============================================================================
// 预定义Prompt模板库
// ============================================================================

/**
 * @brief 预定义模板
 */
namespace PromptTemplates {

    // AI审稿人模板
    const std::string REVIEW_STRICT = "review_strict_v2";
    const std::string REVIEW_BALANCED = "review_balanced_v2";
    const std::string REVIEW_ENCOURAGING = "review_encouraging_v2";

    // 文献综述模板
    const std::string LIT_REVIEW_SYSTEMATIC = "lit_review_systematic_v2";
    const std::string LIT_REVIEW_NARRATIVE = "lit_review_narrative_v2";
    const std::string LIT_REVIEW_META_ANALYSIS = "lit_review_meta_analysis_v2";

    // 研究计划模板
    const std::string RESEARCH_PLAN_GRANT = "research_plan_grant_v2";
    const std::string RESEARCH_PLAN_THESIS = "research_plan_thesis_v2";
    const std::string RESEARCH_PLAN_PROJECT = "research_plan_project_v2";

    /**
     * @brief 获取预定义模板
     * @param templateName 模板名称
     * @return 模板内容
     */
    std::string getTemplate(const std::string& templateName);

    /**
     * @brief 列出所有可用模板
     * @return 模板名称列表
     */
    std::vector<std::string> listTemplates();

} // namespace PromptTemplates

} // namespace AI
} // namespace PaperCrawler
