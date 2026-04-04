#include "business/RecommendationApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "core/Router.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// ============================================================================
// RecommendationApiModule::Impl - 内部实现
// ============================================================================

class RecommendationApiModule::Impl {
public:
    RecommendationConfig config_;
    std::shared_ptr<IDatabase> database_;

    // 统计信息
    uint64_t totalRecommendations_{0};
    uint64_t cacheHits_{0};
    uint64_t cacheMisses_{0};
    std::chrono::system_clock::time_point startTime_;

    // 用户兴趣缓存
    std::unordered_map<int, std::vector<UserInterest>> userInterestsCache_;

    Impl() {
        startTime_ = std::chrono::system_clock::now();
    }

    /**
     * @brief 初始化推荐模块
     */
    bool initialize(const RecommendationConfig& config,
                   std::shared_ptr<IDatabase> database) {
        config_ = config;
        database_ = database;

        std::cout << "[Recommendation] Initializing recommendation module..." << std::endl;
        std::cout << "  Algorithm: ";

        switch (config_.algorithm) {
            case RecommendationAlgorithm::COLLABORATIVE_FILTERING:
                std::cout << "Collaborative Filtering"; break;
            case RecommendationAlgorithm::CONTENT_BASED:
                std::cout << "Content-Based"; break;
            case RecommendationAlgorithm::HYBRID:
                std::cout << "Hybrid"; break;
            case RecommendationAlgorithm::POPULARITY:
                std::cout << "Popularity"; break;
            case RecommendationAlgorithm::SIMILARITY:
                std::cout << "Similarity"; break;
        }

        std::cout << std::endl;
        std::cout << "  Max recommendations: " << config_.maxRecommendations << std::endl;
        std::cout << "  Min similarity: " << config_.minSimilarity << std::endl;

        std::cout << "[Recommendation] Initialization complete" << std::endl;
        return true;
    }

    /**
     * @brief 从数据库获取论文信息
     */
    std::optional<std::map<std::string, std::string>> fetchPaper(int paperId) {
        if (!database_) {
            return std::nullopt;
        }

        std::string sql = "SELECT id, title, authors, abstract, category, keywords, "
                         "citation_count, publication_year "
                         "FROM papers WHERE id = " + std::to_string(paperId);

        auto results = database_->query(sql);
        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }

    /**
     * @brief 获取用户的浏览历史
     */
    std::vector<int> getUserHistory(int userId, int limit = 100) {
        if (!database_) {
            return {};
        }

        std::string sql = "SELECT paper_id FROM user_reading_history "
                         "WHERE user_id = " + std::to_string(userId) + " "
                         "ORDER BY viewed_at DESC LIMIT " + std::to_string(limit);

        auto results = database_->query(sql);
        std::vector<int> history;

        for (const auto& row : results) {
            auto it = row.find("paper_id");
            if (it != row.end()) {
                history.push_back(std::stoi(it->second));
            }
        }

        return history;
    }

    /**
     * @brief 计算余弦相似度
     */
    double cosineSimilarity(const std::map<std::string, int>& vec1,
                           const std::map<std::string, int>& vec2) {
        double dotProduct = 0.0;
        double norm1 = 0.0;
        double norm2 = 0.0;

        for (const auto& [term, count1] : vec1) {
            dotProduct += count1 * vec2.count(term) ? vec2.at(term) : 0;
            norm1 += count1 * count1;
        }

        for (const auto& [term, count2] : vec2) {
            norm2 += count2 * count2;
        }

        if (norm1 == 0.0 || norm2 == 0.0) {
            return 0.0;
        }

        return dotProduct / (std::sqrt(norm1) * std::sqrt(norm2));
    }

    /**
     * @brief 简化版文本向量化
     */
    std::map<std::string, int> tokenizeText(const std::string& text) {
        std::map<std::string, int> tokens;
        std::stringstream ss(text);
        std::string word;

        while (ss >> word) {
            // 简化处理：转小写，移除标点
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());

            if (word.length() > 2) {
                tokens[word]++;
            }
        }

        return tokens;
    }
};

// ============================================================================
// RecommendationApiModule 实现
// ============================================================================

RecommendationApiModule::RecommendationApiModule()
    : RecommendationApiModule(nullptr) {
}

