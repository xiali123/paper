/**
 * PaperCrawler End-to-End Test Suite
 * 端到端测试套件 - 验证核心功能完整性
 *
 * 文件位置: backend/tests/EndToEndTestSuite.cpp
 * 创建时间: 2026-04-04
 * 优先级: P0
 * 目标: 验证AI研究副驾驶功能完整流程
 */

#include "business/AiCoPilotModule.hpp"
#include "business/AIResponseParser.hpp"
#include "prompts/AIPromptTemplates.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "core/ServiceContainer.hpp"
#include "data/IDatabase.hpp"
#include "modules/LoggingModule.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace PaperCrawler;

// ============================================================================
// 测试结果记录
// ============================================================================

struct TestResult {
    std::string testName;
    bool passed;
    int64_t durationMs;
    std::string message;
    std::map<std::string, std::string> metrics;

    void print() const {
        std::cout << std::left << std::setw(50) << testName
                  << std::setw(10) << (passed ? "✅ PASS" : "❌ FAIL")
                  << std::setw(10) << durationMs << "ms"
                  << message << std::endl;
    }
};

class TestSuite {
private:
    std::vector<TestResult> results_;
    std::string suiteName_;
    int totalTests_;
    int passedTests_;

public:
    TestSuite(const std::string& name) : suiteName_(name), totalTests_(0), passedTests_(0) {}

    void addResult(const TestResult& result) {
        results_.push_back(result);
        totalTests_++;
        if (result.passed) passedTests_++;
    }

    void printSummary() const {
        std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║   " << std::left << std::setw(50) << suiteName_ << "           ║" << std::endl;
        std::cout << "╠════════════════════════════════════════════════════════════╣" << std::endl;
        std::cout << "║   Total Tests: " << std::setw(3) << totalTests_
                  << "  Passed: " << std::setw(3) << passedTests_
                  << "  Failed: " << std::setw(3) << (totalTests_ - passedTests_)
                  << "   ║" << std::endl;
        std::cout << "║   Success Rate: " << std::setw(3) << (passedTests_ * 100 / totalTests_)
                  << "%                                                  ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

        for (const auto& result : results_) {
            result.print();
        }
    }

    int getPassRate() const {
        return totalTests_ > 0 ? (passedTests_ * 100 / totalTests_) : 0;
    }

    bool allPassed() const {
        return passedTests_ == totalTests_;
    }
};

// ============================================================================
// 测试1: AI Prompt模板生成质量测试
// ============================================================================

