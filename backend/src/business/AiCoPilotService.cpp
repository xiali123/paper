// AI研究副驾驶服务实现
// 文件位置：backend/src/business/AiCoPilotService.cpp

#include "business/AiApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/CacheModule.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>

namespace PaperCrawler {
namespace Services {

// ============================================================================
// AI审稿人系统
// ============================================================================

/**
 * @brief 审稿反馈
 */
struct ReviewFeedback {
    int paperId;
    std::string title;
    int reviewScore;           // 评分 (0-10)
    double acceptanceProbability;  // 录用概率 (0.0-1.0)
    std::vector<std::string> strengths;     // 优点
    std::vector<std::string> weaknesses;    // 缺点
    std::vector<std::string> suggestions;  // 改进建议
    std::vector<std::string> comparedPapers;  // 对比论文
    std::string targetVenue;        // 目标期刊/会议
    std::string generatedAt;       // 生成时间
};

/**
 * @brief AI审稿人服务
 */
class AiReviewerService {
public:
    AiReviewerService(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<AiApiClient> aiClient,
        std::shared_ptr<ICache> cache)
        : database_(database), aiClient_(aiClient), cache_(cache) {}

    /**
     * @brief 审稿论文
     * @param paperId 论文ID
     * @param targetVenue 目标期刊/会议
     * @return 审稿反馈
     */
    ReviewFeedback reviewPaper(int paperId, const std::string& targetVenue) {
        auto logger = spdlog::get("AiCoPilot");

        // 1. 从数据库获取论文
        auto paper = getPaperFromDatabase(paperId);
        if (!paper) {
            throw Errors::PaperNotFound(paperId);
        }

        // 2. 检查缓存
        std::string cacheKey = "review:" + paperId + ":" + targetVenue;
        auto cached = cache_->get(cacheKey);
        if (cached) {
            logger->info("Review cache hit for paper {}", paperId);
            return deserializeReview(*cached);
        }

        // 3. 调用AI进行审稿
        logger->info("Starting AI review for paper {} to {}", paperId, targetVenue);

        // 构建AI prompt
        std::ostringstream prompt;
        prompt << "You are an expert academic reviewer for " << targetVenue << ".\n\n"
               << "Please review the following paper:\n\n"
               << "Title: " << paper->title << "\n"
               << "Authors: " << paper->authors << "\n"
               << "Abstract: " << paper->abstract << "\n"
               << "Year: " << paper->year << "\n\n"
               << "Provide a comprehensive review including:\n"
               << "1. Overall score (0-10)\n"
               << "2. Acceptance probability (0.0-1.0)\n"
               << "3. Key strengths (3-5 points)\n"
               << "4. Major weaknesses (3-5 points)\n"
               << "5. Specific suggestions for improvement\n"
               << "6. Similar papers for comparison\n\n"
               << "Format your response as JSON.";

        // 调用AI API（带三层缓存）
        auto aiResponse = aiClient_->complete(prompt.str(), true);  // 使用缓存

        // 4. 解析AI响应
        auto feedback = parseReviewResponse(paperId, *paper, targetVenue, aiResponse);

        // 5. 缓存结果（30分钟）
        cache_->set(cacheKey, serializeReview(feedback), 1800);

        logger->info("AI review completed for paper {}", paperId);
        return feedback;
    }

    /**
     * @brief 批量审稿
     */
    std::vector<ReviewFeedback> reviewBatch(
        const std::vector<int>& paperIds,
        const std::string& targetVenue) {

        std::vector<ReviewFeedback> feedbacks;

        for (int paperId : paperIds) {
            try {
                auto feedback = reviewPaper(paperId, targetVenue);
                feedbacks.push_back(feedback);
            } catch (const std::exception& e) {
                spdlog::get("AiCoPilot")->error(
                    "Failed to review paper {}: {}", paperId, e.what()
                );
            }
        }

        return feedbacks;
    }

private:
    std::optional<PaperDto> getPaperFromDatabase(int paperId) {
        std::ostringstream sql;
        sql << "SELECT * FROM papers WHERE id = " << paperId;

        auto results = database_->query(sql.str());
        if (results.empty()) {
            return std::nullopt;
        }

        return PaperDto::fromRow(results[0]);
    }