RecommendationApiModule::RecommendationApiModule(std::shared_ptr<IDatabase> database)
    : impl_(std::make_unique<Impl>()) {
    // TODO: 接收database参数并保存到impl_
    // impl_->database_ = database;
}

RecommendationApiModule::~RecommendationApiModule() = default;

void RecommendationApiModule::setConfig(const RecommendationConfig& config) {
    impl_->config_ = config;
}

RecommendationConfig RecommendationApiModule::getConfig() const {
    return impl_->config_;
}

// ============================================================================
// API端点实现
// ============================================================================

std::vector<RecommendationResult> RecommendationApiModule::getRecommendations(
    const RecommendationRequest& request) {

    impl_->totalRecommendations_++;

    std::cout << "[Recommendation] Generating recommendations for user " << request.userId << std::endl;

    std::vector<RecommendationResult> results;

    // 根据配置选择推荐算法
    switch (impl_->config_.algorithm) {
        case RecommendationAlgorithm::COLLABORATIVE_FILTERING:
            results = collaborativeFiltering(request.userId, request.limit, request.excludedPaperIds);
            break;

        case RecommendationAlgorithm::CONTENT_BASED:
            results = contentBasedRecommendation(request.userId, request.limit, request.excludedPaperIds);
            break;

        case RecommendationAlgorithm::HYBRID:
            results = hybridRecommendation(request.userId, request.limit, request.excludedPaperIds);
            break;

        case RecommendationAlgorithm::POPULARITY:
            results = getTrendingPapers(request.limit);
            break;

        case RecommendationAlgorithm::SIMILARITY:
            // 基于用户最近浏览的论文推荐相似论文
            {
                auto history = impl_->getUserHistory(request.userId, 1);
                if (!history.empty()) {
                    results = getSimilarPapers(history[0], request.limit);
                }
            }
            break;
    }

    // 过滤排除的论文
    if (!request.excludedPaperIds.empty()) {
        results.erase(
            std::remove_if(results.begin(), results.end(),
                [&](const RecommendationResult& r) {
                    return std::find(request.excludedPaperIds.begin(),
                                  request.excludedPaperIds.end(),
                                  r.paperId) != request.excludedPaperIds.end();
                }),
            results.end()
        );
    }

    // 限制结果数量
    if (results.size() > static_cast<size_t>(request.limit)) {
        results.resize(request.limit);
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::getSimilarPapers(
    int paperId,
    int limit,
    const std::string& category) {

    std::cout << "[Recommendation] Finding similar papers for paper " << paperId << std::endl;

    // 获取目标论文
    auto targetPaper = impl_->fetchPaper(paperId);
    if (!targetPaper.has_value()) {
        return {};
    }

    // TODO: 实现基于内容的相似度计算
    // 这里返回模拟结果
    std::vector<RecommendationResult> results;

    for (int i = 1; i <= limit; ++i) {
        RecommendationResult result;
        result.paperId = paperId + i;
        result.title = "Similar Paper " + std::to_string(i);
        result.authors = "Author " + std::to_string(i);
        result.score = 0.9 - (i * 0.05);
        result.reason = "基于内容相似度推荐";
        result.algorithm = "content-based";

        results.push_back(result);
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::getTrendingPapers(
    int limit,
    const std::string& timeWindow) {

    std::cout << "[Recommendation] Getting trending papers (time window: " << timeWindow << ")" << std::endl;

    // TODO: 从数据库查询热门论文
    std::vector<RecommendationResult> results;

    for (int i = 1; i <= limit; ++i) {
        RecommendationResult result;
        result.paperId = i;
        result.title = "Trending Paper " + std::to_string(i);
        result.authors = "Popular Author " + std::to_string(i);
        result.score = 1.0 - (i * 0.02);
        result.reason = "高被引热门论文";
        result.algorithm = "popularity";

        results.push_back(result);
    }

    return results;
}

std::string RecommendationApiModule::explainRecommendation(int userId, int paperId) {
    std::cout << "[Recommendation] Explaining recommendation for user " << userId
              << ", paper " << paperId << std::endl;

    // TODO: 实现推荐解释
    std::stringstream explanation;
    explanation << "{";
    explanation << "\"userId\": " << userId << ",";
    explanation << "\"paperId\": " << paperId << ",";
    explanation << "\"reason\": \"基于您的浏览历史和兴趣偏好推荐\",";
    explanation << "\"factors\": [";
    explanation << "\"相似用户喜欢\", ";
    explanation << "\"主题相关\", ";
    explanation << "\"高被引\"";
    explanation << "]";
    explanation << "}";

    return explanation.str();
}

bool RecommendationApiModule::recordFeedback(int userId, int paperId, bool liked, int rating) {
    std::cout << "[Recommendation] Recording feedback: user=" << userId
              << ", paper=" << paperId << ", liked=" << liked << ", rating=" << rating << std::endl;

    // 更新用户兴趣
    updateUserInterests(userId, paperId, liked);

    // TODO: 保存反馈到数据库
    return true;
}

std::map<std::string, double> RecommendationApiModule::getUserProfile(int userId) {
    std::cout << "[Recommendation] Getting user profile for user " << userId << std::endl;

    // 获取用户兴趣
    auto interests = getUserInterests(userId);

    std::map<std::string, double> profile;
    for (const auto& interest : interests) {
        profile[interest.category] = interest.weight;
    }

    return profile;
}

std::map<std::string, std::string> RecommendationApiModule::getStats() {
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - impl_->startTime_);

    std::map<std::string, std::string> stats;
    stats["total_recommendations"] = std::to_string(impl_->totalRecommendations_);
    stats["cache_hits"] = std::to_string(impl_->cacheHits_);
    stats["cache_misses"] = std::to_string(impl_->cacheMisses_);
    stats["cache_hit_rate"] = impl_->totalRecommendations_ > 0 ?
        std::to_string(static_cast<double>(impl_->cacheHits_) / impl_->totalRecommendations_) : "0.0";
    stats["uptime_seconds"] = std::to_string(uptime.count());

    return stats;
}

// ============================================================================
// 推荐算法实现
// ============================================================================

std::vector<RecommendationResult> RecommendationApiModule::collaborativeFiltering(
    int userId,
    int limit,
    const std::vector<int>& excludedIds) {

    std::cout << "[Recommendation] Using collaborative filtering" << std::endl;

    // TODO: 实现协同过滤算法
    // 1. 找到相似用户
    // 2. 推荐相似用户喜欢的论文
    std::vector<RecommendationResult> results;

    // 模拟实现
    for (int i = 1; i <= limit; ++i) {
        RecommendationResult result;
        result.paperId = userId * 100 + i;
        result.title = "CF Recommended Paper " + std::to_string(i);
        result.authors = "Similar users liked this";
        result.score = 0.8 - (i * 0.03);
        result.reason = "与您兴趣相似的用户也喜欢";
        result.algorithm = "collaborative-filtering";

        results.push_back(result);
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::contentBasedRecommendation(
    int userId,
    int limit,
    const std::vector<int>& excludedIds) {

    std::cout << "[Recommendation] Using content-based recommendation" << std::endl;

    // 获取用户浏览历史
    auto history = impl_->getUserHistory(userId, 20);
    if (history.empty()) {
        // 冷启动：返回热门论文
        return getTrendingPapers(limit);
    }

    // TODO: 实现基于内容的推荐
    // 1. 分析用户历史论文的内容
    // 2. 构建用户画像
    // 3. 匹配相似论文
    std::vector<RecommendationResult> results;

    for (int i = 1; i <= limit; ++i) {
        RecommendationResult result;
        result.paperId = history[0] + i;
        result.title = "Content-Based Paper " + std::to_string(i);
        result.authors = "Based on your interests";
        result.score = 0.85 - (i * 0.04);
        result.reason = "与您浏览过的论文主题相似";
        result.algorithm = "content-based";

        results.push_back(result);
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::hybridRecommendation(
    int userId,
    int limit,
    const std::vector<int>& excludedIds) {

    std::cout << "[Recommendation] Using hybrid recommendation" << std::endl;

    // 获取多种推荐结果
    auto cfResults = collaborativeFiltering(userId, limit / 2, excludedIds);
    auto cbResults = contentBasedRecommendation(userId, limit / 2, excludedIds);

    // 合并结果，去重，重新排序
    std::vector<RecommendationResult> hybridResults;
    hybridResults.insert(hybridResults.end(), cfResults.begin(), cfResults.end());
    hybridResults.insert(hybridResults.end(), cbResults.begin(), cbResults.end());

    // 按分数排序
    std::sort(hybridResults.begin(), hybridResults.end(),
        [](const RecommendationResult& a, const RecommendationResult& b) {
            return a.score > b.score;
        });

    // 去重（保留分数高的）
    std::set<int> seen;
    std::vector<RecommendationResult> uniqueResults;

    for (const auto& result : hybridResults) {
        if (seen.find(result.paperId) == seen.end()) {
            seen.insert(result.paperId);
            uniqueResults.push_back(result);
            uniqueResults.back().reason = "混合推荐（协同过滤+内容）";
            uniqueResults.back().algorithm = "hybrid";

            if (uniqueResults.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }

    return uniqueResults;
}

double RecommendationApiModule::calculateSimilarity(int paperId1, int paperId2) {
    auto paper1 = impl_->fetchPaper(paperId1);
    auto paper2 = impl_->fetchPaper(paperId2);

    if (!paper1.has_value() || !paper2.has_value()) {
        return 0.0;
    }

    // 简化相似度计算（基于标题和摘要）
    auto text1 = (*paper1)["title"] + " " + (*paper1)["abstract"];
    auto text2 = (*paper2)["title"] + " " + (*paper2)["abstract"];

    auto tokens1 = impl_->tokenizeText(text1);
    auto tokens2 = impl_->tokenizeText(text2);

    return impl_->cosineSimilarity(tokens1, tokens2);
}

std::vector<UserInterest> RecommendationApiModule::getUserInterests(int userId) {
    // TODO: 从数据库或缓存获取用户兴趣
    std::vector<UserInterest> interests;

    UserInterest interest1;
    interest1.category = "Machine Learning";
    interest1.weight = 0.9;
    interest1.lastUpdated = std::chrono::system_clock::now();
    interests.push_back(interest1);

    UserInterest interest2;
    interest2.category = "Computer Vision";
    interest2.weight = 0.7;
    interest2.lastUpdated = std::chrono::system_clock::now();
    interests.push_back(interest2);

    return interests;
}

void RecommendationApiModule::updateUserInterests(int userId, int paperId, bool liked) {
    // TODO: 实现用户兴趣更新
    // 1. 获取论文的类别
    // 2. 更新用户对该类别的权重
    std::cout << "[Recommendation] Updating user interests for user " << userId << std::endl;
}

void RecommendationApiModule::cacheRecommendations(
    int userId,
    const std::vector<RecommendationResult>& results) {

    // TODO: 实现推荐结果缓存（使用Redis）
}

std::optional<std::vector<RecommendationResult>> RecommendationApiModule::getCachedRecommendations(
    int userId) {

    // TODO: 从Redis缓存获取推荐结果
    return std::nullopt;
}

// ============================================================================
// 路由注册
// ============================================================================

void RecommendationApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix(); // "/api/recommendations"

    spdlog::info("[RecommendationApiModule] Registering routes with prefix: {}", prefix);

    // GET /api/recommendations/papers - 论文推荐
    router.get(prefix + "/papers", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"recommendations\":[],\"count\":0,\"algorithm\":\"hybrid\"}";
        return response;
    });

    // GET /api/recommendations/trending - 热门内容
    router.get(prefix + "/trending", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"trending\":[],\"count\":0}";
        return response;
    });

    // POST /api/recommendations/feedback - 推荐反馈
    router.post(prefix + "/feedback", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"message\":\"Feedback recorded (stub mode)\"}";
        return response;
    });

    // GET /api/recommendations/stats - 推荐统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"total_recommendations\":0,\"user_feedback\":0}";
        return response;
    });

    spdlog::info("[RecommendationApiModule] Registered 4 routes");
}

} // namespace PaperCrawler

// ============================================================================
// 模块导出函数（用于动态加载）
// ============================================================================

extern "C" {

using namespace PaperCrawler;

PAPERCRAWLER_API IModule* createModule() {
    return new RecommendationApiModule();
}

PAPERCRAWLER_API void destroyModule(IModule* module) {
    delete module;
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

} // extern "C"
