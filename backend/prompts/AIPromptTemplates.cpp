/**
 * AI Prompt Templates
 * AI提示词模板库
 *
 * 文件位置: backend/prompts/AIPromptTemplates.cpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 *
 * 功能：
 * 1. AI审稿人Prompt模板
 * 2. 文献综述Prompt模板
 * 3. 研究计划Prompt模板
 * 4. 动态参数替换
 * 5. 多语言支持（中英文）
 */

#include "prompts/AIPromptTemplates.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <regex>
#include <iomanip>

namespace PaperCrawler {
namespace AI {

// ============================================================================
// AI Prompt Templates 实现
// ============================================================================

class AIPromptTemplates::Impl {
public:
    // 模板版本
    std::string version_ = "2.0";

    // 默认配置
    int defaultMaxTokens_ = 2000;
    double defaultTemperature_ = 0.7;
};

AIPromptTemplates::AIPromptTemplates()
    : impl_(std::make_unique<Impl>()) {
}

AIPromptTemplates::~AIPromptTemplates() = default;

// ============================================================================
// 1. AI审稿人Prompt模板
// ============================================================================

std::string AIPromptTemplates::generateReviewPrompt(
    const ReviewPromptContext& context,
    ReviewPromptStyle style) {

    std::ostringstream prompt;

    // 角色设定
    prompt << generateRoleSetting("expert_reviewer", context.targetJournal);

    // 任务说明
    prompt << "## Task\n\n";
    prompt << "You are reviewing a research paper submission for **" << context.targetJournal << "**.\n\n";

    // 审稿风格设定
    prompt << "## Review Style\n\n";
    switch (style) {
        case ReviewPromptStyle::Strict:
            prompt << "Adopt a **strict and rigorous** review style. Be critical but fair. ";
            prompt << "Focus on identifying weaknesses and areas for improvement. ";
            prompt << "Hold the paper to high scientific standards.\n\n";
            break;
        case ReviewPromptStyle::Balanced:
            prompt << "Adopt a **balanced and objective** review style. ";
            prompt << "Acknowledge both strengths and weaknesses equally. ";
            prompt << "Provide constructive feedback for improvement.\n\n";
            break;
        case ReviewPromptStyle::Encouraging:
            prompt << "Adopt an **encouraging and supportive** review style. ";
            prompt << "Emphasize the paper's contributions while suggesting improvements. ";
            prompt << "Focus on helping the authors enhance their work.\n\n";
            break;
    }

    // 论文信息
    prompt << "## Paper Information\n\n";
    prompt << "**Title:** " << context.paperTitle << "\n\n";
    prompt << "**Authors:** " << context.paperAuthors << "\n\n";
    prompt << "**Abstract:**\n" << context.paperAbstract << "\n\n";

    if (!context.paperContent.empty()) {
        prompt << "**Full Paper Content:**\n" << context.paperContent << "\n\n";
    }

    // 审稿标准
    prompt << "## Review Criteria\n\n";
    prompt << generateReviewCriteria(context);

    // 输出格式要求
    prompt << "## Output Format\n\n";
    prompt << generateReviewOutputFormat(context.includeComparison);

    // 领域特定指导
    if (!context.researchField.empty()) {
        prompt << "## Domain-Specific Guidelines\n\n";
        prompt << generateDomainGuidelines(context.researchField);
    }

    // 最终指令
    prompt << "## Instructions\n\n";
    prompt << "Please provide a comprehensive review following the format above. ";
    prompt << "Be specific, constructive, and fair in your assessment.\n\n";

    return prompt.str();
}

std::string AIPromptTemplates::generateRoleSetting(
    const std::string& role,
    const std::string& context) {

    std::ostringstream roleSetting;

    if (role == "expert_reviewer") {
        roleSetting << "You are an **expert peer reviewer** for " << context << ".\n";
        roleSetting << "You have extensive experience in academic publishing and have reviewed ";
        roleSetting << "numerous papers for top-tier journals and conferences.\n\n";
        roleSetting << "Your expertise includes:\n";
        roleSetting << "- Deep knowledge of research methodology and experimental design\n";
        roleSetting << "- Strong understanding of statistical analysis and data interpretation\n";
        roleSetting << "- Familiarity with current literature and research trends\n";
        roleSetting << "- Ability to provide constructive and actionable feedback\n\n";
    }

    return roleSetting.str();
}

std::string AIPromptTemplates::generateReviewCriteria(const ReviewPromptContext& context) {
    std::ostringstream criteria;

    criteria << "Please evaluate the paper based on the following criteria:\n\n";
    criteria << "1. **Originality and Novelty** (1-10)\n";
    criteria << "   - Does the paper present new ideas or findings?\n";
    criteria << "   - Is the research question innovative and timely?\n";
    criteria << "   - Does it advance the state of knowledge?\n\n";

    criteria << "2. **Technical Soundness** (1-10)\n";
    criteria << "   - Is the methodology appropriate and well-designed?\n";
    criteria << "   - Are the experiments/analyses rigorous and reproducible?\n";
    criteria << "   - Is the statistical analysis correct and appropriate?\n\n";

    criteria << "3. **Clarity and Presentation** (1-10)\n";
    criteria << "   - Is the paper well-organized and easy to follow?\n";
    criteria << "   - Are the figures and tables clear and informative?\n";
    criteria << "   - Is the writing clear and grammatically correct?\n\n";

    criteria << "4. **Significance and Impact** (1-10)\n";
    criteria << "   - Does the paper address an important problem?\n";
    criteria << "   - What are the potential practical applications?\n";
    criteria << "   - How will this work influence the field?\n\n";

    criteria << "5. **References and Related Work** (1-10)\n";
    criteria << "   - Is the literature review comprehensive and up-to-date?\n";
    criteria << "   - Does the paper adequately cite and compare with related work?\n";
    criteria << "   - Are the references appropriate and relevant?\n\n";

    return criteria.str();
}

std::string AIPromptTemplates::generateReviewOutputFormat(bool includeComparison) {
    std::ostringstream format;

    format << "Please provide your review in the following **JSON format**:\n\n";
    format << "```json\n";
    format << "{\n";
    format << "  \"review_score\": <integer 1-10>,\n";
    format << "  \"acceptance_probability\": <float 0-1>,\n";
    format << "  \"methodology_score\": <integer 1-10>,\n";
    format << "  \"innovation_score\": <integer 1-10>,\n";
    format << "  \"presentation_score\": <integer 1-10>,\n";
    format << "  \"strengths\": [\n";
    format << "    \"<specific strength 1>\",\n";
    format << "    \"<specific strength 2>\",\n";
    format << "    \"<specific strength 3>\",\n";
    format << "    \"...\n";
    format << "  ],\n";
    format << "  \"weaknesses\": [\n";
    format << "    \"<specific weakness 1>\",\n";
    format << "    \"<specific weakness 2>\",\n";
    format << "    \"<specific weakness 3>\",\n";
    format << "    \"...\n";
    format << "  ],\n";
    format << "  \"improvement_suggestions\": [\n";
    format << "    \"<specific suggestion 1>\",\n";
    format << "    \"<specific suggestion 2>\",\n";
    format << "    \"<specific suggestion 3>\",\n";
    format << "    \"...\n";
    format << "  ],\n";
    format << "  \"reviewer_comments\": \"<detailed comprehensive comments>\",\n";

    if (includeComparison) {
        format << "  \"compared_papers\": [\n";
        format << "    {\n";
        format << "      \"title\": \"<paper title>\",\n";
        format << "      \"authors\": \"<authors>\",\n";
        format << "      \"year\": <year>,\n";
        format << "      \"similarity\": \"<how it's similar>\",\n";
        format << "      \"difference\": \"<how it's different>\",\n";
        format << "      \"reason\": \"<why this comparison is relevant>\"\n";
        format << "    },\n";
        format << "    \"...\"\n";
        format << "  ],\n";
    }

    format << "  \"recommendation\": \"<accept / minor revision / major revision / reject>\",\n";
    format << "  \"confidence\": \"<high / medium / low>\"\n";
    format << "}\n";
    format << "```\n\n";

    format << "**Important Notes:**\n";
    format << "- All scores should be integers between 1 and 10\n";
    format << "- Acceptance probability should be a float between 0 and 1\n";
    format << "- Provide at least 3-5 specific points for strengths, weaknesses, and suggestions\n";
    format << "- Comments should be detailed (200-500 words)\n";
    format << "- Be constructive and specific in your feedback\n\n";

    return format.str();
}

// ============================================================================
// 2. 文献综述Prompt模板
// ============================================================================

std::string AIPromptTemplates::generateLiteratureReviewPrompt(
    const LiteratureReviewContext& context) {

    std::ostringstream prompt;

    // 角色设定
    prompt << "You are an **expert academic researcher and scholar** with deep expertise in ";
    prompt << context.researchField << ".\n";
    prompt << "You have extensive experience conducting literature reviews and meta-analyses.\n\n";

    // 任务说明
    prompt << "## Task\n\n";
    prompt << "Generate a comprehensive literature review on: **" << context.title << "**\n\n";

    // 综述范围
    prompt << "## Review Scope\n\n";
    prompt << "- **Field**: " << context.researchField << "\n";
    prompt << "- **Number of papers**: " << context.paperIds.size() << "\n";
    prompt << "- **Max length**: " << context.maxLength << " words\n\n";

    // 综述类型
    prompt << "## Review Type\n\n";
    if (context.reviewType == "systematic") {
        prompt << "This is a **systematic literature review** following PRISMA guidelines.\n";
    } else if (context.reviewType == "meta_analysis") {
        prompt << "This is a **meta-analysis** synthesizing quantitative results.\n";
    } else {
        prompt << "This is a **narrative literature review** providing critical analysis.\n";
    }
    prompt << "\n";

    // 内容要求
    prompt << "## Required Content\n\n";
    if (context.includeGaps) {
        prompt << "✅ Include **research gaps** and limitations in current literature\n";
    }
    if (context.includeTrends) {
        prompt << "✅ Include **research trends** and future directions\n";
    }
    if (context.includeMethodology) {
        prompt << "✅ Include **methodology summary** across studies\n";
    }
    if (context.includeKeyFindings) {
        prompt << "✅ Include **key findings** and contributions\n";
    }
    if (context.includeFutureDirections) {
        prompt << "✅ Include **future research directions** and opportunities\n";
    }
    prompt << "\n";

    // 输出格式
    prompt << "## Output Format\n\n";
    prompt << "Please provide your review in the following **JSON format**:\n\n";
    prompt << "```json\n";
    prompt << "{\n";
    prompt << "  \"title\": \"" << context.title << "\",\n";
    prompt << "  \"abstract\": \"<200-300 word summary of the review>\",\n";
    prompt << "  \"introduction\": \"<background and motivation>\",\n";
    prompt << "  \"methodology\": \"<methodology used in this review>\",\n";

    if (context.includeKeyFindings) {
        prompt << "  \"key_findings\": [\n";
        prompt << "    \"<key finding 1>\",\n";
        prompt << "    \"<key finding 2>\",\n";
        prompt << "    \"...\"\n";
        prompt << "  ],\n";
    }

    if (context.includeMethodology) {
        prompt << "  \"methodology_summary\": \"<summary of methodologies across papers>\",\n";
    }

    prompt << "  \"themes\": [\n";
    prompt << "    {\n";
    prompt << "      \"theme\": \"<theme name>\",\n";
    prompt << "      \"description\": \"<theme description>\",\n";
    prompt << "      \"papers\": [<paper_ids>],\n";
    prompt << "      \"key_insights\": [\n";
    prompt << "        \"<insight 1>\",\n";
    prompt << "        \"<insight 2>\"\n";
    prompt << "      ]\n";
    prompt << "    },\n";
    prompt << "    \"...\"\n";
    prompt << "  ],\n";

    if (context.includeGaps) {
        prompt << "  \"research_gaps\": [\n";
        prompt << "    {\n";
        prompt << "      \"gap\": \"<specific research gap>\",\n";
        prompt << "      \"importance\": \"<why this gap matters>\",\n";
        prompt << "      \"potential_solutions\": [\n";
        prompt << "        \"<potential solution 1>\",\n";
        prompt << "        \"<potential solution 2>\"\n";
        prompt << "      ]\n";
        prompt << "    },\n";
        prompt << "    \"...\"\n";
        prompt << "  ],\n";
    }

    if (context.includeTrends) {
        prompt << "  \"trends\": [\n";
        prompt << "    {\n";
        prompt << "      \"trend\": \"<research trend>\",\n";
        prompt << "      \"direction\": \"<emerging / growing / declining>\",\n";
        prompt << "      \"evidence\": \"<supporting evidence>\",\n";
        prompt << "      \"future_outlook\": \"<future predictions>\"\n";
        prompt << "    },\n";
        prompt << "    \"...\"\n";
        prompt << "  ],\n";
    }

    if (context.includeFutureDirections) {
        prompt << "  \"future_directions\": [\n";
        prompt << "    {\n";
        prompt << "      \"direction\": \"<future research direction>\",\n";
        prompt << "      \"rationale\": \"<why this direction is promising>\",\n";
        prompt << "      \"challenges\": [\n";
        prompt << "        \"<challenge 1>\",\n";
        prompt << "        \"<challenge 2>\"\n";
        prompt << "      ],\n";
        prompt << "      \"opportunities\": [\n";
        prompt << "        \"<opportunity 1>\",\n";
        prompt << "        \"<opportunity 2>\"\n";
        prompt << "      ]\n";
        prompt << "    },\n";
        prompt << "    \"...\"\n";
        prompt << "  ],\n";
    }

    prompt << "  \"conclusion\": \"<concluding remarks and takeaways>\",\n";
    prompt << "  \"references_count\": <number_of_references>\n";
    prompt << "}\n";
    prompt << "```\n\n";

    // 具体要求
    prompt << "## Guidelines\n\n";
    prompt << "- Organize papers into coherent themes and categories\n";
    prompt << "- Synthesize findings, don't just summarize individual papers\n";
    prompt << "- Identify contradictions and inconsistencies in the literature\n";
    prompt << "- Highlight methodological differences and their impacts\n";
    prompt << "- Be critical and analytical in your approach\n";
    prompt << "- Use academic language and maintain scholarly tone\n";
    prompt << "- Keep total review within " << context.maxLength << " words\n\n";

    return prompt.str();
}

// ============================================================================
// 3. 研究计划Prompt模板
// ============================================================================

std::string AIPromptTemplates::generateResearchPlanPrompt(
    const ResearchPlanContext& context) {

    std::ostringstream prompt;

    // 角色设定
    prompt << "You are an **expert research advisor and grant reviewer** with extensive experience in ";
    prompt << context.researchField << ".\n";
    prompt << "You have successfully mentored numerous PhD students and reviewed countless research proposals.\n\n";

    // 任务说明
    prompt << "## Task\n\n";
    prompt << "Create a detailed research plan for the following project:\n\n";
    prompt << "**Title**: " << context.title << "\n\n";
    prompt << "**Research Question**: " << context.researchQuestion << "\n\n";

    // 项目背景
    prompt << "## Project Background\n\n";
    prompt << "- **Field**: " << context.researchField << "\n";
    prompt << "- **Duration**: " << context.durationMonths << " months\n";
    prompt << "- **Budget Level**: " << context.budgetLevel << "\n";

    if (!context.keywords.empty()) {
        prompt << "- **Keywords**: ";
        for (size_t i = 0; i < context.keywords.size(); ++i) {
            prompt << context.keywords[i];
            if (i < context.keywords.size() - 1) prompt << ", ";
        }
        prompt << "\n";
    }
    prompt << "\n";

    // 输出格式
    prompt << "## Output Format\n\n";
    prompt << "Please provide your research plan in the following **JSON format**:\n\n";
    prompt << "```json\n";
    prompt << "{\n";
    prompt << "  \"title\": \"" << context.title << "\",\n";
    prompt << "  \"research_question\": \"" << context.researchQuestion << "\",\n";
    prompt << "  \"abstract\": \"<200-300 word summary>\",\n\n";

    // 背景和意义
    prompt << "  \"background_and_significance\": {\n";
    prompt << "    \"current_state\": \"<current state of the field>\",\n";
    prompt << "    \"knowledge_gap\": \"<specific gap this project addresses>\",\n";
    prompt << "    \"significance\": \"<why this research matters>\",\n";
    prompt << "    \"potential_impact\": \"<expected impact on the field>\",\n";
    prompt << "    \"broader_impacts\": \"<societal, economic, or educational impacts>\"\n";
    prompt << "  },\n\n";

    // 研究目标
    prompt << "  \"objectives\": [\n";
    prompt << "    {\n";
    prompt << "      \"objective\": \"<specific objective 1>\",\n";
    prompt << "      \"description\": \"<detailed description>\",\n";
    prompt << "      \"success_criteria\": \"<how to measure success>\",\n";
    prompt << "      \"priority\": \"<high / medium / low>\"\n";
    prompt << "    },\n";
    prompt << "    \"...\"\n";
    prompt << "  ],\n\n";

    // 方法论
    prompt << "  \"methodology\": {\n";
    prompt << "    \"research_design\": \"<overall research design>\",\n";
    prompt << "    \"approach\": \"<qualitative / quantitative / mixed / computational>\",\n";
    prompt << "    \"data_collection\": {\n";
    prompt << "      \"primary_methods\": [\n";
    prompt << "        \"<method 1>\",\n";
    prompt << "        \"<method 2>\"\n";
    prompt << "      ],\n";
    prompt << "      \"secondary_sources\": [\n";
    prompt << "        \"<source 1>\",\n";
    prompt << "        \"<source 2>\"\n";
    prompt << "      ]\n";
    prompt << "    },\n";
    prompt << "    \"data_analysis\": {\n";
    prompt << "      \"tools\": [\n";
    prompt << "        \"<tool 1>\",\n";
    prompt << "        \"<tool 2>\"\n";
    prompt << "      ],\n";
    prompt << "      \"techniques\": [\n";
    prompt << "        \"<technique 1>\",\n";
    prompt << "        \"<technique 2>\"\n";
    prompt << "      ]\n";
    prompt << "    },\n";
    prompt << "    \"validation\": \"<how to validate findings>\",\n";
    prompt << "    \"limitations\": \"<methodological limitations>\"\n";
    prompt << "  },\n\n";

    // 时间安排
    prompt << "  \"timeline\": {\n";
    prompt << "    \"phase_1\": {\n";
    prompt << "      \"name\": \"<Phase 1 name>\",\n";
    prompt << "      \"duration\": \"<X months>\",\n";
    prompt << "      \"objectives\": [\n";
    prompt << "        \"<objective 1>\",\n";
    prompt << "        \"<objective 2>\"\n";
    prompt << "      ],\n";
    prompt << "      \"deliverables\": [\n";
    prompt << "        \"<deliverable 1>\",\n";
    prompt << "        \"<deliverable 2>\"\n";
    prompt << "      ]\n";
    prompt << "    },\n";
    prompt << "    \"...\"\n";
    prompt << "  },\n\n";

    // 所需资源
    prompt << "  \"required_resources\": {\n";
    prompt << "    \"personnel\": [\n";
    prompt << "      {\n";
    prompt << "        \"role\": \"<role>\",\n";
    prompt << "        \"qualifications\": \"<required qualifications>\",\n";
    prompt << "        \"time_commitment\": \"<FTE / hours per week>\",\n";
    prompt << "        \"justification\": \"<why this role is needed>\"\n";
    prompt << "      },\n";
    prompt << "      \"...\"\n";
    prompt << "    ],\n";
    prompt << "    \"equipment\": [\n";
    prompt << "      {\n";
    prompt << "        \"item\": \"<equipment>\",\n";
    prompt << "        \"specifications\": \"<specs>\",\n";
    prompt << "        \"cost_estimate\": \"<estimated cost>\",\n";
    prompt << "        \"justification\": \"<why needed>\"\n";
    prompt << "      },\n";
    prompt << "      \"...\"\n";
    prompt << "    ],\n";
    prompt << "    \"budget_estimate\": {\n";
    prompt << "      \"personnel\": \"<amount>\",\n";
    prompt << "      \"equipment\": \"<amount>\",\n";
    prompt << "      \"materials\": \"<amount>\",\n";
    prompt << "      \"travel\": \"<amount>\",\n";
    prompt << "      \"other\": \"<amount>\",\n";
    prompt << "      \"total\": \"<total amount>\"\n";
    prompt << "    }\n";
    prompt << "  },\n\n";

    // 风险评估
    prompt << "  \"risk_assessment\": {\n";
    prompt << "    \"potential_risks\": [\n";
    prompt << "      {\n";
    prompt << "        \"risk\": \"<specific risk>\",\n";
    prompt << "        \"probability\": \"<low / medium / high>\",\n";
    prompt << "        \"impact\": \"<low / medium / high>\",\n";
    prompt << "        \"mitigation_strategies\": [\n";
    prompt << "        \"<strategy 1>\",\n";
    prompt << "        \"<strategy 2>\"\n";
    prompt << "        ]\n";
    prompt << "      },\n";
    prompt << "      \"...\"\n";
    prompt << "    ],\n";
    prompt << "    \"alternative_approaches\": [\n";
    prompt << "      \"<alternative 1>\",\n";
    prompt << "      \"<alternative 2>\"\n";
    prompt << "    ]\n";
    prompt << "  },\n\n";

    // 预期成果
    prompt << "  \"expected_outcomes\": [\n";
    prompt << "    {\n";
    prompt << "      \"outcome\": \"<expected outcome 1>\",\n";
    prompt << "      \"deliverables\": [\n";
    prompt << "        \"<deliverable 1>\",\n";
    prompt << "        \"<deliverable 2>\"\n";
    prompt << "      ],\n";
    prompt << "      \"metrics\": [\n";
    prompt << "        \"<metric 1>\",\n";
    prompt << "        \"<metric 2>\"\n";
    prompt << "      ],\n";
    prompt << "      \"dissemination_plan\": \"<how results will be shared>\"\n";
    prompt << "    },\n";
    prompt << "    \"...\"\n";
    prompt << "  ],\n\n";

    // 评分
    prompt << "  \"feasibility_analysis\": {\n";
    prompt << "    \"feasibility_score\": <integer 1-10>,\n";
    prompt << "    \"innovation_score\": <integer 1-10>,\n";
    prompt << "    \"impact_score\": <integer 1-10>,\n";
    prompt << "    \"overall_assessment\": \"<strengths and concerns>\",\n";
    prompt << "    \"recommendations\": [\n";
    prompt << "      \"<recommendation 1>\",\n";
    prompt << "      \"<recommendation 2>\"\n";
    prompt << "    ]\n";
    prompt << "  }\n";
    prompt << "}\n";
    prompt << "```\n\n";

    // 指导原则
    prompt << "## Guidelines\n\n";
    prompt << "- Be realistic about what can be accomplished in " << context.durationMonths << " months\n";
    prompt << "- Consider the " << context.budgetLevel << " budget level in resource planning\n";
    prompt << "- Ensure objectives are Specific, Measurable, Achievable, Relevant, and Time-bound (SMART)\n";
    prompt << "- Include contingency plans for high-risk activities\n";
    prompt << "- Consider interdisciplinary approaches if relevant\n";
    prompt << "- Ensure alignment with funding agency priorities (if applicable)\n";
    prompt << "- Include plans for reproducibility and open science\n\n";

    return prompt.str();
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string AIPromptTemplates::generateDomainGuidelines(const std::string& researchField) {
    std::ostringstream guidelines;

    if (researchField == "Computer Science" || researchField == "AI" || researchField == "Machine Learning") {
        guidelines << "### Computer Science / AI / ML Guidelines\n\n";
        guidelines << "- Evaluate algorithmic novelty and theoretical contributions\n";
        guidelines << "- Assess experimental design and benchmark comparisons\n";
        guidelines << "- Check for proper baselines and ablation studies\n";
        guidelines << "- Verify reproducibility and code availability\n";
        guidelines << "- Consider computational efficiency and scalability\n";
        guidelines << "- Evaluate real-world applicability\n\n";
    } else if (researchField == "Medicine" || researchField == "Biology" || researchField == "Healthcare") {
        guidelines << "### Medical / Biological Sciences Guidelines\n\n";
        guidelines << "- Evaluate ethical considerations and IRB approval\n";
        guidelines << "- Assess sample size and statistical power\n";
        guidelines << "- Check for proper controls and validation methods\n";
        guidelines << "- Consider clinical significance vs. statistical significance\n";
        guidelines << "- Evaluate patient safety and consent procedures\n";
        guidelines << "- Assess reproducibility and experimental rigor\n\n";
    } else if (researchField == "Physics" || researchField == "Chemistry" || researchField == "Materials Science") {
        guidelines << "### Physical Sciences Guidelines\n\n";
        guidelines << "- Evaluate theoretical foundations and mathematical rigor\n";
        guidelines << "- Assess experimental methodology and measurement accuracy\n";
        guidelines << "- Check for error analysis and uncertainty quantification\n";
        guidelines << "- Consider reproducibility of experiments\n";
        guidelines << "- Evaluate novelty in approach or findings\n";
        guidelines << "- Assess alignment with established physical principles\n\n";
    } else {
        guidelines << "### General Research Guidelines\n\n";
        guidelines << "- Evaluate methodological rigor and validity\n";
        guidelines << "- Assess contribution to the field\n";
        guidelines << "- Check for ethical considerations\n";
        guidelines << "- Consider reproducibility and generalizability\n";
        guidelines << "- Evaluate clarity of presentation\n";
        guidelines << "- Assess significance of findings\n\n";
    }

    return guidelines.str();
}

std::string AIPromptTemplates::replaceVariables(
    const std::string& templateStr,
    const std::map<std::string, std::string>& variables) {

    std::string result = templateStr;

    for (const auto& [key, value] : variables) {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return result;
}

// ============================================================================
// 多语言支持
// ============================================================================

std::string AIPromptTemplates::generateReviewPromptChinese(
    const ReviewPromptContext& context,
    ReviewPromptStyle style) {

    // 中文审稿人Prompt模板
    std::ostringstream prompt;

    prompt << "你是一位**专业审稿人**，为《" << context.targetJournal << "》审阅学术论文。\n\n";
    prompt << "## 审稿任务\n\n";
    prompt << "请对以下论文进行全面、客观、建设性的评审。\n\n";

    prompt << "## 论文信息\n\n";
    prompt << "**标题**: " << context.paperTitle << "\n\n";
    prompt << "**作者**: " << context.paperAuthors << "\n\n";
    prompt << "**摘要**:\n" << context.paperAbstract << "\n\n";

    // 中文审稿标准
    prompt << "## 审稿标准\n\n";
    prompt << "请根据以下标准评审论文：\n\n";
    prompt << "1. **创新性与新颖性** (1-10分)\n";
    prompt << "2. **技术正确性** (1-10分)\n";
    prompt << "3. **表达清晰度** (1-10分)\n";
    prompt << "4. **重要性与影响力** (1-10分)\n";
    prompt << "5. **文献综述** (1-10分)\n\n";

    prompt << "请用**中文**提供详细的审稿意见。\n\n";

    return prompt.str();
}

} // namespace AI
} // namespace PaperCrawler
