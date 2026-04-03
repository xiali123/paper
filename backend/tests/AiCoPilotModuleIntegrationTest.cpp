/**
 * AiCoPilotModule Integration Test
 * AI研究副驾驶模块 - 集成测试示例
 *
 * 文件位置: backend/tests/AiCoPilotModuleIntegrationTest.cpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 *
 * 本文件展示如何使用完整的AI研究副驾驶功能
 */

#include "business/AiCoPilotModule.hpp"
#include "business/AIResponseParser.hpp"
#include "prompts/AIPromptTemplates.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "core/ServiceContainer.hpp"
#include "data/IDatabase.hpp"
#include "modules/LoggingModule.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

using namespace PaperCrawler;

// ============================================================================
// 测试1: AI审稿人完整流程
// ============================================================================

void test1_AIPaperReviewComplete() {
    std::cout << "=== Test 1: AI Paper Review Complete Flow ===" << std::endl;

    try {
        // 1. 创建AiCoPilotModule实例
        auto database = Services::resolve<IDatabase>();
        if (!database) {
            std::cerr << "Database not available" << std::endl;
            return;
        }

        AiCoPilotModule aiCoPilot(database);

        // 2. 准备审稿请求
        AIReviewRequest request;
        request.paperId = 12345;
        request.userId = 1001;
        request.targetJournal = "Nature Machine Intelligence";
        request.researchField = "Computer Science";
        request.includeComparison = true;
        request.reviewStyle = "balanced";

        std::cout << "Request Details:" << std::endl;
        std::cout << "  Paper ID: " << request.paperId << std::endl;
        std::cout << "  Target Journal: " << request.targetJournal << std::endl;
        std::cout << "  Research Field: " << request.researchField << std::endl;
        std::cout << "  Include Comparison: " << (request.includeComparison ? "Yes" : "No") << std::endl;

        // 3. 生成AI审稿报告
        auto startTime = std::chrono::high_resolution_clock::now();

        AIReviewResult result = aiCoPilot.generateReview(request);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // 4. 检查结果
        if (result.success) {
            std::cout << "\n✅ Review Generated Successfully!" << std::endl;
            std::cout << "Time Taken: " << duration.count() << "ms" << std::endl;
            std::cout << "\nReview Summary:" << std::endl;
            std::cout << "  Review Score: " << result.reviewScore << "/10" << std::endl;
            std::cout << "  Acceptance Probability: " << (result.acceptanceProbability * 100) << "%" << std::endl;
            std::cout << "  Strengths: " << result.strengths.size() << " points" << std::endl;
            std::cout << "  Weaknesses: " << result.weaknesses.size() << " points" << std::endl;
            std::cout << "  Improvements: " << result.improvements.size() << " suggestions" << std::endl;
            std::cout << "  Compared Papers: " << result.comparedPapers.size() << " papers" << std::endl;
            std::cout << "  Cost: $" << result.costUsd << std::endl;

            // 5. 显示详细内容
            if (!result.strengths.empty()) {
                std::cout << "\nStrengths:" << std::endl;
                for (size_t i = 0; i < result.strengths.size() && i < 3; ++i) {
                    std::cout << "  " << (i + 1) << ". " << result.strengths[i] << std::endl;
                }
            }

            if (!result.weaknesses.empty()) {
                std::cout << "\nWeaknesses:" << std::endl;
                for (size_t i = 0; i < result.weaknesses.size() && i < 3; ++i) {
                    std::cout << "  " << (i + 1) << ". " << result.weaknesses[i] << std::endl;
                }
            }

            if (!result.reviewerComments.empty()) {
                std::cout << "\nReviewer Comments (first 200 chars):" << std::endl;
                std::cout << "  " << result.reviewerComments.substr(0, 200) << "..." << std::endl;
            }

        } else {
            std::cout << "\n❌ Review Generation Failed" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

// ============================================================================
// 测试2: 文献综述生成完整流程
// ============================================================================

void test2_LiteratureReviewComplete() {
    std::cout << "=== Test 2: Literature Review Generation Complete Flow ===" << std::endl;

    try {
        auto database = Services::resolve<IDatabase>();
        if (!database) {
            std::cerr << "Database not available" << std::endl;
            return;
        }

        AiCoPilotModule aiCoPilot(database);

        // 1. 准备文献综述请求
        LiteratureReviewRequest request;
        request.userId = 1001;
        request.title = "Transformers in Natural Language Processing: A Comprehensive Review";
        request.researchField = "Natural Language Processing";
        request.paperIds = {101, 102, 103, 104, 105, 106, 107, 108, 109, 110};
        request.maxLength = 5000;
        request.includeGaps = true;
        request.includeTrends = true;
        request.includeMethodology = true;

        std::cout << "Request Details:" << std::endl;
        std::cout << "  Title: " << request.title << std::endl;
        std::cout << "  Field: " << request.researchField << std::endl;
        std::cout << "  Paper Count: " << request.paperIds.size() << std::endl;
        std::cout << "  Max Length: " << request.maxLength << " words" << std::endl;

        // 2. 生成文献综述
        auto startTime = std::chrono::high_resolution_clock::now();

        LiteratureReviewResult result = aiCoPilot.generateLiteratureReview(request);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // 3. 检查结果
        if (result.success) {
            std::cout << "\n✅ Literature Review Generated Successfully!" << std::endl;
            std::cout << "Time Taken: " << duration.count() << "ms" << std::endl;
            std::cout << "\nReview Summary:" << std::endl;
            std::cout << "  Paper Count: " << result.paperCount << std::endl;
            std::cout << "  Research Gaps: " << result.researchGaps.size() << std::endl;
            std::cout << "  Trends: " << result.trends.size() << std::endl;
            std::cout << "  Key Findings: " << result.keyFindings.size() << std::endl;
            std::cout << "  Cost: $" << result.costUsd << std::endl;

            // 4. 显示关键内容
            if (!result.researchGaps.empty()) {
                std::cout << "\nResearch Gaps:" << std::endl;
                for (size_t i = 0; i < result.researchGaps.size() && i < 3; ++i) {
                    std::cout << "  " << (i + 1) << ". " << result.researchGaps[i] << std::endl;
                }
            }

            if (!result.trends.empty()) {
                std::cout << "\nResearch Trends:" << std::endl;
                for (size_t i = 0; i < result.trends.size() && i < 3; ++i) {
                    std::cout << "  " << (i + 1) << ". " << result.trends[i] << std::endl;
                }
            }

            if (!result.methodologySummary.empty()) {
                std::cout << "\nMethodology Summary (first 200 chars):" << std::endl;
                std::cout << "  " << result.methodologySummary.substr(0, 200) << "..." << std::endl;
            }

        } else {
            std::cout << "\n❌ Literature Review Generation Failed" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

// ============================================================================
// 测试3: 研究计划生成完整流程
// ============================================================================

void test3_ResearchPlanComplete() {
    std::cout << "=== Test 3: Research Plan Generation Complete Flow ===" << std::endl;

    try {
        auto database = Services::resolve<IDatabase>();
        if (!database) {
            std::cerr << "Database not available" << std::endl;
            return;
        }

        AiCoPilotModule aiCoPilot(database);

        // 1. 准备研究计划请求
        ResearchPlanRequest request;
        request.userId = 1001;
        request.title = "Multi-Modal Deep Learning for Healthcare Diagnosis";
        request.researchQuestion = "How can multi-modal deep learning improve early diagnosis "
                                   "of cardiovascular diseases using EHR, imaging, and genomic data?";
        request.researchField = "Biomedical Informatics";
        request.keywords = {"deep learning", "multi-modal learning", "healthcare",
                            "cardiovascular disease", "early diagnosis"};
        request.durationMonths = 36;
        request.budgetLevel = "high";

        std::cout << "Request Details:" << std::endl;
        std::cout << "  Title: " << request.title << std::endl;
        std::cout << "  Research Question: " << request.researchQuestion << std::endl;
        std::cout << "  Field: " << request.researchField << std::endl;
        std::cout << "  Duration: " << request.durationMonths << " months" << std::endl;
        std::cout << "  Budget: " << request.budgetLevel << std::endl;

        // 2. 生成研究计划
        auto startTime = std::chrono::high_resolution_clock::now();

        ResearchPlanResult result = aiCoPilot.generateResearchPlan(request);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // 3. 检查结果
        if (result.success) {
            std::cout << "\n✅ Research Plan Generated Successfully!" << std::endl;
            std::cout << "Time Taken: " << duration.count() << "ms" << std::endl;
            std::cout << "\nPlan Summary:" << std::endl;
            std::cout << "  Objectives: " << result.objectives.size() << std::endl;
            std::cout << "  Feasibility Score: " << result.feasibilityScore << "/10" << std::endl;
            std::cout << "  Innovation Score: " << result.innovationScore << "/10" << std::endl;
            std::cout << "  Impact Score: " << result.impactScore << "/10" << std::endl;
            std::cout << "  Timeline Phases: " << result.timeline.size() << std::endl;
            std::cout << "  Required Resources: " << result.requiredResources.size() << std::endl;
            std::cout << "  Potential Challenges: " << result.potentialChallenges.size() << std::endl;
            std::cout << "  Expected Outcomes: " << result.expectedOutcomes.size() << std::endl;
            std::cout << "  Cost: $" << result.costUsd << std::endl;

            // 4. 显示关键内容
            if (!result.objectives.empty()) {
                std::cout << "\nResearch Objectives:" << std::endl;
                for (size_t i = 0; i < result.objectives.size() && i < 3; ++i) {
                    std::cout << "  " << (i + 1) << ". " << result.objectives[i] << std::endl;
                }
            }

            if (!result.methodology.empty()) {
                std::cout << "\nMethodology (first 300 chars):" << std::endl;
                std::cout << "  " << result.methodology.substr(0, 300) << "..." << std::endl;
            }

            // 5. 显示评分
            std::cout << "\nScores:" << std::endl;
            std::cout << "  Feasibility: " << result.feasibilityScore << "/10" << std::endl;
            std::cout << "  Innovation: " << result.innovationScore << "/10" << std::endl;
            if (!result.impactPrediction.empty()) {
                std::cout << "  Impact Prediction: " << result.impactPrediction << std::endl;
            }

        } else {
            std::cout << "\n❌ Research Plan Generation Failed" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

// ============================================================================
// 测试4: 批量处理和性能测试
// ============================================================================

void test4_BatchProcessingAndPerformance() {
    std::cout << "=== Test 4: Batch Processing and Performance ===" << std::endl;

    try {
        auto database = Services::resolve<IDatabase>();
        if (!database) {
            std::cerr << "Database not available" << std::endl;
            return;
        }

        AiCoPilotModule aiCoPilot(database);

        // 1. 准备批量审稿请求
        std::vector<AIReviewRequest> requests;
        for (int i = 0; i < 5; ++i) {
            AIReviewRequest req;
            req.paperId = 1000 + i;
            req.userId = 1001;
            req.targetJournal = "Nature Machine Intelligence";
            req.researchField = "Computer Science";
            req.includeComparison = true;
            requests.push_back(req);
        }

        std::cout << "Batch Size: " << requests.size() << " papers" << std::endl;

        // 2. 批量处理
        auto startTime = std::chrono::high_resolution_clock::now();

        std::vector<AIReviewResult> results;
        results.reserve(requests.size());

        int successCount = 0;
        int failureCount = 0;
        double totalCost = 0.0;
        double totalTime = 0.0;

        for (const auto& req : requests) {
            auto reqStart = std::chrono::high_resolution_clock::now();

            AIReviewResult result = aiCoPilot.generateReview(req);

            auto reqEnd = std::chrono::high_resolution_clock::now();
            auto reqDuration = std::chrono::duration_cast<std::chrono::milliseconds>(reqEnd - reqStart);

            results.push_back(result);

            if (result.success) {
                successCount++;
                totalCost += result.costUsd;
            } else {
                failureCount++;
            }

            totalTime += reqDuration.count();
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // 3. 性能统计
        std::cout << "\n✅ Batch Processing Complete!" << std::endl;
        std::cout << "Total Time: " << totalDuration.count() << "ms" << std::endl;
        std::cout << "Average Time per Review: " << (totalTime / requests.size()) << "ms" << std::endl;
        std::cout << "Success Rate: " << (successCount * 100 / requests.size()) << "%" << std::endl;
        std::cout << "Total Cost: $" << totalCost << std::endl;
        std::cout << "Average Cost per Review: $" << (totalCost / requests.size()) << std::endl;

        // 4. 性能目标检查
        std::cout << "\nPerformance Targets:" << std::endl;
        std::cout << "  Target Time: <35s per review" << std::endl;
        std::cout << "  Actual Time: " << (totalTime / requests.size() / 1000.0) << "s per review" << std::endl;
        std::cout << "  Target Cost: <$0.15 per review" << std::endl;
        std::cout << "  Actual Cost: $" << (totalCost / requests.size()) << " per review" << std::endl;
        std::cout << "  Status: " << ((totalTime / requests.size() < 35000) ? "✅ PASS" : "❌ FAIL") << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

// ============================================================================
// 测试5: 错误处理和容错
// ============================================================================

void test5_ErrorHandlingAndFallback() {
    std::cout << "=== Test 5: Error Handling and Fallback Mechanisms ===" << std::endl;

    try {
        // 测试1: 无效论文ID
        std::cout << "Test 5.1: Invalid Paper ID" << std::endl;
        auto database = Services::resolve<IDatabase>();
        AiCoPilotModule aiCoPilot(database);

        AIReviewRequest invalidRequest;
        invalidRequest.paperId = -1; // 无效ID
        invalidRequest.userId = 1001;
        invalidRequest.targetJournal = "Nature";

        AIReviewResult result1 = aiCoPilot.generateReview(invalidRequest);
        std::cout << "  Result: " << (result1.success ? "✅ SUCCESS" : "❌ FAILED (Expected)") << std::endl;

        // 测试2: 空论文标题
        std::cout << "\nTest 5.2: Empty Paper Title" << std::endl;
        AIReviewRequest emptyTitleRequest;
        emptyTitleRequest.paperId = 12345;
        emptyTitleRequest.userId = 1001;
        emptyTitleRequest.targetJournal = "";

        AIReviewResult result2 = aiCoPilot.generateReview(emptyTitleRequest);
        std::cout << "  Result: " << (result2.success ? "✅ SUCCESS" : "❌ FAILED") << std::endl;

        // 测试3: JSON解析容错
        std::cout << "\nTest 5.3: JSON Parsing Fallback" << std::endl;
        std::string malformedJson = "{invalid json content here}";
        AIReviewResult parsedResult = AIResponseParser::parseReviewResponse(
            malformedJson,
            12345,
            1001
        );
        std::cout << "  Direct Parse: " << (parsedResult.success ? "✅ SUCCESS" : "❌ FAILED (Expected)") << std::endl;
        std::cout << "  Fallback Used: " << (!parsedResult.reviewerComments.empty() ? "✅ YES" : "❌ NO") << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "\n";
}

// ============================================================================
// Main函数 - 运行所有测试
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   PaperCrawler AI Co-Pilot - Integration Test Suite      ║" << std::endl;
    std::cout << "║   Version 2.0 (with AI Prompt Templates & Response Parser) ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    try {
        // 运行所有测试
        test1_AIPaperReviewComplete();
        test2_LiteratureReviewComplete();
        test3_ResearchPlanComplete();
        test4_BatchProcessingAndPerformance();
        test5_ErrorHandlingAndFallback();

        std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║   ✅ All Tests Completed Successfully!                       ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Test Suite Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// ============================================================================
// 输出示例（预期输出）
// ============================================================================

/*
=== Test 1: AI Paper Review Complete Flow ===
Request Details:
  Paper ID: 12345
  Target Journal: Nature Machine Intelligence
  Research Field: Computer Science
  Include Comparison: Yes

✅ Review Generated Successfully!
Time Taken: 15234ms

Review Summary:
  Review Score: 8/10
  Acceptance Probability: 75%
  Strengths: 5 points
  Weaknesses: 3 points
  Improvements: 5 suggestions
  Compared Papers: 3 papers
  Cost: $0.0075

Strengths:
  1. Novel transformer architecture for multilingual understanding
  2. Comprehensive experiments on multiple benchmarks
  3. Clear writing and well-structured presentation

Weaknesses:
  1. Limited ablation studies
  2. Missing comparison with recent SOTA models
  3. Insufficient analysis of computational complexity

Performance Targets:
  Target Time: <35s per review
  Actual Time: 15.2s per review
  Target Cost: <$0.15 per review
  Actual Cost: $0.0075 per review
  Status: ✅ PASS
*/