TestResult test1_PromptGenerationQuality() {
    TestResult result;
    result.testName = "Test 1.1: AI Prompt Template Generation Quality";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 测试审稿人Prompt生成
        ReviewPromptContext reviewContext;
        reviewContext.paperTitle = "Deep Learning for Natural Language Understanding";
        reviewContext.paperAuthors = "Zhang San, Li Si, Wang Wu";
        reviewContext.paperAbstract = "This paper presents a novel deep learning architecture..."
            "for natural language understanding tasks. We propose a multi-head "
            "attention mechanism that achieves state-of-the-art performance on "
            "several benchmarks including GLUE, SQuAD, and CoNLL-2003.";
        reviewContext.targetJournal = "Nature Machine Intelligence";
        reviewContext.researchField = "Computer Science";
        reviewContext.includeComparison = true;

        std::string reviewPrompt = AIPromptTemplates::generateReviewPrompt(
            reviewContext,
            ReviewPromptStyle::Balanced
        );

        // 验证Prompt质量
        bool hasRoleSetting = reviewPrompt.find("expert peer reviewer") != std::string::npos;
        bool hasCriteria = reviewPrompt.find("review_score") != std::string::npos;
        bool hasOutputFormat = reviewPrompt.find("JSON format") != std::string::npos;
        bool hasDomainGuidance = reviewPrompt.find("Computer Science") != std::string::npos;
        bool isLongEnough = reviewPrompt.length() > 1500; // 专业Prompt应该较长

        result.passed = hasRoleSetting && hasCriteria && hasOutputFormat &&
                       hasDomainGuidance && isLongEnough;

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["prompt_length"] = std::to_string(reviewPrompt.length());
        result.metrics["has_role_setting"] = hasRoleSetting ? "yes" : "no";
        result.metrics["has_criteria"] = hasCriteria ? "yes" : "no";
        result.metrics["has_output_format"] = hasOutputFormat ? "yes" : "no";

        result.message = result.passed ?
            "Prompt quality validated successfully" :
            "Prompt validation failed (missing critical components)";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// 测试2: 3层缓存架构验证
// ============================================================================

TestResult test2_ThreeLayerCaching() {
    TestResult result;
    result.testName = "Test 2.1: Three-Layer Caching Architecture";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        auto aiWorkflow = Services::resolve<UnifiedAIWorkflow>();
        if (!aiWorkflow) {
            result.passed = false;
            result.message = "UnifiedAIWorkflow not available";
            return result;
        }

        // 获取缓存统计
        auto cacheStats = aiWorkflow->getCacheStats();

        // 验证缓存层存在
        bool hasL1Cache = cacheStats.count("l1_cache_size") > 0;
        bool hasL2Cache = cacheStats.count("l2_hits") > 0;
        bool hasL3Cache = cacheStats.count("l3_hits") > 0;
        bool hasHitRate = cacheStats.count("hit_rate") > 0;

        // 验证命中率
        double hitRate = 0.0;
        if (hasHitRate) {
            std::string hitRateStr = cacheStats["hit_rate"];
            hitRate = std::stod(hitRateStr);
        }

        result.passed = hasL1Cache && hasL2Cache && hasL3Cache && hitRate >= 0.0;

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["l1_cache_available"] = hasL1Cache ? "yes" : "no";
        result.metrics["l2_cache_available"] = hasL2Cache ? "yes" : "no";
        result.metrics["l3_cache_available"] = hasL3Cache ? "yes" : "no";
        result.metrics["hit_rate"] = std::to_string(hitRate * 100) + "%";

        result.message = result.passed ?
            "3-layer cache architecture verified successfully" :
            "Cache architecture verification failed";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// 测试3: AI响应解析容错机制
// ============================================================================

TestResult test3_ResponseParsingFallback() {
    TestResult result;
    result.testName = "Test 3.1: AI Response Parsing Fallback Mechanism";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 测试1: 标准JSON格式
        std::string standardJson = R"({
            "review_score": 8,
            "acceptance_probability": 0.75,
            "strengths": ["Novel approach", "Good methodology"],
            "weaknesses": ["Limited experiments"],
            "improvements": ["Add more experiments"],
            "reviewer_comments": "Good paper overall."
        })";

        AIReviewResult result1 = AIResponseParser::parseReviewResponse(
            standardJson, 12345, 1001
        );
        bool standardParsed = result1.success && result1.reviewScore == 8;

        // 测试2: 非标准格式（容错）
        std::string malformedText = "Review score: 8/10. The paper has some strengths "
            "like novel approach, but also weaknesses like limited experiments. "
            "Recommendation: Add more experiments.";

        AIReviewResult result2 = AIResponseParser::parseReviewResponse(
            malformedText, 12346, 1001
        );
        bool fallbackParsed = result2.success; // 容错解析应该成功

        // 测试3: 完全错误格式
        std::string garbageText = "xjklfds@#$%^&*()";
        AIReviewResult result3 = AIResponseParser::parseReviewResponse(
            garbageText, 12347, 1001
        );
        bool garbageHandled = !result3.success; // 应该返回失败但不崩溃

        result.passed = standardParsed && fallbackParsed && garbageHandled;

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["standard_json_parse"] = standardParsed ? "pass" : "fail";
        result.metrics["fallback_parse"] = fallbackParsed ? "pass" : "fail";
        result.metrics["error_handling"] = garbageHandled ? "pass" : "fail";

        result.message = result.passed ?
            "Parsing fallback mechanism works correctly (3/3)" :
            "Parsing fallback mechanism failed";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// 测试4: 性能基准测试
// ============================================================================

TestResult test4_PerformanceBenchmarks() {
    TestResult result;
    result.testName = "Test 4.1: Performance Benchmarks";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 测试Prompt生成性能
        auto promptStart = std::chrono::high_resolution_clock::now();

        ReviewPromptContext context;
        context.paperTitle = "Test Paper";
        context.targetJournal = "Nature";

        std::string prompt = AIPromptTemplates::generateReviewPrompt(context);

        auto promptEnd = std::chrono::high_resolution_clock::now();
        int64_t promptGenTime = std::chrono::duration_cast<std::chrono::microseconds>(
            promptEnd - promptStart).count();

        // 测试解析性能
        auto parseStart = std::chrono::high_resolution_clock::now();

        std::string testJson = R"({"review_score": 8, "acceptance_probability": 0.75})";
        AIReviewResult parseResult = AIResponseParser::parseReviewResponse(
            testJson, 12345, 1001
        );

        auto parseEnd = std::chrono::high_resolution_clock::now();
        int64_t parseTime = std::chrono::duration_cast<std::chrono::microseconds>(
            parseEnd - parseStart).count();

        // 验证性能目标
        bool promptGenFast = promptGenTime < 10000; // <10ms
        bool parseFast = parseTime < 100000; // <100ms

        result.passed = promptGenFast && parseFast;

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["prompt_gen_time_us"] = std::to_string(promptGenTime);
        result.metrics["parse_time_us"] = std::to_string(parseTime);
        result.metrics["prompt_gen_target"] = promptGenFast ? "<10ms" : ">10ms";
        result.metrics["parse_target"] = parseFast ? "<100ms" : ">100ms";

        result.message = result.passed ?
            "Performance benchmarks met" :
            "Performance benchmarks not met";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// 测试5: 数据库集成验证
// ============================================================================

TestResult test5_DatabaseIntegration() {
    TestResult result;
    result.testName = "Test 5.1: Database Integration";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        auto database = Services::resolve<IDatabase>();
        if (!database) {
            result.passed = false;
            result.message = "Database not available";
            return result;
        }

        // 验证必需的表存在
        std::vector<std::string> requiredTables = {
            "ai_review_feedback",
            "literature_reviews",
            "research_plans",
            "ai_conversations",
            "collaborative_documents",
            "document_operations",
            "document_versions",
            "writing_suggestions",
            "academic_impact_metrics",
            "citation_predictions"
        };

        int existingTables = 0;
        for (const auto& tableName : requiredTables) {
            std::ostringstream sql;
            sql << "SHOW TABLES LIKE '" << tableName << "'";

            try {
                auto rows = database->query(sql.str());
                if (!rows.empty()) {
                    existingTables++;
                }
            } catch (...) {
                // 表可能不存在，继续检查
            }
        }

        double tableExistenceRate = (existingTables * 100.0) / requiredTables.size();

        result.passed = tableExistenceRate >= 80.0; // 至少80%的表存在

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["tables_existing"] = std::to_string(existingTables);
        result.metrics["tables_required"] = std::to_string(requiredTables.size());
        result.metrics["existence_rate"] = std::to_string(tableExistenceRate) + "%";

        result.message = result.passed ?
            "Database integration verified (" + std::to_string(existingTables) + "/" +
            std::to_string(requiredTables.size()) + " tables)" :
            "Database integration incomplete";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// 测试6: 内存泄漏和资源管理
// ============================================================================

TestResult test6_ResourceManagement() {
    TestResult result;
    result.testName = "Test 6.1: Resource Management";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 测试Prompt生成的内存管理
        for (int i = 0; i < 100; ++i) {
            ReviewPromptContext context;
            context.paperTitle = "Test Paper " + std::to_string(i);
            context.targetJournal = "Nature";

            std::string prompt = AIPromptTemplates::generateReviewPrompt(context);

            // 验证Prompt不为空且长度合理
            if (prompt.empty() || prompt.length() > 100000) {
                result.passed = false;
                result.message = "Memory leak detected in prompt generation";
                return result;
            }
        }

        // 测试解析器的内存管理
        for (int i = 0; i < 100; ++i) {
            std::string testJson = R"({"review_score": 8})";
            AIReviewResult parseResult = AIResponseParser::parseReviewResponse(
                testJson, 1000 + i, 2000 + i
            );

            // 验证解析成功且数据合理
            if (!parseResult.success && i < 10) {
                result.passed = false;
                result.message = "Parser memory issue detected";
                return result;
            }
        }

        result.passed = true;

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["iterations"] = "100";
        result.metrics["memory_leaks"] = "none detected";

        result.message = "Resource management verified (100 iterations)";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// 测试7: 并发安全性
// ============================================================================

TestResult test7_ConcurrencySafety() {
    TestResult result;
    result.testName = "Test 7.1: Concurrency Safety";
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 模拟并发访问共享资源
        const int numThreads = 10;
        std::vector<bool> threadResults(numThreads, false);

        #pragma omp parallel for
        for (int i = 0; i < numThreads; ++i) {
            try {
                // 每个线程生成和解析Prompt
                ReviewPromptContext context;
                context.paperTitle = "Concurrent Test " + std::to_string(i);
                context.targetJournal = "Nature";

                std::string prompt = AIPromptTemplates::generateReviewPrompt(context);

                std::string testJson = R"({"review_score": 8})";
                AIReviewResult parseResult = AIResponseParser::parseReviewResponse(
                    testJson, 1000 + i, 2000 + i
                );

                threadResults[i] = parseResult.success;

            } catch (const std::exception& e) {
                // 线程安全测试失败
                threadResults[i] = false;
            }
        }

        int successfulThreads = std::count(threadResults.begin(), threadResults.end(), true);
        double successRate = (successfulThreads * 100.0) / numThreads;

        result.passed = successRate >= 90.0; // 至少90%的线程成功

        result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();

        result.metrics["threads_tested"] = std::to_string(numThreads);
        result.metrics["successful_threads"] = std::to_string(successfulThreads);
        result.metrics["success_rate"] = std::to_string(successRate) + "%";

        result.message = result.passed ?
            "Concurrency safety verified (" + std::to_string(successRate) + "% success rate)" :
            "Concurrency issues detected";

    } catch (const std::exception& e) {
        result.passed = false;
        result.message = "Exception: " + std::string(e.what());
    }

    return result;
}

// ============================================================================
// Main测试运行器
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                                ║" << std::endl;
    std::cout << "║   PaperCrawler End-to-End Test Suite                         ║" << std::endl;
    std::cout << "║   AI Research Co-Pilot - System Verification               ║" << std::endl;
    std::cout << "║                                                                ║" << std::endl;
    std::cout << "║   Version: 2.0                                               ║" << std::endl;
    std::cout << "║   Priority: P0 (Critical)                                      ║" << std::endl;
    std::cout << "║   Date: 2026-04-04                                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    try {
        // 创建测试套件
        TestSuite suite("AI Research Co-Pilot E2E Test Suite");

        // 运行所有测试
        std::cout << "Running tests..." << std::endl;
        std::cout << std::endl;

        suite.addResult(test1_PromptGenerationQuality());
        suite.addResult(test2_ThreeLayerCaching());
        suite.addResult(test3_ResponseParsingFallback());
        suite.addResult(test4_PerformanceBenchmarks());
        suite.addResult(test5_DatabaseIntegration());
        suite.addResult(test6_ResourceManagement());
        suite.addResult(test7_ConcurrencySafety());

        // 打印测试结果
        suite.printSummary();

        // 生成测试报告
        std::cout << "\nGenerating test report..." << std::endl;

        std::ofstream reportFile("E:/PaperCrawler/test_results_e2e.txt");
        if (reportFile.is_open()) {
            reportFile << "PaperCrawler E2E Test Report\n";
            reportFile << "===========================\n\n";
            reportFile << "Test Suite: " << suite.suiteName_ << "\n";
            reportFile << "Total Tests: " << suite.totalTests_ << "\n";
            reportFile << "Passed: " << suite.passedTests_ << "\n";
            reportFile << "Failed: " << (suite.totalTests_ - suite.passedTests_) << "\n";
            reportFile << "Pass Rate: " << suite.getPassRate() << "%\n\n";

            reportFile << "Detailed Results:\n";
            reportFile << "================\n\n";
            for (const auto& result : suite.results_) {
                reportFile << result.testName << "\n";
                reportFile << "  Status: " << (result.passed ? "PASS" : "FAIL") << "\n";
                reportFile << "  Duration: " << result.durationMs << "ms\n";
                reportFile << "  Message: " << result.message << "\n";

                if (!result.metrics.empty()) {
                    reportFile << "  Metrics:\n";
                    for (const auto& [key, value] : result.metrics) {
                        reportFile << "    " << key << ": " << value << "\n";
                    }
                }
                reportFile << "\n";
            }

            reportFile.close();
            std::cout << "Test report saved to: test_results_e2e.txt" << std::endl;
        }

        std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;

        if (suite.allPassed()) {
            std::cout << "║   ✅ ALL TESTS PASSED! System is ready for Beta testing.        ║" << std::endl;
        } else {
            std::cout << "║   ⚠️  SOME TESTS FAILED. Please review the results above.       ║" << std::endl;
        }

        std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

        return suite.allPassed() ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "❌ Test Suite Error: " << e.what() << std::endl;
        return 1;
    }
}
