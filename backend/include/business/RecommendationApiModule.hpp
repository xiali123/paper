#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <optional>
#include <set>

namespace PaperCrawler {

/**
 * @brief 推荐算法类型
 */
enum class RecommendationAlgorithm {
    COLLABORATIVE_FILTERING,  // 协同过滤
    CONTENT_BASED,            // 基于内容
    HYBRID,                   // 混合推荐
    POPULARITY,               // 热度推荐
    SIMILARITY                // 相似度推荐
};

/**
 * @brief 推荐配置
 */
struct RecommendationConfig {
    RecommendationAlgorithm algorithm{RecommendationAlgorithm::HYBRID};
    int maxRecommendations{20};
    double minSimilarity{0.5};
    bool includeExplanation{true};
    int diversityFactor{3};  // 多样性因子（推荐不同类型的论文）
};

/**
 * @brief 推荐请求
 */
struct RecommendationRequest {
    int userId;
    std::string context{"home"};  // home, profile, search, paper_detail
    std::vector<int> excludedPaperIds;  // 排除的论文ID
    std::string category;  // 限定类别（可选）
    int limit{20};
};

/**
 * @brief 推荐结果
 */
struct RecommendationResult {
    int paperId;
    std::string title;
    std::string authors;
    double score{0.0};  // 推荐分数
    std::string reason;  // 推荐理由
    std::string algorithm;  // 使用的算法
};

/**
 * @brief 用户兴趣向量
 */
struct UserInterest {
    std::string category;
    double weight;
    std::chrono::system_clock::time_point lastUpdated;
};

/**
 * @brief 推荐API模块
 *
 * 功能：
 * 1. 基于用户的个性化推荐
 * 2. 基于论文的相似度推荐
 * 3. 热门论文推荐
 * 4. 混合推荐策略
 * 5. 推荐解释
 */
class RecommendationApiModule : public BusinessModuleBase {
public:
    RecommendationApiModule();
    explicit RecommendationApiModule(std::shared_ptr<IDatabase> database);
    ~RecommendationApiModule() override;

    std::string getName() const override { return "Recommendation"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Paper recommendation system with multiple algorithms";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/recommendations"; }


    /**
     * @brief 设置推荐配置
     */
    void setConfig(const RecommendationConfig& config);

    /**
     * @brief 获取配置
     */
    RecommendationConfig getConfig() const;

    // ========================================================================
    // API端点处理方法
    // ========================================================================

    /**
     * @brief 获取个性化推荐
     * GET /api/recommendations/for-user/:userId
     */
    std::vector<RecommendationResult> getRecommendations(const RecommendationRequest& request);

    /**
     * @brief 获取相似论文
     * GET /api/recommendations/similar/:paperId
     */
    std::vector<RecommendationResult> getSimilarPapers(
        int paperId,
        int limit = 10,
        const std::string& category = ""
    );

    /**
     * @brief 获取热门论文
     * GET /api/recommendations/trending
     */
    std::vector<RecommendationResult> getTrendingPapers(
        int limit = 20,
        const std::string& timeWindow = "7d"  // 1d, 7d, 30d
    );

    /**
     * @brief 获取推荐理由
     * GET /api/recommendations/explain/:userId/:paperId
     */
    std::string explainRecommendation(int userId, int paperId);

    /**
     * @brief 更新用户兴趣
     * POST /api/recommendations/feedback
     */
    bool recordFeedback(int userId, int paperId, bool liked, int rating = 0);

    /**
     * @brief 获取用户兴趣画像
     * GET /api/recommendations/profile/:userId
     */
    std::map<std::string, double> getUserProfile(int userId);

    /**
     * @brief 获取推荐统计
     * GET /api/recommendations/stats
     */
    std::map<std::string, std::string> getStats();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes() override;

    // ========================================================================
    // 推荐算法实现
    // ========================================================================

    /**
     * @brief 协同过滤推荐
     */
    std::vector<RecommendationResult> collaborativeFiltering(
        int userId,
        int limit,
        const std::vector<int>& excludedIds
    );

    /**
     * @brief 基于内容的推荐
     */
    std::vector<RecommendationResult> contentBasedRecommendation(
        int userId,
        int limit,
        const std::vector<int>& excludedIds
    );

    /**
     * @brief 混合推荐
     */
    std::vector<RecommendationResult> hybridRecommendation(
        int userId,
        int limit,
        const std::vector<int>& excludedIds
    );

    /**
     * @brief 计算论文相似度
     */
    double calculateSimilarity(int paperId1, int paperId2);

    /**
     * @brief 获取用户兴趣向量
     */
    std::vector<UserInterest> getUserInterests(int userId);

    /**
     * @brief 更新用户兴趣
     */
    void updateUserInterests(int userId, int paperId, bool liked);

    /**
     * @brief 缓存推荐结果
     */
    void cacheRecommendations(int userId, const std::vector<RecommendationResult>& results);

    /**
     * @brief 从缓存获取推荐
     */
    std::optional<std::vector<RecommendationResult>> getCachedRecommendations(int userId);
};

} // namespace PaperCrawler
