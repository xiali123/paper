/**
 * AI Prompt Templates - Usage Examples
 * AI提示词模板使用示例
 *
 * 文件位置: backend/prompts/AIPromptTemplatesExamples.cpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 *
 * 本文件展示如何使用AI Prompt模板系统
 */

#include "prompts/AIPromptTemplates.hpp"
#include "business/AIResponseParser.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "modules/LoggingModule.hpp"
#include <iostream>
#include <fstream>

using namespace PaperCrawler;
using namespace AI;

// ============================================================================
// 示例1: AI审稿人Prompt
// ============================================================================

void example1_AIPaperReview() {
    std::cout << "=== Example 1: AI Paper Review ===" << std::endl;

    // 1. 准备审稿上下文
    ReviewPromptContext context;
    context.paperId = "12345";
    context.paperTitle = "Deep Learning for Natural Language Understanding";
    context.paperAuthors = "Zhang San, Li Si, Wang Wu";
    context.paperAbstract = "This paper presents a novel deep learning architecture..."
        "for natural language understanding tasks. We propose a multi-head "
        "attention mechanism that achieves state-of-the-art performance on "
        "several benchmarks including GLUE, SQuAD, and CoNLL-2003.";

    context.paperContent = ""; // 完整论文内容（如果需要）
    context.researchField = "Computer Science";
    context.targetJournal = "Nature Machine Intelligence";
    context.includeComparison = true;
    context.includeMethodology = true;
    context.includeReferences = true;
    context.maxSuggestions = 5;

    // 2. 生成审稿Prompt
    std::string reviewPrompt = AIPromptTemplates::generateReviewPrompt(
        context,
        ReviewPromptStyle::Balanced
    );

    std::cout << "Generated Review Prompt:" << std::endl;
    std::cout << reviewPrompt << std::endl;

    // 3. 调用AI生成审稿意见
    // （假设我们已经初始化了UnifiedAIWorkflow）
    /*
    auto aiWorkflow = Services::resolve<UnifiedAIWorkflow>();
    if (aiWorkflow) {
        RAGContext ragContext;
        // 添加相关论文作为对比
        ragContext.relevantPapers = {"11111", "22222", "33333"};

        AIResult result = aiWorkflow->executeAIRequest(
            reviewPrompt,
            AIModelType::GPT_4,
            ragContext,
            userId
        );

        if (result.success) {
            // 4. 解析AI响应
            AIReviewResult reviewResult = AIResponseParser::parseReviewResponse(
                result.content,
                std::stoi(context.paperId),
                userId
            );

            std::cout << "Review Score: " << reviewResult.reviewScore << "/10" << std::endl;
            std::cout << "Acceptance Probability: " << reviewResult.acceptanceProbability << std::endl;
            std::cout << "Strengths: " << reviewResult.strengths.size() << std::endl;
            std::cout << "Weaknesses: " << reviewResult.weaknesses.size() << std::endl;
            std::cout << "Suggestions: " << reviewResult.improvements.size() << std::endl;
        }
    }
    */

    std::cout << "\n";
}

// ============================================================================
// 示例2: 文献综述Prompt
// ============================================================================

void example2_LiteratureReview() {
    std::cout << "=== Example 2: Literature Review ===" << std::endl;

    // 1. 准备综述上下文
    LiteratureReviewContext context;
    context.userId = "user_001";
    context.title = "Transformers in Natural Language Processing: A Comprehensive Review";
    context.researchField = "Natural Language Processing";
    context.paperIds = {101, 102, 103, 104, 105, 106, 107, 108, 109, 110};

    // 内容选项
    context.maxLength = 5000;
    context.includeGaps = true;
    context.includeTrends = true;
    context.includeMethodology = true;
    context.includeKeyFindings = true;
    context.includeFutureDirections = true;
    context.themeCount = 5;

    // 2. 生成综述Prompt
    std::string reviewPrompt = AIPromptTemplates::generateLiteratureReviewPrompt(context);

    std::cout << "Generated Literature Review Prompt:" << std::endl;
    std::cout << reviewPrompt << std::endl;

    // 3. 调用AI生成文献综述
    /*
    auto aiWorkflow = Services::resolve<UnifiedAIWorkflow>();
    if (aiWorkflow) {
        RAGContext ragContext;
        ragContext.relevantPapers = {"101", "102", "103", "104", "105"};

        AIResult result = aiWorkflow->executeAIRequest(
            reviewPrompt,
            AIModelType::GPT_4,
            ragContext,
            std::stoi(context.userId)
        );

        if (result.success) {
            LiteratureReviewResult reviewResult =
                AIResponseParser::parseLiteratureReviewResponse(result.content, context);

            std::cout << "Title: " << reviewResult.title << std::endl;
            std::cout << "Paper Count: " << reviewResult.paperCount << std::endl;
            std::cout << "Research Gaps: " << reviewResult.researchGaps.size() << std::endl;
            std::cout << "Trends: " << reviewResult.trends.size() << std::endl;
            std::cout << "Key Findings: " << reviewResult.keyFindings.size() << std::endl;
        }
    }
    */

    std::cout << "\n";
}

