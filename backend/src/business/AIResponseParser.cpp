/**
 * AI Response Parser
 * AI响应解析器 - 将AI返回的JSON转换为结构化数据
 *
 * 文件位置: backend/src/business/AIResponseParser.cpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 *
 * 功能：
 * 1. 解析AI审稿人JSON响应
 * 2. 解析文献综述JSON响应
 * 3. 解析研究计划JSON响应
 * 4. 错误处理和容错机制
 * 5. 数据验证和清洗
 */

#include "business/AIResponseParser.hpp"
#include "modules/LoggingModule.hpp"
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <algorithm>

namespace PaperCrawler {

using json = nlohmann::json;

// ============================================================================
// AI Response Parser 实现
// ============================================================================

AIReviewResult AIResponseParser::parseReviewResponse(
    const std::string& jsonResponse,
    int paperId,
    int userId) {

    AIReviewResult result;
    result.success = false;
    result.paperId = paperId;
    result.reviewScore = 0;
    result.acceptanceProbability = 0.0f;
    result.costUsd = 0.0;
    result.reviewedAt = std::chrono::system_clock::now();

    try {
        // 1. 尝试直接解析JSON
        auto parsedJson = json::parse(jsonResponse);

        // 2. 验证JSON结构
        if (!validateReviewJson(parsedJson)) {
            if (auto logging = Services::resolve<LoggingModule>()) {
                logging->warn("Invalid review JSON structure, attempting fallback parsing");
            }
            // 尝试容错解析
            return parseReviewResponseFallback(jsonResponse, paperId, userId);
        }

        // 3. 提取评分
        if (parsedJson.contains("review_score")) {
            result.reviewScore = parsedJson["review_score"].get<int>();
        } else if (parsedJson.contains("overall_score")) {
            result.reviewScore = parsedJson["overall_score"].get<int>();
        } else if (parsedJson.contains("score")) {
            result.reviewScore = parsedJson["score"].get<int>();
        }

        // 验证评分范围
        result.reviewScore = std::clamp(result.reviewScore, 1, 10);

        // 4. 提取录用概率
        if (parsedJson.contains("acceptance_probability")) {
            result.acceptanceProbability = parsedJson["acceptance_probability"].get<float>();
        } else if (parsedJson.contains("acceptance_rate")) {
            result.acceptanceProbability = parsedJson["acceptance_rate"].get<float>();
        }

        // 验证概率范围
        result.acceptanceProbability = std::clamp(result.acceptanceProbability, 0.0f, 1.0f);

        // 5. 提取论文亮点
        if (parsedJson.contains("strengths")) {
            if (parsedJson["strengths"].is_array()) {
                for (const auto& item : parsedJson["strengths"]) {
                    result.strengths.push_back(item.get<std::string>());
                }
            } else if (parsedJson["strengths"].is_string()) {
                result.strengths.push_back(parsedJson["strengths"].get<std::string>());
            }
        }

        // 6. 提取论文弱点
        if (parsedJson.contains("weaknesses")) {
            if (parsedJson["weaknesses"].is_array()) {
                for (const auto& item : parsedJson["weaknesses"]) {
                    result.weaknesses.push_back(item.get<std::string>());
                }
            } else if (parsedJson["weaknesses"].is_string()) {
                result.weaknesses.push_back(parsedJson["weaknesses"].get<std::string>());
            }
        }

        // 7. 提取改进建议
        if (parsedJson.contains("improvements") || parsedJson.contains("improvement_suggestions")) {
            auto improvementsKey = parsedJson.contains("improvements") ? "improvements" : "improvement_suggestions";
            if (parsedJson[improvementsKey].is_array()) {
                for (const auto& item : parsedJson[improvementsKey]) {
                    result.improvements.push_back(item.get<std::string>());
                }
            } else if (parsedJson[improvementsKey].is_string()) {
                result.improvements.push_back(parsedJson[improvementsKey].get<std::string>());
            }
        }

        // 8. 提取对比论文
        if (parsedJson.contains("compared_papers")) {
            if (parsedJson["compared_papers"].is_array()) {
                for (const auto& paper : parsedJson["compared_papers"]) {
                    if (paper.is_object() && paper.contains("title") && paper.contains("reason")) {
                        result.comparedPapers[paper["title"].get<std::string>()] =
                            paper["reason"].get<std::string>();
                    } else if (paper.is_string()) {
                        result.comparedPapers[paper.get<std::string>()] = "Similar work";
                    }
                }
            }
        }

        // 9. 提取详细评论
        if (parsedJson.contains("reviewer_comments")) {
            result.reviewerComments = parsedJson["reviewer_comments"].get<std::string>();
        } else if (parsedJson.contains("comments")) {
            result.reviewerComments = parsedJson["comments"].get<std::string>();
        } else if (parsedJson.contains("detailed_feedback")) {
            result.reviewerComments = parsedJson["detailed_feedback"].get<std::string>();
        }

        // 10. 提取成本信息
        if (parsedJson.contains("cost_usd")) {
            result.costUsd = parsedJson["cost_usd"].get<double>();
        }

        // 11. 提取子评分（如果存在）
        if (parsedJson.contains("methodology_score")) {
            // 可以存储在metadata中
        }
        if (parsedJson.contains("innovation_score")) {
            // 可以存储在metadata中
        }
        if (parsedJson.contains("presentation_score")) {
            // 可以存储在metadata中
        }

        result.success = true;

        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->info("Successfully parsed AI review response for paper " + std::to_string(paperId));
        }

    } catch (const json::parse_error& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("JSON parse error in review response: " + std::string(e.what()));
        }
        // 尝试容错解析
        return parseReviewResponseFallback(jsonResponse, paperId, userId);
    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Error parsing review response: " + std::string(e.what()));
        }
        result.success = false;
    }

    return result;
}