    ReviewFeedback parseReviewResponse(
        int paperId,
        const PaperDto& paper,
        const std::string& targetVenue,
        const std::string& aiResponse) {

        try {
            auto json = nlohmann::json::parse(aiResponse);

            ReviewFeedback feedback;
            feedback.paperId = paperId;
            feedback.title = paper.title;
            feedback.targetVenue = targetVenue;
            feedback.generatedAt = getCurrentTimestamp();

            feedback.reviewScore = json["score"].get<int>();
            feedback.acceptanceProbability = json["acceptance_probability"].get<double>();

            if (json.contains("strengths")) {
                for (const auto& strength : json["strengths"]) {
                    feedback.strengths.push_back(strength.get<std::string>());
                }
            }

            if (json.contains("weaknesses")) {
                for (const auto& weakness : json["weaknesses"]) {
                    feedback.weaknesses.push_back(weakness.get<std::string>());
                }
            }

            if (json.contains("suggestions")) {
                for (const auto& suggestion : json["suggestions"]) {
                    feedback.suggestions.push_back(suggestion.get<std::string>());
                }
            }

            if (json.contains("compared_papers")) {
                for (const auto& comparedPaper : json["compared_papers"]) {
                    feedback.comparedPapers.push_back(comparedPaper.get<std::string>());
                }
            }

            return feedback;

        } catch (const std::exception& e) {
            spdlog::get("AiCoPilot")->error("Failed to parse review response: {}", e.what());
            throw Errors::InternalError("Failed to parse AI review response");
        }
    }

    std::string serializeReview(const ReviewFeedback& feedback) {
        nlohmann::json json;
        json["paper_id"] = feedback.paperId;
        json["title"] = feedback.title;
        json["review_score"] = feedback.reviewScore;
        json["acceptance_probability"] = feedback.acceptanceProbability;
        json["strengths"] = feedback.strengths;
        json["weaknesses"] = feedback.weaknesses;
        json["suggestions"] = feedback.suggestions;
        json["compared_papers"] = feedback.comparedPapers;
        json["target_venue"] = feedback.targetVenue;
        json["generated_at"] = feedback.generatedAt;
        return json.dump();
    }

    ReviewFeedback deserializeReview(const std::string& data) {
        auto json = nlohmann::json::parse(data);

        ReviewFeedback feedback;
        feedback.paperId = json["paper_id"];
        feedback.title = json["title"];
        feedback.reviewScore = json["review_score"];
        feedback.acceptanceProbability = json["acceptance_probability"];
        feedback.strengths = json["strengths"].get<std::vector<std::string>>();
        feedback.weaknesses = json["weaknesses"].get<std::vector<std::string>>();
        feedback.suggestions = json["suggestions"].get<std::vector<std::string>>();
        feedback.comparedPapers = json["compared_papers"].get<std::vector<std::string>>();
        feedback.targetVenue = json["target_venue"];
        feedback.generatedAt = json["generated_at"];
        return feedback;
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<AiApiClient> aiClient_;
    std::shared_ptr<ICache> cache_;
};

// ============================================================================
// AI文献综述生成器
// ============================================================================

/**
 * @brief 文献综述
 */
struct LiteratureReview {
    int id;
    int userId;
    std::string title;
    std::string researchField;
    int paperCount;
    std::string reviewContent;      // 综述内容
    std::string researchGaps;      // 研究空白
    std::string trends;            // 研究趋势
    std::string generatedAt;
};

/**
 * @brief AI文献综述服务
 */
class LiteratureReviewService {
public:
    LiteratureReviewService(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<AiApiClient> aiClient)
        : database_(database), aiClient_(aiClient) {}