// ============================================================================
// 示例3: 研究计划Prompt
// ============================================================================

void example3_ResearchPlan() {
    std::cout << "=== Example 3: Research Plan ===" << std::endl;

    // 1. 准备研究计划上下文
    ResearchPlanContext context;
    context.userId = "user_001";
    context.title = "Multi-Modal Deep Learning for Healthcare Diagnosis";
    context.researchQuestion = "How can multi-modal deep learning improve "
                              "early diagnosis of cardiovascular diseases using "
                              "electronic health records, medical imaging, "
                              "and genomic data?";

    context.researchField = "Biomedical Informatics";
    context.keywords = {"deep learning", "multi-modal learning", "healthcare",
                       "cardiovascular disease", "early diagnosis"};

    context.durationMonths = 36; // 3年项目
    context.budgetLevel = "high"; // 高预算项目

    // 内容选项
    context.includeTimeline = true;
    context.includeBudget = true;
    context.includeRisks = true;
    context.includeTeam = true;
    context.includeEthics = true;

    // 2. 生成研究计划Prompt
    std::string planPrompt = AIPromptTemplates::generateResearchPlanPrompt(context);

    std::cout << "Generated Research Plan Prompt:" << std::endl;
    std::cout << planPrompt << std::endl;

    // 3. 调用AI生成研究计划
    /*
    auto aiWorkflow = Services::resolve<UnifiedAIWorkflow>();
    if (aiWorkflow) {
        AIResult result = aiWorkflow->executeAIRequest(
            planPrompt,
            AIModelType::GPT_4,
            std::nullopt, // 研究计划不需要RAG
            std::stoi(context.userId)
        );

        if (result.success) {
            ResearchPlanResult planResult =
                AIResponseParser::parseResearchPlanResponse(result.content, context);

            std::cout << "Title: " << planResult.title << std::endl;
            std::cout << "Objectives: " << planResult.objectives.size() << std::endl;
            std::cout << "Feasibility Score: " << planResult.feasibilityScore << "/10" << std::endl;
            std::cout << "Innovation Score: " << planResult.innovationScore << "/10" << std::endl;
            std::cout << "Impact Score: " << planResult.impactScore << "/10" << std::endl;
        }
    }
    */

    std::cout << "\n";
}

// ============================================================================
// 示例4: 使用PromptBuilder构建自定义Prompt
// ============================================================================

void example4_PromptBuilder() {
    std::cout << "=== Example 4: Custom Prompt Builder ===" << std::endl;

    // 使用Builder模式构建自定义Prompt
    PromptBuilder builder;

    std::string customPrompt = builder
        .addRole("You are an expert research advisor specializing in grant writing.")
        .addTask("Create a research outline for a project on quantum computing applications in cryptography.")
        .addContext("field", "Quantum Computing")
        .addContext("duration", "24 months")
        .addContext("budget", "$500,000")
        .addCriteria({
            "Scientific novelty and significance",
            "Feasibility of proposed approach",
            "Qualification of research team",
            "Adequacy of available resources",
            "Potential impact on the field"
        })
        .addOutputFormat("Provide a structured outline in JSON format.")
        .addInstructions("Be specific, realistic, and align with NSF funding priorities.")
        .build();

    std::cout << "Custom Prompt:" << std::endl;
    std::cout << customPrompt << std::endl;

    std::cout << "\n";
}

// ============================================================================
// 示例5: Prompt验证
// ============================================================================

void example5_PromptValidation() {
    std::cout << "=== Example 5: Prompt Validation ===" << std::endl;

    // 创建一个测试Prompt
    std::string testPrompt = R"(
    You are an expert peer reviewer for Nature Machine Intelligence.

    ## Task
    Review the following paper...

    ## Criteria
    1. Originality
    2. Technical Soundness
    3. Clarity
    4. Significance

    ## Output Format
    Please provide your review in JSON format...
    )";

    // 验证Prompt
    PromptValidationResult validation = PromptValidator::validate(testPrompt);

    std::cout << "Validation Result:" << std::endl;
    std::cout << "Valid: " << (validation.valid ? "Yes" : "No") << std::endl;
    std::cout << "Estimated Tokens: " << validation.estimatedTokens << std::endl;

    if (!validation.errors.empty()) {
        std::cout << "Errors:" << std::endl;
        for (const auto& error : validation.errors) {
            std::cout << "  - " << error << std::endl;
        }
    }

    if (!validation.warnings.empty()) {
        std::cout << "Warnings:" << std::endl;
        for (const auto& warning : validation.warnings) {
            std::cout << "  - " << warning << std::endl;
        }
    }

    // 检查是否超出Token限制
    bool exceedsLimit = PromptValidator::exceedsTokenLimit(testPrompt, 4096);
    std::cout << "Exceeds 4096 token limit: " << (exceedsLimit ? "Yes" : "No") << std::endl;

    std::cout << "\n";
}