LiteratureReviewResult AIResponseParser::parseLiteratureReviewResponse(
    const std::string& jsonResponse,
    const LiteratureReviewRequest& request) {

    LiteratureReviewResult result;
    result.success = false;
    result.reviewId = 0;
    result.title = request.title;
    result.paperCount = static_cast<int>(request.paperIds.size());
    result.costUsd = 0.0;
    result.generatedAt = std::chrono::system_clock::now();

    try {
        auto parsedJson = json::parse(jsonResponse);

        // 验证JSON结构
        if (!validateLiteratureReviewJson(parsedJson)) {
            return parseLiteratureReviewResponseFallback(jsonResponse, request);
        }

        // 提取综述内容
        if (parsedJson.contains("review_content")) {
            result.reviewContent = parsedJson["review_content"].get<std::string>();
        } else if (parsedJson.contains("content")) {
            result.reviewContent = parsedJson["content"].get<std::string>();
        } else if (parsedJson.contains("review")) {
            result.reviewContent = parsedJson["review"].get<std::string>();
        }

        // 提取研究空白
        if (parsedJson.contains("research_gaps")) {
            if (parsedJson["research_gaps"].is_array()) {
                for (const auto& gap : parsedJson["research_gaps"]) {
                    result.researchGaps.push_back(gap.get<std::string>());
                }
            } else if (parsedJson["research_gaps"].is_string()) {
                result.researchGaps.push_back(parsedJson["research_gaps"].get<std::string>());
            }
        }

        // 提取研究趋势
        if (parsedJson.contains("trends")) {
            if (parsedJson["trends"].is_array()) {
                for (const auto& trend : parsedJson["trends"]) {
                    result.trends.push_back(trend.get<std::string>());
                }
            } else if (parsedJson["trends"].is_string()) {
                result.trends.push_back(parsedJson["trends"].get<std::string>());
            }
        }

        // 提取方法论总结
        if (parsedJson.contains("methodology_summary")) {
            result.methodologySummary = parsedJson["methodology_summary"].get<std::string>();
        }

        // 提取主要发现
        if (parsedJson.contains("key_findings")) {
            if (parsedJson["key_findings"].is_array()) {
                for (const auto& finding : parsedJson["key_findings"]) {
                    result.keyFindings.push_back(finding.get<std::string>());
                }
            }
        }

        // 提取未来方向
        if (parsedJson.contains("future_directions")) {
            if (parsedJson["future_directions"].is_array()) {
                for (const auto& direction : parsedJson["future_directions"]) {
                    result.futureDirections.push_back(direction.get<std::string>());
                }
            }
        }

        // 提取成本
        if (parsedJson.contains("cost_usd")) {
            result.costUsd = parsedJson["cost_usd"].get<double>();
        }

        result.success = true;

        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->info("Successfully parsed literature review response");
        }

    } catch (const json::parse_error& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("JSON parse error in literature review response: " + std::string(e.what()));
        }
        return parseLiteratureReviewResponseFallback(jsonResponse, request);
    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Error parsing literature review response: " + std::string(e.what()));
        }
        result.success = false;
    }

    return result;
}

