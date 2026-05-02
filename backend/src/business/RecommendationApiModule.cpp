#include "business/RecommendationApiModule.hpp"
#include "data/PreparedStatement.hpp"
#include "data/DatabaseModule.hpp"
#include "data/QueryCache.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "features/ai/VectorStore.hpp"
#include "features/ai/EmbeddingGenerator.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <iostream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

using json = nlohmann::json;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief 简单的SQL字符串转义（防止SQL注入）
 */
static std::string escapeSqlString(const std::string& input) {
    std::string result;
    result.reserve(input.length() * 2);

    for (char c : input) {
        switch (c) {
            case '\'': result.append("\\'"); break;
            case '\"': result.append("\\\""); break;
            case '\\': result.append("\\\\"); break;
            case '\n': result.append("\\n"); break;
            case '\r': result.append("\\r"); break;
            case '\t': result.append("\\t"); break;
            case '\0': result.append("\\0"); break;
            default: result.push_back(c); break;
        }
    }

    return result;
}

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

    // 内存缓存（替代Redis）
    struct CacheEntry {
        std::vector<RecommendationResult> results;
        std::chrono::system_clock::time_point expiresAt;
        uint64_t hitCount{0};
    };
    std::unordered_map<std::string, CacheEntry> inMemoryCache_;
    std::mutex cacheMutex_;

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

        spdlog::info("[Recommendation] Initializing recommendation module...");

        std::string algoName;
        switch (config_.algorithm) {
            case RecommendationAlgorithm::COLLABORATIVE_FILTERING:
                algoName = "Collaborative Filtering"; break;
            case RecommendationAlgorithm::CONTENT_BASED:
                algoName = "Content-Based"; break;
            case RecommendationAlgorithm::HYBRID:
                algoName = "Hybrid"; break;
            case RecommendationAlgorithm::POPULARITY:
                algoName = "Popularity"; break;
            case RecommendationAlgorithm::SIMILARITY:
                algoName = "Similarity"; break;
        }

        spdlog::info("  Algorithm: {}", algoName);
        spdlog::info("  Max recommendations: {}", config_.maxRecommendations);
        spdlog::info("  Min similarity: {}", config_.minSimilarity);

        spdlog::info("[Recommendation] Initialization complete");
        return true;
    }

    /**
     * @brief 从数据库获取论文信息
     */
    std::optional<std::map<std::string, std::string>> fetchPaper(int paperId) {
        if (!database_) {
            return std::nullopt;
        }

        PreparedStatement stmt(database_, "SELECT id, title, authors, abstract, category, keywords, "
                         "citation_count, publication_year FROM papers WHERE id = ?");
        stmt.bind(0, paperId);
        auto results = stmt.query();
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

        PreparedStatement stmt(database_, "SELECT paper_id FROM user_reading_history "
                         "WHERE user_id = ? ORDER BY last_accessed_at DESC LIMIT ?");
        stmt.bind(0, userId);
        stmt.bind(1, limit);
        auto results = stmt.query();
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
     * @brief 计算Jaccard相似度（基于关键词集合）
     */
    double jaccardSimilarity(const std::set<std::string>& set1,
                            const std::set<std::string>& set2) {
        if (set1.empty() || set2.empty()) {
            return 0.0;
        }

        std::set<std::string> intersection;
        std::set_intersection(set1.begin(), set1.end(),
                            set2.begin(), set2.end(),
                            std::inserter(intersection, intersection.begin()));

        std::set<std::string> unionSet;
        std::set_union(set1.begin(), set1.end(),
                      set2.begin(), set2.end(),
                      std::inserter(unionSet, unionSet.begin()));

        return static_cast<double>(intersection.size()) / unionSet.size();
    }

    /**
     * @brief 解析JSON关键词为集合
     */
    std::set<std::string> parseKeywords(const std::string& keywordsJson) {
        std::set<std::string> result;

        try {
            if (keywordsJson.empty()) {
                return result;
            }

            // 尝试解析为JSON数组
            auto j = json::parse(keywordsJson);
            if (j.is_array()) {
                for (const auto& item : j) {
                    if (item.is_string()) {
                        std::string kw = item.get<std::string>();
                        // 转小写，移除空格
                        std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
                        kw.erase(std::remove_if(kw.begin(), kw.end(), ::isspace), kw.end());
                        if (!kw.empty()) {
                            result.insert(kw);
                        }
                    }
                }
            }
        } catch (const json::exception& e) {
            // JSON解析失败，尝试作为逗号分隔的字符串处理
            std::stringstream ss(keywordsJson);
            std::string item;
            while (std::getline(ss, item, ',')) {
                std::transform(item.begin(), item.end(), item.begin(), ::tolower);
                item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());
                if (!item.empty()) {
                    result.insert(item);
                }
            }
        }

        return result;
    }

    /**
     * @brief 简化版文本向量化（用于余弦相似度）
     */
    std::map<std::string, int> tokenizeText(const std::string& text) {
        std::map<std::string, int> tokens;
        std::stringstream ss(text);
        std::string word;

        while (ss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());

            if (word.length() > 2) {
                tokens[word]++;
            }
        }

        return tokens;
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
            dotProduct += count1 * (vec2.count(term) ? vec2.at(term) : 0);
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
     * @brief 从数据库查询相似论文
     */
    std::vector<RecommendationResult> fetchSimilarPapersFromDb(int paperId, int limit) {
        if (!database_) {
            return {};
        }

        std::string sql =
            "SELECT p.id, p.title, p.authors, p.publication, p.year, "
            "p.citation_count, p.abstract, p.keywords, "
            "ps.similarity_score "
            "FROM papers p "
            "JOIN paper_similarity ps ON (ps.paper_id1 = p.id OR ps.paper_id2 = p.id) "
            "WHERE (ps.paper_id1 = ? OR ps.paper_id2 = ?) "
            "AND p.id != ? "
            "AND ps.similarity_score >= ? "
            "ORDER BY ps.similarity_score DESC, p.citation_count DESC "
            "LIMIT ?";

        PreparedStatement stmt(database_, sql);
        stmt.bind(0, paperId);
        stmt.bind(1, paperId);
        stmt.bind(2, paperId);
        stmt.bind(3, config_.minSimilarity);
        stmt.bind(4, limit);
        auto results = stmt.query();
        std::vector<RecommendationResult> recommendations;

        for (const auto& row : results) {
            RecommendationResult r;
            r.paperId = std::stoi(row.at("id"));
            r.title = row.at("title");
            r.authors = row.at("authors");
            r.publication = row.count("publication") ? row.at("publication") : "";
            r.year = row.count("year") ? row.at("year") : "";
            r.score = std::stod(row.at("similarity_score"));
            r.reason = "与您浏览的论文内容相似（相似度: " +
                      std::to_string(r.score).substr(0, 4) + ")";
            r.algorithm = "content-based";
            recommendations.push_back(r);
        }

        return recommendations;
    }

    /**
     * @brief 从数据库查询热门论文
     */
    std::vector<RecommendationResult> fetchTrendingPapersFromDb(int limit, const std::string& timeWindow) {
        if (!database_) {
            return {};
        }

        // 解析时间窗口
        int days = 30; // 默认30天
        if (timeWindow == "week" || timeWindow == "7d") {
            days = 7;
        } else if (timeWindow == "month" || timeWindow == "30d") {
            days = 30;
        } else if (timeWindow == "quarter" || timeWindow == "90d") {
            days = 90;
        } else if (timeWindow == "year" || timeWindow == "365d") {
            days = 365;
        }

        // 尝试调用存储过程
        PreparedStatement stmt(database_, "CALL get_trending_papers(?, ?)");
        stmt.bind(0, limit);
        stmt.bind(1, days);
        auto results = stmt.query();
        std::vector<RecommendationResult> recommendations;

        for (const auto& row : results) {
            RecommendationResult r;
            r.paperId = std::stoi(row.at("paper_id"));
            r.title = row.at("title");
            r.authors = row.at("authors");
            r.publication = row.count("publication") ? row.at("publication") : "";
            r.year = row.count("year") ? row.at("year") : "";
            r.score = std::stod(row.at("score"));
            r.reason = row.at("reason");
            r.algorithm = "popularity";
            recommendations.push_back(r);
        }

        // 如果存储过程不存在，使用降级方案
        if (recommendations.empty()) {
            PreparedStatement fallbackStmt(database_, "SELECT id, title, authors, publication, year, citation_count, abstract, keywords "
                  "FROM papers WHERE citation_count > 0 ORDER BY citation_count DESC LIMIT ?");
            fallbackStmt.bind(0, limit);
            results = fallbackStmt.query();
            recommendations.clear();

            for (const auto& row : results) {
                RecommendationResult r;
                r.paperId = std::stoi(row.at("id"));
                r.title = row.at("title");
                r.authors = row.at("authors");
                r.publication = row.count("publication") ? row.at("publication") : "";
                r.year = row.count("year") ? row.at("year") : "";
                r.score = std::stod(row.at("citation_count")) / 1000.0; // 归一化
                r.reason = "高被引热门论文";
                r.algorithm = "popularity";
                recommendations.push_back(r);
            }
        }

        return recommendations;
    }

    /**
     * @brief 从数据库执行协同过滤推荐
     */
    std::vector<RecommendationResult> fetchCollaborativeFilteringFromDb(int userId, int limit) {
        if (!database_) {
            return {};
        }

        // 尝试调用存储过程
        PreparedStatement stmt(database_, "CALL recommend_by_collaborative_filtering(?, ?)");
        stmt.bind(0, userId);
        stmt.bind(1, limit);
        auto results = stmt.query();
        std::vector<RecommendationResult> recommendations;

        for (const auto& row : results) {
            RecommendationResult r;
            r.paperId = std::stoi(row.at("paper_id"));
            r.title = row.at("title");
            r.authors = row.at("authors");
            r.publication = row.count("publication") ? row.at("publication") : "";
            r.year = row.count("year") ? row.at("year") : "";
            r.score = std::stod(row.at("score"));
            r.reason = row.at("reason");
            r.algorithm = "collaborative-filtering";
            recommendations.push_back(r);
        }

        return recommendations;
    }

    /**
     * @brief 从数据库执行基于内容的推荐
     */
    std::vector<RecommendationResult> fetchContentBasedFromDb(int userId, int limit) {
        if (!database_) {
            return {};
        }

        // 尝试调用存储过程
        PreparedStatement stmt(database_, "CALL recommend_by_content(?, ?)");
        stmt.bind(0, userId);
        stmt.bind(1, limit);
        auto results = stmt.query();
        std::vector<RecommendationResult> recommendations;

        for (const auto& row : results) {
            RecommendationResult r;
            r.paperId = std::stoi(row.at("paper_id"));
            r.title = row.at("title");
            r.authors = row.at("authors");
            r.publication = row.count("publication") ? row.at("publication") : "";
            r.year = row.count("year") ? row.at("year") : "";
            r.score = std::stod(row.at("score"));
            r.reason = row.at("reason");
            r.algorithm = "content-based";
            recommendations.push_back(r);
        }

        return recommendations;
    }

    /**
     * @brief 保存推荐反馈到数据库
     */
    bool saveFeedbackToDb(int userId, int paperId, bool liked, int rating) {
        if (!database_) {
            return false;
        }

        PreparedStatement stmt(database_, "INSERT INTO recommendation_feedback "
                         "(user_id, paper_id, liked, rating, algorithm, created_at) "
                         "VALUES (?, ?, ?, ?, 'hybrid', NOW())");
        stmt.bind(0, userId);
        stmt.bind(1, paperId);
        stmt.bind(2, liked ? 1 : 0);
        stmt.bind(3, rating);
        return stmt.execute();
    }

    /**
     * @brief 从数据库获取用户兴趣
     */
    std::vector<UserInterest> getUserInterestsFromDb(int userId) {
        if (!database_) {
            return {};
        }

        PreparedStatement stmt(database_, "SELECT interest_keyword, weight, last_seen_at "
                         "FROM research_interest_evolution WHERE user_id = ? ORDER BY weight DESC LIMIT 10");
        stmt.bind(0, userId);
        auto results = stmt.query();
        std::vector<UserInterest> interests;

        for (const auto& row : results) {
            UserInterest interest;
            interest.category = row.at("interest_keyword");
            interest.weight = std::stod(row.at("weight"));
            interests.push_back(interest);
        }

        return interests;
    }

    /**
     * @brief 更新用户兴趣到数据库
     */
    bool updateUserInterestsInDb(int userId, int paperId, bool liked) {
        if (!database_) {
            return false;
        }

        // 获取论文的关键词
        PreparedStatement stmt(database_, "SELECT keywords FROM papers WHERE id = ?");
        stmt.bind(0, paperId);
        auto results = stmt.query();

        if (results.empty()) {
            return false;
        }

        std::string keywords = results[0].at("keywords");
        auto keywordSet = parseKeywords(keywords);

        // 更新或插入每个关键词的权重
        for (const auto& keyword : keywordSet) {
            double weightDelta = liked ? 0.2 : 0.05;
            PreparedStatement updateStmt(database_,
                "INSERT INTO research_interest_evolution "
                "(user_id, interest_keyword, weight, first_seen_at, last_seen_at) "
                "VALUES (?, ?, ?, NOW(), NOW()) "
                "ON DUPLICATE KEY UPDATE "
                "weight = weight + ?, last_seen_at = NOW()");
            updateStmt.bind(0, userId);
            updateStmt.bind(1, keyword);
            updateStmt.bind(2, weightDelta);
            updateStmt.bind(3, weightDelta);
            updateStmt.execute();
        }

        return true;
    }

    /**
     * @brief 保存推荐历史
     */
    bool saveRecommendationHistory(int userId, const std::vector<RecommendationResult>& results,
                                   const std::string& algorithm) {
        if (!database_ || results.empty()) {
            return false;
        }

        for (const auto& r : results) {
            PreparedStatement stmt(database_, "INSERT INTO user_recommendation_history "
                             "(user_id, paper_id, algorithm, score, reason, created_at) "
                             "VALUES (?, ?, ?, ?, ?, NOW())");
            stmt.bind(0, userId);
            stmt.bind(1, r.paperId);
            stmt.bind(2, algorithm);
            stmt.bind(3, r.score);
            stmt.bind(4, r.reason);
            stmt.execute();
        }

        return true;
    }

    /**
     * @brief 清理过期缓存
     */
    void cleanExpiredCache() {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto now = std::chrono::system_clock::now();

        for (auto it = inMemoryCache_.begin(); it != inMemoryCache_.end(); ) {
            if (now > it->second.expiresAt) {
                it = inMemoryCache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief 检查并清理过期缓存（定期调用）
     */
    void maybeCleanCache() {
        static auto lastClean = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastClean);

        if (elapsed.count() >= 5) { // 每5分钟清理一次
            cleanExpiredCache();
            lastClean = now;
        }
    }

    /**
     * @brief 基于嵌入向量的推荐（利用VectorStore和EmbeddingGenerator）
     *
     * 算法流程：
     * 1. 获取用户阅读历史
     * 2. 将用户历史论文的标题+摘要拼接，生成用户画像嵌入向量
     * 3. 在VectorStore中搜索与该嵌入最相似的论文
     * 4. 过滤掉已读和排除列表中的论文
     */
    std::vector<RecommendationResult> embeddingBasedRecommendation(
        int userId, int limit, const std::vector<int>& excludedIds) {
        if (!database_) return {};

        // 1. 获取用户阅读历史
        auto history = getUserHistory(userId, 20);
        if (history.empty()) return {};

        // 2. 将历史论文的标题+摘要拼接成用户画像文本
        std::string userProfileText;
        for (int pid : history) {
            auto paper = fetchPaper(pid);
            if (paper) {
                if (paper->count("title")) userProfileText += paper->at("title") + " ";
                if (paper->count("abstract")) userProfileText += paper->at("abstract") + " ";
            }
        }

        if (userProfileText.empty()) return {};

        // 3. 使用EmbeddingGenerator生成用户画像嵌入向量
        EmbeddingGenerator gen;
        gen.setProvider("local");
        auto queryVec = gen.generate(userProfileText);

        if (queryVec.empty()) {
            spdlog::warn("[Recommendation] Failed to generate embedding for user {}", userId);
            return {};
        }

        // 4. 通过VectorStore搜索相似论文
        VectorStore store;
        store.setDatabase(database_);
        auto results = store.search(queryVec, limit * 2, 0.3f);

        // 5. 过滤掉排除列表和已读论文
        std::set<int> excludeSet(excludedIds.begin(), excludedIds.end());
        excludeSet.insert(history.begin(), history.end());

        std::vector<RecommendationResult> recommendations;
        for (const auto& sr : results) {
            int paperId = std::stoi(sr.id);
            if (excludeSet.count(paperId)) continue;

            RecommendationResult rr;
            rr.paperId = paperId;
            rr.score = sr.score;
            rr.algorithm = "embedding";
            rr.reason = "Similar to your reading interests (embedding similarity: " +
                        std::to_string(static_cast<int>(sr.score * 100)) + "%)";

            // 获取论文详细信息
            auto paper = fetchPaper(paperId);
            if (paper) {
                rr.title = paper->count("title") ? paper->at("title") : "";
                rr.authors = paper->count("authors") ? paper->at("authors") : "";
            }

            recommendations.push_back(rr);
            if (static_cast<int>(recommendations.size()) >= limit) break;
        }

        spdlog::info("[Recommendation] Embedding-based recommendation for user {}: {} results",
                     userId, recommendations.size());
        return recommendations;
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
    impl_->database_ = database;
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

    spdlog::info("[Recommendation] Generating recommendations for user {}", request.userId);

    // 定期清理缓存
    impl_->maybeCleanCache();

    // 检查缓存
    std::string cacheKey = "user_" + std::to_string(request.userId) +
                          "_algo_" + std::to_string(static_cast<int>(impl_->config_.algorithm)) +
                          "_limit_" + std::to_string(request.limit);

    auto cached = getCachedRecommendations(cacheKey);
    if (cached.has_value()) {
        impl_->cacheHits_++;
        spdlog::info("[Recommendation] Cache hit for user {}", request.userId);
        auto results = *cached;

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

        if (results.size() > static_cast<size_t>(request.limit)) {
            results.resize(request.limit);
        }

        return results;
    }

    impl_->cacheMisses_++;

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

    // 保存到缓存
    cacheRecommendations(cacheKey, results);

    // 保存推荐历史
    std::string algoName;
    switch (impl_->config_.algorithm) {
        case RecommendationAlgorithm::COLLABORATIVE_FILTERING: algoName = "collaborative-filtering"; break;
        case RecommendationAlgorithm::CONTENT_BASED: algoName = "content-based"; break;
        case RecommendationAlgorithm::HYBRID: algoName = "hybrid"; break;
        case RecommendationAlgorithm::POPULARITY: algoName = "popularity"; break;
        case RecommendationAlgorithm::SIMILARITY: algoName = "similarity"; break;
    }
    impl_->saveRecommendationHistory(request.userId, results, algoName);

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::getSimilarPapers(
    int paperId,
    int limit,
    const std::string& category) {

    spdlog::info("[Recommendation] Finding similar papers for paper {}", paperId);

    // 首先尝试从数据库查询预计算的相似度
    auto dbResults = impl_->fetchSimilarPapersFromDb(paperId, limit);
    if (!dbResults.empty()) {
        return dbResults;
    }

    // 降级方案：实时计算相似度
    auto targetPaper = impl_->fetchPaper(paperId);
    if (!targetPaper.has_value()) {
        return {};
    }

    std::string targetKeywords = (*targetPaper).count("keywords") ?
        (*targetPaper).at("keywords") : "";
    auto targetKeywordSet = impl_->parseKeywords(targetKeywords);

    // 获取其他论文
    std::string sql = "SELECT id, title, authors, abstract, keywords, "
                     "citation_count, publication, year "
                     "FROM papers WHERE id != ?";
    if (!category.empty()) {
        sql += " AND category = ?";
    }
    sql += " LIMIT 500";

    PreparedStatement stmt(impl_->database_, sql);
    stmt.bind(0, paperId);
    if (!category.empty()) {
        stmt.bind(1, category);
    }
    auto allPapers = stmt.query();

    // 计算相似度并排序
    std::vector<std::pair<double, std::map<std::string, std::string>>> scoredPapers;

    for (const auto& paper : allPapers) {
        std::string keywords = paper.count("keywords") ? paper.at("keywords") : "";
        auto keywordSet = impl_->parseKeywords(keywords);

        double score = impl_->jaccardSimilarity(targetKeywordSet, keywordSet);

        if (score >= impl_->config_.minSimilarity) {
            scoredPapers.push_back({score, paper});
        }
    }

    // 按相似度排序
    std::sort(scoredPapers.begin(), scoredPapers.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    // 构建结果
    std::vector<RecommendationResult> results;
    for (size_t i = 0; i < std::min(scoredPapers.size(), static_cast<size_t>(limit)); ++i) {
        const auto& [score, paper] = scoredPapers[i];

        RecommendationResult r;
        r.paperId = std::stoi(paper.at("id"));
        r.title = paper.at("title");
        r.authors = paper.at("authors");
        r.publication = paper.count("publication") ? paper.at("publication") : "";
        r.year = paper.count("year") ? paper.at("year") : "";
        r.score = score;
        r.reason = "基于关键词相似度推荐（相似度: " + std::to_string(score).substr(0, 4) + ")";
        r.algorithm = "content-based";

        results.push_back(r);
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::getTrendingPapers(
    int limit,
    const std::string& timeWindow) {

    spdlog::info("[Recommendation] Getting trending papers (time window: {})", timeWindow);

    return impl_->fetchTrendingPapersFromDb(limit, timeWindow);
}

std::string RecommendationApiModule::explainRecommendation(int userId, int paperId) {
    spdlog::info("[Recommendation] Explaining recommendation for user {}, paper {}", userId, paperId);

    json explanation;
    explanation["userId"] = userId;
    explanation["paperId"] = paperId;

    // 获取用户兴趣
    auto interests = getUserInterests(userId);
    json interestsJson = json::array();
    for (const auto& interest : interests) {
        interestsJson.push_back({
            {"category", interest.category},
            {"weight", interest.weight}
        });
    }
    explanation["userInterests"] = interestsJson;

    // 获取论文信息
    auto paper = impl_->fetchPaper(paperId);
    if (paper.has_value()) {
        explanation["paperTitle"] = (*paper).at("title");
        explanation["paperAuthors"] = (*paper).at("authors");

        std::string keywords = (*paper).count("keywords") ? (*paper).at("keywords") : "";
        auto keywordSet = impl_->parseKeywords(keywords);
        explanation["paperKeywords"] = keywordSet;

        // 计算与用户兴趣的匹配度
        json factors = json::array();

        // 1. 检查论文关键词是否匹配用户兴趣
        for (const auto& interest : interests) {
            std::string interestLower = interest.category;
            std::transform(interestLower.begin(), interestLower.end(), interestLower.begin(), ::tolower);

            for (const auto& keyword : keywordSet) {
                if (keyword.find(interestLower) != std::string::npos ||
                    interestLower.find(keyword) != std::string::npos) {
                    factors.push_back({
                        {"type", "interest_match"},
                        {"description", "匹配您的兴趣领域: " + interest.category},
                        {"weight", interest.weight}
                    });
                    break;
                }
            }
        }

        // 2. 检查用户浏览历史中是否有相似论文
        auto history = impl_->getUserHistory(userId, 10);
        int similarCount = 0;
        for (int histPaperId : history) {
            double sim = calculateSimilarity(paperId, histPaperId);
            if (sim > 0.3) {
                similarCount++;
            }
        }
        if (similarCount > 0) {
            factors.push_back({
                {"type", "history_match"},
                {"description", "与您浏览过的 " + std::to_string(similarCount) + " 篇论文相似"},
                {"weight", similarCount * 0.2}
            });
        }

        // 3. 检查引用数（热门度）
        if (paper->count("citation_count")) {
            int citationCount = std::stoi(paper->at("citation_count"));
            if (citationCount > 100) {
                factors.push_back({
                    {"type", "popularity"},
                    {"description", "高被引论文 (" + std::to_string(citationCount) + " 次引用)"},
                    {"weight", std::min(1.0, citationCount / 1000.0)}
                });
            }
        }

        explanation["factors"] = factors;

        // 计算综合推荐分数
        double totalWeight = 0.0;
        for (const auto& factor : factors) {
            totalWeight += factor["weight"].get<double>();
        }
        explanation["score"] = std::min(1.0, totalWeight);
    } else {
        explanation["error"] = "Paper not found";
    }

    return explanation.dump();
}

bool RecommendationApiModule::recordFeedback(int userId, int paperId, bool liked, int rating) {
    spdlog::info("[Recommendation] Recording feedback: user={}, paper={}, liked={}, rating={}", userId, paperId, liked, rating);

    // 保存反馈到数据库
    bool saved = impl_->saveFeedbackToDb(userId, paperId, liked, rating);

    // 更新用户兴趣
    updateUserInterests(userId, paperId, liked);

    return saved;
}

std::map<std::string, double> RecommendationApiModule::getUserProfile(int userId) {
    spdlog::info("[Recommendation] Getting user profile for user {}", userId);

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
    stats["cached_entries"] = std::to_string(impl_->inMemoryCache_.size());

    return stats;
}

// ============================================================================
// 推荐算法实现
// ============================================================================

std::vector<RecommendationResult> RecommendationApiModule::collaborativeFiltering(
    int userId,
    int limit,
    const std::vector<int>& excludedIds) {

    spdlog::info("[Recommendation] Using collaborative filtering");

    // 首先尝试从数据库获取
    auto dbResults = impl_->fetchCollaborativeFilteringFromDb(userId, limit);
    if (!dbResults.empty()) {
        return dbResults;
    }

    // 降级方案：简化版协同过滤
    auto userHistory = impl_->getUserHistory(userId, 50);
    if (userHistory.empty()) {
        // 冷启动：返回热门论文
        return getTrendingPapers(limit);
    }

    // 找到相似用户（基于共同阅读的论文）
    std::unordered_map<int, int> similarUsers; // user_id -> common_paper_count

    for (int paperId : userHistory) {
        PreparedStatement stmt(impl_->database_, "SELECT user_id FROM user_reading_history "
                         "WHERE paper_id = ? AND user_id != ?");
        stmt.bind(0, paperId);
        stmt.bind(1, userId);
        auto results = stmt.query();
        for (const auto& row : results) {
            int otherUserId = std::stoi(row.at("user_id"));
            similarUsers[otherUserId]++;
        }
    }

    // 过滤掉共同论文数太少的用户
    std::vector<std::pair<int, int>> sortedSimilarUsers;
    for (const auto& [otherUserId, commonCount] : similarUsers) {
        if (commonCount >= 2) {
            sortedSimilarUsers.push_back({otherUserId, commonCount});
        }
    }

    // 按相似度排序
    std::sort(sortedSimilarUsers.begin(), sortedSimilarUsers.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // 收集相似用户喜欢的论文（排除用户已读的）
    std::unordered_map<int, int> paperScores; // paper_id -> score
    std::unordered_set<int> userReadSet(userHistory.begin(), userHistory.end());
    std::unordered_set<int> excludedSet(excludedIds.begin(), excludedIds.end());

    for (const auto& [otherUserId, similarity] : sortedSimilarUsers) {
        PreparedStatement stmt(impl_->database_, "SELECT paper_id FROM user_reading_history "
                         "WHERE user_id = ?");
        stmt.bind(0, otherUserId);
        auto results = stmt.query();
        for (const auto& row : results) {
            int paperId = std::stoi(row.at("paper_id"));

            // 排除用户已读的和被排除的论文
            if (userReadSet.find(paperId) == userReadSet.end() &&
                excludedSet.find(paperId) == excludedSet.end()) {
                paperScores[paperId] += similarity;
            }
        }
    }

    // 构建推荐结果
    std::vector<std::pair<int, int>> scoredPapers;
    for (const auto& [paperId, score] : paperScores) {
        scoredPapers.push_back({paperId, score});
    }

    std::sort(scoredPapers.begin(), scoredPapers.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<RecommendationResult> results;
    for (size_t i = 0; i < std::min(scoredPapers.size(), static_cast<size_t>(limit)); ++i) {
        auto paper = impl_->fetchPaper(scoredPapers[i].first);
        if (paper.has_value()) {
            RecommendationResult r;
            r.paperId = scoredPapers[i].first;
            r.title = paper->at("title");
            r.authors = paper->at("authors");
            r.publication = paper->count("publication") ? paper->at("publication") : "";
            r.year = paper->count("year") ? paper->at("year") : "";
            r.score = std::min(1.0, scoredPapers[i].second / 10.0);
            r.reason = "与您兴趣相似的用户也喜欢";
            r.algorithm = "collaborative-filtering";
            results.push_back(r);
        }
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::contentBasedRecommendation(
    int userId,
    int limit,
    const std::vector<int>& excludedIds) {

    spdlog::info("[Recommendation] Using content-based recommendation");

    // 首先尝试从数据库获取
    auto dbResults = impl_->fetchContentBasedFromDb(userId, limit);
    if (!dbResults.empty()) {
        return dbResults;
    }

    // 降级方案：简化版内容推荐
    auto history = impl_->getUserHistory(userId, 20);
    if (history.empty()) {
        // 冷启动：返回热门论文
        return getTrendingPapers(limit);
    }

    // 构建用户兴趣画像（基于历史论文的关键词）
    std::map<std::string, double> userInterestProfile;

    for (int paperId : history) {
        auto paper = impl_->fetchPaper(paperId);
        if (paper.has_value() && paper->count("keywords")) {
            auto keywords = impl_->parseKeywords(paper->at("keywords"));
            for (const auto& keyword : keywords) {
                userInterestProfile[keyword] += 1.0;
            }
        }
    }

    // 归一化
    double totalWeight = 0.0;
    for (auto& [keyword, weight] : userInterestProfile) {
        totalWeight += weight;
    }
    if (totalWeight > 0) {
        for (auto& [keyword, weight] : userInterestProfile) {
            weight /= totalWeight;
        }
    }

    // 获取所有论文并计算匹配度
    std::unordered_set<int> userReadSet(history.begin(), history.end());
    std::unordered_set<int> excludedSet(excludedIds.begin(), excludedIds.end());

    std::string sql = "SELECT id, title, authors, keywords, abstract, citation_count, publication, year "
                     "FROM papers LIMIT 500";

    auto allPapers = impl_->database_->query(sql);
    std::vector<std::pair<double, std::map<std::string, std::string>>> scoredPapers;

    for (const auto& paper : allPapers) {
        int paperId = std::stoi(paper.at("id"));

        // 跳过用户已读的
        if (userReadSet.find(paperId) != userReadSet.end() ||
            excludedSet.find(paperId) != excludedSet.end()) {
            continue;
        }

        double score = 0.0;

        // 计算关键词匹配度
        if (paper.count("keywords")) {
            auto keywords = impl_->parseKeywords(paper.at("keywords"));
            for (const auto& keyword : keywords) {
                if (userInterestProfile.count(keyword)) {
                    score += userInterestProfile[keyword];
                }
            }
        }

        if (score > 0) {
            scoredPapers.push_back({score, paper});
        }
    }

    // 按分数排序
    std::sort(scoredPapers.begin(), scoredPapers.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    // 构建结果
    std::vector<RecommendationResult> results;
    for (size_t i = 0; i < std::min(scoredPapers.size(), static_cast<size_t>(limit)); ++i) {
        const auto& [score, paper] = scoredPapers[i];

        RecommendationResult r;
        r.paperId = std::stoi(paper.at("id"));
        r.title = paper.at("title");
        r.authors = paper.at("authors");
        r.publication = paper.count("publication") ? paper.at("publication") : "";
        r.year = paper.count("year") ? paper.at("year") : "";
        r.score = std::min(1.0, score);
        r.reason = "与您浏览过的论文主题相似";
        r.algorithm = "content-based";

        results.push_back(r);
    }

    return results;
}

std::vector<RecommendationResult> RecommendationApiModule::hybridRecommendation(
    int userId,
    int limit,
    const std::vector<int>& excludedIds) {

    spdlog::info("[Recommendation] Using hybrid recommendation");

    // 混合策略：60% 基于内容 + 40% 热门
    int cbLimit = static_cast<int>(limit * 0.6);
    int trendingLimit = limit - cbLimit;

    auto cbResults = contentBasedRecommendation(userId, cbLimit, excludedIds);
    auto trendingResults = getTrendingPapers(trendingLimit);

    // 合并结果
    std::vector<RecommendationResult> hybridResults;

    // 添加基于内容的推荐，权重提升
    for (auto& r : cbResults) {
        r.score *= 1.2; // 稍微提升内容推荐的权重
        r.reason = "混合推荐（内容匹配度: " + std::to_string(r.score).substr(0, 4) + "）";
        r.algorithm = "hybrid";
        hybridResults.push_back(r);
    }

    // 添加热门推荐
    std::unordered_set<int> seenIds;
    for (const auto& r : cbResults) {
        seenIds.insert(r.paperId);
    }

    for (auto& r : trendingResults) {
        if (seenIds.find(r.paperId) == seenIds.end() &&
            std::find(excludedIds.begin(), excludedIds.end(), r.paperId) == excludedIds.end()) {
            r.score *= 0.8; // 稍微降低热门推荐的权重
            r.reason = "混合推荐（热门论文）";
            r.algorithm = "hybrid";
            hybridResults.push_back(r);
            seenIds.insert(r.paperId);

            if (hybridResults.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }

    // 按分数排序
    std::sort(hybridResults.begin(), hybridResults.end(),
        [](const RecommendationResult& a, const RecommendationResult& b) {
            return a.score > b.score;
        });

    // 限制结果数量
    if (hybridResults.size() > static_cast<size_t>(limit)) {
        hybridResults.resize(limit);
    }

    return hybridResults;
}

double RecommendationApiModule::calculateSimilarity(int paperId1, int paperId2) {
    auto paper1 = impl_->fetchPaper(paperId1);
    auto paper2 = impl_->fetchPaper(paperId2);

    if (!paper1.has_value() || !paper2.has_value()) {
        return 0.0;
    }

    // 1. 关键词Jaccard相似度
    std::string keywords1 = paper1->count("keywords") ? paper1->at("keywords") : "";
    std::string keywords2 = paper2->count("keywords") ? paper2->at("keywords") : "";

    auto kwSet1 = impl_->parseKeywords(keywords1);
    auto kwSet2 = impl_->parseKeywords(keywords2);

    double kwSimilarity = impl_->jaccardSimilarity(kwSet1, kwSet2);

    // 2. 摘要余弦相似度
    std::string abstract1 = paper1->count("abstract") ? paper1->at("abstract") : "";
    std::string abstract2 = paper2->count("abstract") ? paper2->at("abstract") : "";

    auto tokens1 = impl_->tokenizeText(abstract1 + " " + paper1->at("title"));
    auto tokens2 = impl_->tokenizeText(abstract2 + " " + paper2->at("title"));

    double cosSimilarity = impl_->cosineSimilarity(tokens1, tokens2);

    // 综合相似度：50% 关键词 + 50% 摘要
    return 0.5 * kwSimilarity + 0.5 * cosSimilarity;
}

std::vector<UserInterest> RecommendationApiModule::getUserInterests(int userId) {
    // 首先尝试从数据库获取
    auto dbInterests = impl_->getUserInterestsFromDb(userId);
    if (!dbInterests.empty()) {
        return dbInterests;
    }

    // 降级方案：从阅读历史中提取兴趣
    auto history = impl_->getUserHistory(userId, 50);
    std::map<std::string, int> keywordCounts;

    for (int paperId : history) {
        auto paper = impl_->fetchPaper(paperId);
        if (paper.has_value() && paper->count("keywords")) {
            auto keywords = impl_->parseKeywords(paper->at("keywords"));
            for (const auto& keyword : keywords) {
                keywordCounts[keyword]++;
            }
        }
    }

    // 转换为UserInterest并按权重排序
    std::vector<UserInterest> interests;
    for (const auto& [keyword, count] : keywordCounts) {
        UserInterest interest;
        interest.category = keyword;
        interest.weight = static_cast<double>(count) / history.size();
        interests.push_back(interest);
    }

    std::sort(interests.begin(), interests.end(),
        [](const UserInterest& a, const UserInterest& b) {
            return a.weight > b.weight;
        });

    // 限制数量
    if (interests.size() > 10) {
        interests.resize(10);
    }

    return interests;
}

void RecommendationApiModule::updateUserInterests(int userId, int paperId, bool liked) {
    impl_->updateUserInterestsInDb(userId, paperId, liked);
}

void RecommendationApiModule::cacheRecommendations(
    int userId,
    const std::vector<RecommendationResult>& results) {
    std::string cacheKey = "user_" + std::to_string(userId);
    cacheRecommendations(cacheKey, results);
}

void RecommendationApiModule::cacheRecommendations(
    const std::string& cacheKey,
    const std::vector<RecommendationResult>& results) {
    std::lock_guard<std::mutex> lock(impl_->cacheMutex_);

    Impl::CacheEntry entry;
    entry.results = results;
    entry.expiresAt = std::chrono::system_clock::now() + std::chrono::minutes(30); // 30分钟TTL
    entry.hitCount = 0;

    impl_->inMemoryCache_[cacheKey] = entry;
}

std::optional<std::vector<RecommendationResult>> RecommendationApiModule::getCachedRecommendations(
    int userId) {
    std::string cacheKey = "user_" + std::to_string(userId);
    return getCachedRecommendations(cacheKey);
}

std::optional<std::vector<RecommendationResult>> RecommendationApiModule::getCachedRecommendations(
    const std::string& cacheKey) {
    std::lock_guard<std::mutex> lock(impl_->cacheMutex_);

    auto it = impl_->inMemoryCache_.find(cacheKey);
    if (it != impl_->inMemoryCache_.end()) {
        auto now = std::chrono::system_clock::now();
        if (now < it->second.expiresAt) {
            it->second.hitCount++;
            return it->second.results;
        } else {
            impl_->inMemoryCache_.erase(it);
        }
    }

    return std::nullopt;
}

// ============================================================================
// 路由注册
// ============================================================================

void RecommendationApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[RecommendationApiModule] Registering routes with prefix: {}", prefix);

    // 接收数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[RecommendationApiModule] ✅ Received injected database connection from ModuleLoader!");
        impl_->database_ = database_;
    }

    // 备用：尝试从全局DatabaseModule获取
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                impl_->database_ = dbPtr;
                spdlog::info("[RecommendationApiModule] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[RecommendationApiModule] Failed to get global database connection: {}", e.what());
        }
    }

    // 备用：MessageBus订阅
    if (!database_) {
        auto& messageBus = MessageBus::getInstance();
        messageBus.registerHandler(MessageType::CUSTOM,
            [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                if (dbMsg && dbMsg->isSuccess()) {
                    impl_->database_ = dbMsg->getConnection();
                    spdlog::info("[RecommendationApi] ✅ Received database connection from MessageBus!");
                }
                auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "RecommendationApi", "DatabaseModule");
                response->setData("acknowledged", true);
                response->setData("moduleName", "RecommendationApi");
                return response;
            },
            "RecommendationApi"
        );
        spdlog::info("[RecommendationApi] Successfully subscribed to database connection messages");
    }

    // GET /api/recommendations/papers - 论文推荐
    router.get(prefix + "/papers", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        // 解析查询参数
        int userId = 1; // 默认用户
        int limit = 10;
        std::string algo = "hybrid";

        auto userIdIt = req.queryParams.find("user_id");
        if (userIdIt != req.queryParams.end()) {
            userId = std::stoi(userIdIt->second);
        }

        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            limit = std::min(50, std::max(1, std::stoi(limitIt->second)));
        }

        auto algoIt = req.queryParams.find("algorithm");
        if (algoIt != req.queryParams.end()) {
            algo = algoIt->second;
            if (algo == "collaborative") {
                impl_->config_.algorithm = RecommendationAlgorithm::COLLABORATIVE_FILTERING;
            } else if (algo == "content") {
                impl_->config_.algorithm = RecommendationAlgorithm::CONTENT_BASED;
            } else if (algo == "trending") {
                impl_->config_.algorithm = RecommendationAlgorithm::POPULARITY;
            } else {
                impl_->config_.algorithm = RecommendationAlgorithm::HYBRID;
            }
        }

        // 查询缓存
        std::string cacheKey = CacheKeys::recommendations(userId, limit);
        auto cached = QueryCache::instance().get(cacheKey);
        if (cached) {
            spdlog::debug("[RecommendationApi] Papers cache HIT for user={}, limit={}", userId, limit);
            response.statusCode = 200;
            response.headers["X-Cache"] = "HIT";
            response.body = *cached;
            return response;
        }

        try {
            RecommendationRequest request;
            request.userId = userId;
            request.limit = limit;
            request.excludedPaperIds = {};

            auto recommendations = getRecommendations(request);

            json result;
            result["success"] = true;
            result["count"] = recommendations.size();
            result["algorithm"] = algo;

            json items = json::array();
            for (const auto& r : recommendations) {
                json item;
                item["paper_id"] = r.paperId;
                item["title"] = r.title;
                item["authors"] = r.authors;
                item["publication"] = r.publication;
                item["year"] = r.year;
                item["score"] = r.score;
                item["reason"] = r.reason;
                item["algorithm"] = r.algorithm;
                items.push_back(item);
            }
            result["recommendations"] = items;

            response.statusCode = 200;
            response.body = result.dump();
            QueryCache::instance().put(cacheKey, response.body, CacheTTL::RECOMMENDATIONS);
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    // GET /api/recommendations/trending - 热门内容
    router.get(prefix + "/trending", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        int limit = 10;
        std::string timeWindow = "month";

        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            limit = std::min(50, std::max(1, std::stoi(limitIt->second)));
        }

        auto windowIt = req.queryParams.find("window");
        if (windowIt != req.queryParams.end()) {
            timeWindow = windowIt->second;
        }

        try {
            auto trending = getTrendingPapers(limit, timeWindow);

            json result;
            result["success"] = true;
            result["count"] = trending.size();
            result["time_window"] = timeWindow;

            json items = json::array();
            for (const auto& t : trending) {
                json item;
                item["paper_id"] = t.paperId;
                item["title"] = t.title;
                item["authors"] = t.authors;
                item["publication"] = t.publication;
                item["year"] = t.year;
                item["score"] = t.score;
                item["reason"] = t.reason;
                items.push_back(item);
            }
            result["trending"] = items;

            response.statusCode = 200;
            response.body = result.dump();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    // GET /api/recommendations/similar/:paperId - 相似论文
    router.get(prefix + "/similar/:paperId", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        auto paperIdIt = req.pathParams.find("paperId");
        if (paperIdIt == req.pathParams.end()) {
            response.statusCode = 400;
            response.body = json{{"success", false}, {"error", "Missing paper_id"}}.dump();
            return response;
        }

        int paperId = std::stoi(paperIdIt->second);
        int limit = 10;

        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            limit = std::min(50, std::max(1, std::stoi(limitIt->second)));
        }

        try {
            auto similar = getSimilarPapers(paperId, limit);

            json result;
            result["success"] = true;
            result["count"] = similar.size();
            result["paper_id"] = paperId;

            json items = json::array();
            for (const auto& s : similar) {
                json item;
                item["paper_id"] = s.paperId;
                item["title"] = s.title;
                item["authors"] = s.authors;
                item["publication"] = s.publication;
                item["year"] = s.year;
                item["score"] = s.score;
                item["reason"] = s.reason;
                items.push_back(item);
            }
            result["similar"] = items;

            response.statusCode = 200;
            response.body = result.dump();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    // POST /api/recommendations/feedback - 推荐反馈
    router.post(prefix + "/feedback", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        try {
            auto body = json::parse(req.body);

            int userId = body.value("user_id", 1);
            int paperId = body["paper_id"];
            bool liked = body.value("liked", true);
            int rating = body.value("rating", 5);

            if (rating < 1) rating = 1;
            if (rating > 5) rating = 5;

            bool success = recordFeedback(userId, paperId, liked, rating);

            // 反馈后失效该用户的推荐缓存
            QueryCache::instance().invalidatePattern("rec:" + std::to_string(userId) + ":");
            spdlog::debug("[RecommendationApi] Cache invalidated for user {} after feedback", userId);

            response.statusCode = success ? 200 : 500;
            response.body = json{
                {"success", success},
                {"message", success ? "Feedback recorded" : "Failed to record feedback"}
            }.dump();
        } catch (const json::exception& e) {
            response.statusCode = 400;
            response.body = json{
                {"success", false},
                {"error", "Invalid JSON: " + std::string(e.what())}
            }.dump();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    // GET /api/recommendations/explain/:paperId - 推荐解释
    router.get(prefix + "/explain/:paperId", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        auto paperIdIt = req.pathParams.find("paperId");
        if (paperIdIt == req.pathParams.end()) {
            response.statusCode = 400;
            response.body = json{{"success", false}, {"error", "Missing paper_id"}}.dump();
            return response;
        }

        int userId = 1;
        auto userIdIt = req.queryParams.find("user_id");
        if (userIdIt != req.queryParams.end()) {
            userId = std::stoi(userIdIt->second);
        }

        int paperId = std::stoi(paperIdIt->second);

        try {
            std::string explanation = explainRecommendation(userId, paperId);

            response.statusCode = 200;
            response.body = explanation;
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    // GET /api/recommendations/stats - 推荐统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        try {
            auto stats = getStats();

            response.statusCode = 200;
            response.body = json{
                {"success", true},
                {"stats", stats}
            }.dump();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    // GET /api/recommendations/embedding - 基于嵌入向量的推荐
    router.get(prefix + "/embedding", [this](const HttpRequest& req) {
        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        int userId = 1;
        int limit = 10;

        auto userIdIt = req.queryParams.find("user_id");
        if (userIdIt != req.queryParams.end()) {
            userId = std::stoi(userIdIt->second);
        }

        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            limit = std::min(50, std::max(1, std::stoi(limitIt->second)));
        }

        try {
            auto recommendations = impl_->embeddingBasedRecommendation(userId, limit, {});

            json result;
            result["success"] = true;
            result["count"] = recommendations.size();
            result["algorithm"] = "embedding";

            json items = json::array();
            for (const auto& r : recommendations) {
                json item;
                item["paper_id"] = r.paperId;
                item["title"] = r.title;
                item["authors"] = r.authors;
                item["publication"] = r.publication;
                item["year"] = r.year;
                item["score"] = r.score;
                item["reason"] = r.reason;
                item["algorithm"] = r.algorithm;
                items.push_back(item);
            }
            result["recommendations"] = items;

            response.statusCode = 200;
            response.body = result.dump();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = json{
                {"success", false},
                {"error", std::string(e.what())}
            }.dump();
        }

        return response;
    });

    spdlog::info("[RecommendationApiModule] Registered 7 routes");
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