// ============================================================================
// 示例6: 中文Prompt生成
// ============================================================================

void example6_ChinesePrompts() {
    std::cout << "=== Example 6: Chinese Prompts ===" << std::endl;

    // 1. 中文审稿人Prompt
    ReviewPromptContext context;
    context.paperTitle = "基于深度学习的中文情感分析研究";
    context.paperAuthors = "张三, 李四, 王五";
    context.paperAbstract = "本文提出了一种新的深度学习架构..."
        "用于中文情感分析任务。我们提出的多头注意力机制"
        "在多个基准测试中取得了最先进的结果。";
    context.targetJournal = "计算机学报";
    context.researchField = "计算机科学";

    std::string chineseReviewPrompt = AIPromptTemplates::generateReviewPromptChinese(
        context,
        ReviewPromptStyle::Balanced
    );

    std::cout << "Chinese Review Prompt:" << std::endl;
    std::cout << chineseReviewPrompt << std::endl;

    std::cout << "\n";
}

// ============================================================================
// 示例7: 批量生成Prompt
// ============================================================================

void example7_BatchPromptGeneration() {
    std::cout << "=== Example 7: Batch Prompt Generation ===" << std::endl;

    // 准备多个论文的审稿任务
    std::vector<ReviewPromptContext> papers = {
        {"001", "Paper A Title", "Author A", "Abstract A...", "", "CS", "Nature", true, true, true, 5},
        {"002", "Paper B Title", "Author B", "Abstract B...", "", "AI", "ICML", true, true, true, 5},
        {"003", "Paper C Title", "Author C", "Abstract C...", "", "ML", "NeurIPS", true, true, true, 5}
    };

    std::vector<std::string> prompts;
    prompts.reserve(papers.size());

    // 批量生成Prompt
    for (const auto& paper : papers) {
        std::string prompt = AIPromptTemplates::generateReviewPrompt(
            paper,
            ReviewPromptStyle::Balanced
        );
        prompts.push_back(prompt);
    }

    std::cout << "Generated " << prompts.size() << " prompts" << std::endl;
    std::cout << "Total characters: " << prompts[0].length() << std::endl;
    std::cout << "Estimated tokens per prompt: " << PromptValidator::estimateTokens(prompts[0]) << std::endl;

    std::cout << "\n";
}

// ============================================================================
// Main函数 - 运行所有示例
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   PaperCrawler AI Prompt Templates - Usage Examples       ║" << std::endl;
    std::cout << "║   Version 2.0                                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    try {
        // 运行所有示例
        example1_AIPaperReview();
        example2_LiteratureReview();
        example3_ResearchPlan();
        example4_PromptBuilder();
        example5_PromptValidation();
        example6_ChinesePrompts();
        example7_BatchPromptGeneration();

        std::cout << "✅ All examples executed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// ============================================================================
// 集成到AiCoPilotModule的示例代码
// ============================================================================

/*
// 在AiCoPilotModule中集成Prompt模板的示例：

AIReviewResult AiCoPilotModule::generateReview(const AIReviewRequest& request) {
    // 1. 准备Prompt上下文
    ReviewPromptContext promptContext;
    promptContext.paperId = std::to_string(request.paperId);
    promptContext.paperTitle = paperData.at("title");
    promptContext.paperAuthors = paperData.at("authors");
    promptContext.paperAbstract = paperData.at("abstract");
    promptContext.targetJournal = request.targetJournal;
    promptContext.researchField = request.researchField;
    promptContext.includeComparison = request.includeComparison;

    // 2. 生成审稿Prompt
    std::string reviewPrompt = AIPromptTemplates::generateReviewPrompt(
        promptContext,
        ReviewPromptStyle::Balanced
    );

    // 3. 调用AI生成审稿意见
    RAGContext ragContext;
    ragContext.relevantPapers = {}; // 从推荐模块获取

    auto aiResult = impl_->aiWorkflow_->executeAIRequest(
        reviewPrompt,
        AIModelType::GPT_4,
        ragContext,
        request.userId
    );

    // 4. 解析AI响应（使用新的解析器）
    if (aiResult.success) {
        AIReviewResult result = AIResponseParser::parseReviewResponse(
            aiResult.content,
            request.paperId,
            request.userId
        );

        // 保存到数据库
        saveReviewToDatabase(result);

        return result;
    }

    // 返回失败结果
    AIReviewResult failureResult;
    failureResult.success = false;
    return failureResult;
}
*/