ResearchPlanResult AIResponseParser::parseResearchPlanResponse(
    const std::string& jsonResponse,
    const ResearchPlanRequest& request) {

    ResearchPlanResult result;
    result.success = false;
    result.planId = 0;
    result.title = request.title;
    result.researchQuestion = request.researchQuestion;
    result.costUsd = 0.0;
    result.plannedAt = std::chrono::system_clock::now();

    try {
        auto parsedJson = json::parse(jsonResponse);

        // 验证JSON结构
        if (!validateResearchPlanJson(parsedJson)) {
            return parseResearchPlanResponseFallback(jsonResponse, request);
        }

        // 提取研究目标
        if (parsedJson.contains("objectives")) {
            if (parsedJson["objectives"].is_array()) {
                for (const auto& objective : parsedJson["objectives"]) {
                    result.objectives.push_back(objective.get<std::string>());
                }
            }
        }

        // 提取方法论
        if (parsedJson.contains("methodology")) {
            result.methodology = parsedJson["methodology"].get<std::string>();
        }

        // 提取时间安排
        if (parsedJson.contains("timeline")) {
            if (parsedJson["timeline"].is_object()) {
                for (auto& [key, value] : parsedJson["timeline"].items()) {
                    result.timeline[key] = value.get<std::string>();
                }
            } else if (parsedJson["timeline"].is_string()) {
                result.timeline["overall"] = parsedJson["timeline"].get<std::string>();
            }
        }

        // 提取所需资源
        if (parsedJson.contains("required_resources")) {
            if (parsedJson["required_resources"].is_array()) {
                for (const auto& resource : parsedJson["required_resources"]) {
                    result.requiredResources.push_back(resource.get<std::string>());
                }
            }
        }

        // 提取潜在挑战
        if (parsedJson.contains("potential_challenges")) {
            if (parsedJson["potential_challenges"].is_array()) {
                for (const auto& challenge : parsedJson["potential_challenges"]) {
                    result.potentialChallenges.push_back(challenge.get<std::string>());
                }
            }
        }

        // 提取预期成果
        if (parsedJson.contains("expected_outcomes")) {
            if (parsedJson["expected_outcomes"].is_array()) {
                for (const auto& outcome : parsedJson["expected_outcomes"]) {
                    result.expectedOutcomes.push_back(outcome.get<std::string>());
                }
            }
        }

        // 提取评分
        if (parsedJson.contains("feasibility_score")) {
            result.feasibilityScore = parsedJson["feasibility_score"].get<int>();
        }
        if (parsedJson.contains("innovation_score")) {
            result.innovationScore = parsedJson["innovation_score"].get<int>();
        }
        if (parsedJson.contains("impact_score")) {
            result.impactScore = parsedJson["impact_score"].get<int>();
        }

        // 提取成本
        if (parsedJson.contains("cost_usd")) {
            result.costUsd = parsedJson["cost_usd"].get<double>();
        }

        result.success = true;

        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->info("Successfully parsed research plan response");
        }

    } catch (const json::parse_error& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("JSON parse error in research plan response: " + std::string(e.what()));
        }
        return parseResearchPlanResponseFallback(jsonResponse, request);
    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Error parsing research plan response: " + std::string(e.what()));
        }
        result.success = false;
    }

    return result;
}

// ============================================================================
// 容错解析方法（Fallback）
// ============================================================================

AIReviewResult AIResponseParser::parseReviewResponseFallback(
    const std::string& response,
    int paperId,
    int userId) {

    AIReviewResult result;
    result.success = false;
    result.paperId = paperId;
    result.reviewScore = 5; // 默认中等评分
    result.acceptanceProbability = 0.5f;
    result.costUsd = 0.0;
    result.reviewedAt = std::chrono::system_clock::now();
    result.reviewerComments = response; // 保存原始响应

    try {
        // 尝试使用正则表达式提取关键信息
        std::string text = response;

        // 提取评分 (支持多种格式)
        std::vector<std::regex> scorePatterns = {
            std::regex(R"(score\s*:?\s*(\d+))", std::regex_constants::icase),
            std::regex(R"(rating\s*:?\s*(\d+))", std::regex_constants::icase),
            std::regex(R"([Ss]core\s*[oO]f\s*(\d+))"),
            std::regex(R"((\d+)/10)"),
            std::regex(R"([Rr]ating\s*:?\s*(\d+))", std::regex_constants::icase)
        };

        for (const auto& pattern : scorePatterns) {
            std::smatch match;
            if (std::regex_search(text, match, pattern)) {
                result.reviewScore = std::stoi(match[1].str());
                result.reviewScore = std::clamp(result.reviewScore, 1, 10);
                break;
            }
        }

        // 提取录用概率
        std::regex probPattern(R"(probability\s*:?\s*([0-9.]+))", std::regex_constants::icase);
        std::smatch probMatch;
        if (std::regex_search(text, probMatch, probPattern)) {
            result.acceptanceProbability = std::stof(probMatch[1].str());
            result.acceptanceProbability = std::clamp(result.acceptanceProbability, 0.0f, 1.0f);
        }

        // 提取strengths部分
        std::regex strengthsPattern(R"(strengths?\s*:?\s*([^.\n]*(?:\.\s*[^.\n]*)*))", std::regex_constants::icase);
        std::smatch strengthsMatch;
        if (std::regex_search(text, strengthsMatch, strengthsPattern)) {
            std::string strengthsText = strengthsMatch[1].str();
            // 分割成多个点
            std::vector<std::string> points = splitText(strengthsText);
            result.strengths = points;
        }

        // 提取weaknesses部分
        std::regex weaknessesPattern(R"(weaknesses?\s*:?\s*([^.\n]*(?:\.\s*[^.\n]*)*))", std::regex_constants::icase);
        std::smatch weaknessesMatch;
        if (std::regex_search(text, weaknessesMatch, weaknessesPattern)) {
            std::string weaknessesText = weaknessesMatch[1].str();
            std::vector<std::string> points = splitText(weaknessesText);
            result.weaknesses = points;
        }

        result.success = true;

        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->info("Successfully parsed review response using fallback method");
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Fallback parsing also failed: " + std::string(e.what()));
        }
        result.success = false;
    }

    return result;
}