    /**
     * @brief 生成文献综述
     * @param userId 用户ID
     * @param researchField 研究领域
     * @param paperIds 论文ID列表
     * @return 文献综述
     */
    LiteratureReview generateReview(
        int userId,
        const std::string& researchField,
        const std::vector<int>& paperIds) {

        auto logger = spdlog::get("AiCoPilot");

        if (paperIds.size() < 5) {
            throw Errors::ValidationFailed("At least 5 papers required for review");
        }

        if (paperIds.size() > 500) {
            throw Errors::ValidationFailed("Maximum 500 papers supported");
        }

        logger->info("Generating literature review for {} papers in field: {}",
                     paperIds.size(), researchField);

        // 1. 获取所有论文
        auto papers = getPapersFromDatabase(paperIds);

        // 2. 构建AI prompt
        std::ostringstream prompt;
        prompt << "Generate a comprehensive literature review for the field: " << researchField << "\n\n"
               << "Based on the following " << papers.size() << " papers:\n\n";

        for (const auto& paper : papers) {
            prompt << "- " << paper.title << " (" << paper.year << ")\n";
        }

        prompt << "\n\nPlease provide:\n"
               << "1. Introduction to the field\n"
               << "2. Main research themes and methodologies\n"
               << "3. Key findings and contributions\n"
               << "4. Research gaps and future directions\n"
               << "5. Trend analysis\n\n"
               << "Format as a structured academic review (2000-3000 words).";

        // 3. 调用AI生成综述
        auto aiResponse = aiClient_->complete(prompt.str());

        // 4. 解析综述
        LiteratureReview review;
        review.userId = userId;
        review.researchField = researchField;
        review.paperCount = papers.size();
        review.reviewContent = aiResponse;
        review.generatedAt = getCurrentTimestamp();

        // 5. 提取研究空白和趋势
        extractGapsAndTrends(review, aiResponse);

        // 6. 保存到数据库
        saveReviewToDatabase(review);

        logger->info("Literature review generated successfully: {} words", aiResponse.length());
        return review;
    }

private:
    std::vector<PaperDto> getPapersFromDatabase(const std::vector<int>& paperIds) {
        std::ostringstream sql;
        sql << "SELECT * FROM papers WHERE id IN (";
        for (size_t i = 0; i < paperIds.size(); ++i) {
            if (i > 0) sql << ",";
            sql << paperIds[i];
        }
        sql << ")";

        auto results = database_->query(sql.str());

        std::vector<PaperDto> papers;
        for (const auto& row : results) {
            papers.push_back(PaperDto::fromRow(row));
        }

        return papers;
    }

    void extractGapsAndTrends(LiteratureReview& review, const std::string& aiResponse) {
        // 使用AI提取研究空白和趋势
        std::ostringstream prompt;
        prompt << "Based on the following literature review, extract:\n"
               << "1. Key research gaps (3-5 points)\n"
               << "2. Future trends (3-5 points)\n\n"
               << "Review:\n" << aiResponse << "\n\n"
               << "Format as JSON with 'gaps' and 'trends' arrays.";

        auto aiResponse2 = aiClient_->complete(prompt.str());

        try {
            auto json = nlohmann::json::parse(aiResponse2);
            review.researchGaps = json["gaps"].get<std::vector<std::string>>()[0];
            review.trends = json["trends"].get<std::vector<std::string>>()[0];
        } catch (const std::exception& e) {
            spdlog::get("AiCoPilot")->warn("Failed to extract gaps and trends: {}", e.what());
        }
    }