LiteratureReviewResult AIResponseParser::parseLiteratureReviewResponseFallback(
    const std::string& response,
    const LiteratureReviewRequest& request) {

    LiteratureReviewResult result;
    result.success = false;
    result.reviewId = 0;
    result.title = request.title;
    result.paperCount = static_cast<int>(request.paperIds.size());
    result.reviewContent = response; // 保存原始响应
    result.costUsd = 0.0;
    result.generatedAt = std::chrono::system_clock::now();

    // 简单的文本分割
    // 在实际应用中，应该使用更复杂的NLP技术
    result.success = true;

    return result;
}

ResearchPlanResult AIResponseParser::parseResearchPlanResponseFallback(
    const std::string& response,
    const ResearchPlanRequest& request) {

    ResearchPlanResult result;
    result.success = false;
    result.planId = 0;
    result.title = request.title;
    result.researchQuestion = request.researchQuestion;
    result.costUsd = 0.0;
    result.plannedAt = std::chrono::system_clock::now();

    // 简单的文本分割
    // 在实际应用中，应该使用更复杂的NLP技术
    result.methodology = response;

    result.success = true;
    return result;
}

// ============================================================================
// 验证方法
// ============================================================================

bool AIResponseParser::validateReviewJson(const json& j) {
    // 检查必需字段
    if (!j.contains("review_score") && !j.contains("overall_score") && !j.contains("score")) {
        return false;
    }

    // 检查数据类型
    if (j.contains("review_score") && !j["review_score"].is_number()) {
        return false;
    }

    return true;
}

bool AIResponseParser::validateLiteratureReviewJson(const json& j) {
    // 至少需要包含综述内容
    return j.contains("review_content") || j.contains("content") || j.contains("review");
}

bool AIResponseParser::validateResearchPlanJson(const json& j) {
    // 至少需要包含研究目标或方法论
    return j.contains("objectives") || j.contains("methodology");
}

// ============================================================================
// 辅助方法
// ============================================================================

std::vector<std::string> AIResponseParser::splitText(const std::string& text) {
    std::vector<std::string> result;

    // 按句子分割（简单实现）
    std::regex sentencePattern(R"([A-Z][^.!?]*[.!?])");
    std::sregex_iterator it(text.begin(), text.end(), sentencePattern);
    std::sregex_iterator end;

    for (; it != end; ++it) {
        std::string sentence = it->str();
        // 去除首尾空白
        sentence.erase(0, sentence.find_first_not_of(" \t\n\r"));
        sentence.erase(sentence.find_last_not_of(" \t\n\r") + 1);
        if (!sentence.empty()) {
            result.push_back(sentence);
        }
    }

    // 如果没有找到句子，尝试按逗号分割
    if (result.empty()) {
        std::stringstream ss(text);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(0, item.find_first_not_of(" \t\n\r"));
            item.erase(item.find_last_not_of(" \t\n\r") + 1);
            if (!item.empty()) {
                result.push_back(item);
            }
        }
    }

    return result;
}

std::string AIResponseParser::extractSection(const std::string& text, const std::string& sectionName) {
    std::regex pattern(sectionName + R"(\s*:?\s*([^;\n]*))", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return match[1].str();
    }
    return "";
}

} // namespace PaperCrawler