    void saveReviewToDatabase(LiteratureReview& review) {
        std::ostringstream sql;
        sql << "INSERT INTO literature_reviews "
             << "(user_id, title, research_field, paper_count, review_content, "
             << "research_gaps, trends, generated_at) VALUES ("
             << review.userId << ", "
             << "'" << database_->escape(review.title) << "', "
             << "'" << database_->escape(review.researchField) << "', "
             << review.paperCount << ", "
             << "'" << database_->escape(review.reviewContent) << "', "
             << "'" << database_->escape(review.researchGaps) << "', "
             << "'" << database_->escape(review.trends) << "', "
             << "NOW())";

        if (!database_->execute(sql.str())) {
            throw Errors::DatabaseError("Failed to save literature review");
        }

        review.id = database_->getLastInsertId();
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<AiApiClient> aiClient_;
};

// ============================================================================
// AI研究规划助手
// ============================================================================

/**
 * @brief 研究计划
 */
struct ResearchPlan {
    std::string title;
    std::string researchQuestion;
    std::string methodology;
    std::string expectedContribution;
    std::string requiredResources;
    std::string timeline;
    std::string predictedImpact;
};

/**
 * @brief AI研究规划服务
 */
class ResearchPlanningService {
public:
    ResearchPlanningService(std::shared_ptr<AiApiClient> aiClient)
        : aiClient_(aiClient) {}

    /**
     * @brief 生成研究计划
     * @param userInterests 用户研究兴趣
     * @param careerStage 职业阶段（PhD学生、博士后、教授等）
     * @return 研究计划
     */
    ResearchPlan generatePlan(
        const std::vector<std::string>& userInterests,
        const std::string& careerStage) {

        auto logger = spdlog::get("AiCoPilot");

        logger->info("Generating research plan for stage: {}", careerStage);

        // 构建AI prompt
        std::ostringstream prompt;
        prompt << "Generate a detailed research plan for a " << careerStage << ".\n\n"
               << "Research interests:\n";
        for (const auto& interest : userInterests) {
            prompt << "- " << interest << "\n";
        }

        prompt << "\n\nPlease provide:\n"
               << "1. Research title\n"
               << "2. Research question\n"
               << "3. Methodology\n"
               << "4. Expected contribution\n"
               << "5. Required resources\n"
               << "6. Timeline\n"
               << "7. Predicted impact\n\n"
               << "Format as JSON.";

        // 调用AI
        auto aiResponse = aiClient_->complete(prompt.str());

        // 解析响应
        ResearchPlan plan = parsePlanResponse(aiResponse);

        logger->info("Research plan generated: {}", plan.title);
        return plan;
    }

    /**
     * @brief 推荐研究方向
     * @param userProfile 用户资料
     * @return 研究方向列表
     */
    std::vector<std::string> recommendDirections(const std::string& userProfile) {
        std::ostringstream prompt;
        prompt << "Based on the following user profile:\n" << userProfile << "\n\n"
               << "Recommend 5 promising research directions.\n"
               << "For each direction, explain:\n"
               << "1. Why it's promising\n"
               << "2. Key papers to read\n"
               << "3. Potential challenges\n"
               << "4. Expected impact\n\n"
               << "Format as JSON array of objects.";

        auto aiResponse = aiClient_->complete(prompt.str());

        // 解析响应
        auto json = nlohmann::json::parse(aiResponse);
        std::vector<std::string> directions;

        for (const auto& item : json) {
            std::ostringstream ss;
            ss << item["title"].get<std::string>() << "\n"
               << "Why: " << item["why"].get<std::string>() << "\n"
               << "Papers: " << item["papers"].get<std::string>() << "\n"
               << "Challenges: " << item["challenges"].get<std::string>() << "\n"
               << "Impact: " << item["impact"].get<std::string>();
            directions.push_back(ss.str());
        }

        return directions;
    }

private:
    ResearchPlan parsePlanResponse(const std::string& aiResponse) {
        auto json = nlohmann::json::parse(aiResponse);

        ResearchPlan plan;
        plan.title = json["title"];
        plan.researchQuestion = json["research_question"];
        plan.methodology = json["methodology"];
        plan.expectedContribution = json["expected_contribution"];
        plan.requiredResources = json["required_resources"];
        plan.timeline = json["timeline"];
        plan.predictedImpact = json["predicted_impact"];

        return plan;
    }

private:
    std::shared_ptr<AiApiClient> aiClient_;
};

} // namespace Services
} // namespace PaperCrawler
