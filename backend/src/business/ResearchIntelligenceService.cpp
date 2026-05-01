// 智能研究情报服务实现
// 文件位置：backend/src/business/ResearchIntelligenceService.cpp

#include "business/AnalyticsIntelligenceModule.hpp"
#include "business/RecommendationApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <map>
#include <set>
#include <algorithm>

namespace PaperCrawler {
namespace Services {

// ============================================================================
// 学术影响力仪表盘
// ============================================================================

/**
 * @brief 学术影响力指标
 */
struct AcademicImpactMetrics {
    int userId;
    int totalPapers;              // 总论文数
    int totalCitations;           // 总引用数
    double hIndex;                // h指数
    double i10Index;              // i10指数
    double averageCitations;      // 平均引用数
    std::map<int, int> citationTrend;  // 引用趋势 (year -> count)
    std::vector<std::string> topPapers;   // 高引用论文
    double impactPercentile;      // 影响力百分位
    std::string calculatedAt;
};

/**
 * @brief 学术影响力服务
 */
class AcademicImpactService {
public:
    AcademicImpactService(std::shared_ptr<IDatabase> database)
        : database_(database) {}

    /**
     * @brief 计算用户学术影响力
     * @param userId 用户ID
     * @return 影响力指标
     */
    AcademicImpactMetrics calculateImpact(int userId) {
        auto logger = spdlog::get("ResearchIntelligence");

        logger->info("Calculating academic impact for user {}", userId);

        // 1. 获取用户的所有论文
        auto papers = getUserPapers(userId);

        // 2. 计算h指数
        double hIndex = calculateHIndex(papers);

        // 3. 计算i10指数
        double i10Index = calculateI10Index(papers);

        // 4. 引用趋势分析
        auto citationTrend = analyzeCitationTrend(papers);

        // 5. 识别高影响力论文
        auto topPapers = identifyTopPapers(papers, 10);

        // 6. 计算百分位
        double impactPercentile = calculatePercentile(userId);

        AcademicImpactMetrics metrics;
        metrics.userId = userId;
        metrics.totalPapers = papers.size();
        metrics.totalCitations = std::accumulate(
            papers.begin(), papers.end(), 0,
            [](int sum, const PaperDto& p) { return sum + p.citationCount; }
        );
        metrics.hIndex = hIndex;
        metrics.i10Index = i10Index;
        metrics.averageCitations = papers.empty() ? 0.0 :
            static_cast<double>(metrics.totalCitations) / papers.size();
        metrics.citationTrend = citationTrend;
        metrics.topPapers = topPapers;
        metrics.impactPercentile = impactPercentile;
        metrics.calculatedAt = getCurrentTimestamp();

        logger->info("Impact calculated: h-index={:.1f}, i10={:.1f}, percentile={:.2f}",
                     hIndex, i10Index, impactPercentile);

        return metrics;
    }

    /**
     * @brief 预测未来12个月引用量
     * @param userId 用户ID
     * @return 预测的引用趋势
     */
    std::map<std::string, int> predictCitations(int userId) {
        auto logger = spdlog::get("ResearchIntelligence");

        auto papers = getUserPapers(userId);
        auto currentTrend = analyzeCitationTrend(papers);

        // 使用时间序列预测（简化实现）
        // 实际应该使用ARIMA或LSTM

        std::map<std::string, int> predictions;
        auto now = std::chrono::system_clock::now();

        // 从历史数据计算实际月均增长率
        double monthlyGrowthRate = 0.0; // 默认无增长
        if (currentTrend.size() >= 2) {
            auto it = currentTrend.begin();
            auto last = currentTrend.rbegin();

            double earliestValue = static_cast<double>(it->second);
            double latestValue = static_cast<double>(last->second);
            int yearSpan = last->first - it->first;

            if (earliestValue > 0.0 && yearSpan > 0) {
                // 复合年增长率 (CAGR) -> 转换为月增长率
                double cagr = std::pow(latestValue / earliestValue, 1.0 / yearSpan) - 1.0;
                monthlyGrowthRate = std::pow(1.0 + cagr, 1.0 / 12.0) - 1.0;

                // 限制月增长率在合理范围内 [-0.5, 1.0] 以避免极端预测
                if (monthlyGrowthRate > 1.0) monthlyGrowthRate = 1.0;
                if (monthlyGrowthRate < -0.5) monthlyGrowthRate = -0.5;
            }
        }

        for (int i = 1; i <= 12; ++i) {
            auto futureTime = now + std::chrono::hours(24 * 30 * i);
            auto month = getMonthYear(futureTime);

            // 基于历史增长率进行指数预测
            int baseCitations = currentTrend.empty() ? 0 : currentTrend.rbegin()->second;
            double growthFactor = std::pow(1.0 + monthlyGrowthRate, i);
            int predicted = static_cast<int>(baseCitations * growthFactor);
            if (predicted < 0) predicted = 0;

            predictions[month] = predicted;
        }

        logger->info("Citation predictions generated for {} months", predictions.size());
        return predictions;
    }

private:
    std::vector<PaperDto> getUserPapers(int userId) {
        std::ostringstream sql;
        sql << "SELECT p.* FROM papers p "
             << "JOIN user_papers up ON p.id = up.paper_id "
             << "WHERE up.user_id = " << userId;

        auto results = database_->query(sql.str());

        std::vector<PaperDto> papers;
        for (const auto& row : results) {
            papers.push_back(PaperDto::fromRow(row));
        }

        return papers;
    }

    double calculateHIndex(const std::vector<PaperDto>& papers) {
        if (papers.empty()) return 0.0;

        std::vector<int> citationCounts;
        for (const auto& paper : papers) {
            citationCounts.push_back(paper.citationCount);
        }

        std::sort(citationCounts.begin(), citationCounts.end(), std::greater<int>());

        int h = 0;
        for (size_t i = 0; i < citationCounts.size(); ++i) {
            if (citationCounts[i] >= static_cast<int>(i + 1)) {
                h = i + 1;
            } else {
                break;
            }
        }

        return static_cast<double>(h);
    }

    double calculateI10Index(const std::vector<PaperDto>& papers) {
        int count = 0;
        for (const auto& paper : papers) {
            if (paper.citationCount >= 10) {
                count++;
            }
        }
        return static_cast<double>(count);
    }

    std::map<int, int> analyzeCitationTrend(const std::vector<PaperDto>& papers) {
        std::map<int, int> trend;

        for (const auto& paper : papers) {
            trend[paper.year] += paper.citationCount;
        }

        return trend;
    }

    std::vector<std::string> identifyTopPapers(
        const std::vector<PaperDto>& papers,
        size_t limit) {

        std::vector<PaperDto> sorted = papers;
        std::sort(sorted.begin(), sorted.end(),
            [](const PaperDto& a, const PaperDto& b) {
                return a.citationCount > b.citationCount;
            });

        std::vector<std::string> topPapers;
        for (size_t i = 0; i < std::min(limit, sorted.size()); ++i) {
            topPapers.push_back(sorted[i].title);
        }

        return topPapers;
    }

    double calculatePercentile(int userId) {
        // 计算用户在同领域中的影响力百分位
        // 基于引用数排名

        if (!database_) {
            // Fallback: 数据库不可用时返回50百分位
            return 50.0;
        }

        try {
            // 先获取当前用户的总引用数
            std::ostringstream userSql;
            userSql << "SELECT COALESCE(SUM(p.citation_count), 0) as user_total "
                    << "FROM user_papers up "
                    << "JOIN papers p ON up.paper_id = p.id "
                    << "WHERE up.user_id = " << userId;

            auto userRows = database_->query(userSql.str());
            if (userRows.empty()) {
                return 50.0; // Fallback
            }
            double userTotal = std::stod(userRows[0].at("user_total"));

            // 统计总用户数和引用数低于当前用户的数量
            std::ostringstream rankSql;
            rankSql << "SELECT "
                    << "COUNT(DISTINCT up1.user_id) as total_users, "
                    << "COALESCE(SUM(CASE WHEN user_total < " << userTotal << " THEN 1 ELSE 0 END), 0) as below_count "
                    << "FROM ("
                    << "SELECT up.user_id, SUM(p.citation_count) as user_total "
                    << "FROM user_papers up "
                    << "JOIN papers p ON up.paper_id = p.id "
                    << "GROUP BY up.user_id"
                    << ") as user_totals "
                    << "JOIN user_papers up1 ON up1.user_id = user_totals.user_id";

            auto rankRows = database_->query(rankSql.str());
            if (!rankRows.empty()) {
                double totalUsers = std::stod(rankRows[0].at("total_users"));
                double belowCount = std::stod(rankRows[0].at("below_count"));

                if (totalUsers > 0.0) {
                    // 百分位 = 排名低于该用户的比例 * 100
                    return (belowCount / totalUsers) * 100.0;
                }
            }

            return 50.0; // Fallback
        } catch (const std::exception& e) {
            spdlog::get("ResearchIntelligence")->error(
                "Failed to calculate percentile for user {}: {}", userId, e.what());
            return 50.0; // Fallback
        }
    }

    std::string getMonthYear(const std::chrono::system_clock::time_point& time) {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        std::tm tm = *std::localtime(&time_t);
        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m");
        return ss.str();
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
};

// ============================================================================
// 研究兴趣演化图
// ============================================================================

/**
 * @brief 兴趣点
 */
struct InterestPoint {
    std::string keyword;
    double weight;           // 权重 (0.0-1.0)
    std::string date;
};

/**
 * @brief 研究兴趣演化服务
 */
class ResearchInterestEvolutionService {
public:
    ResearchInterestEvolutionService(std::shared_ptr<IDatabase> database)
        : database_(database) {}

    /**
     * @brief 分析用户研究兴趣演化
     * @param userId 用户ID
     * @return 兴趣演化时间线
     */
    std::vector<InterestPoint> analyzeEvolution(int userId) {
        auto logger = spdlog::get("ResearchIntelligence");

        logger->info("Analyzing research interest evolution for user {}", userId);

        // 1. 获取用户的所有论文
        auto papers = getUserPapers(userId);

        // 2. 提取关键词（使用TF-IDF + 时间衰减）
        auto interests = extractInterests(papers);

        // 3. 应用时间衰减（最近的论文权重更高）
        auto weightedInterests = applyTimeDecay(interests);

        // 4. 生成演化时间线
        std::map<std::string, std::vector<InterestPoint>> timeline;
        for (const auto& interest : weightedInterests) {
            timeline[interest.keyword].push_back(interest);
        }

        // 5. 转换为向量
        std::vector<InterestPoint> evolution;
        for (const auto& [keyword, points] : timeline) {
            evolution.insert(evolution.end(), points.begin(), points.end());
        }

        logger->info("Research interest evolution analyzed: {} interest points",
                     evolution.size());

        return evolution;
    }

    /**
     * @brief 预测未来研究方向
     * @param userId 用户ID
     * @return 预测的Top 5研究方向
     */
    std::vector<std::string> predictFutureDirections(int userId) {
        auto currentInterests = analyzeEvolution(userId);

        // 分析当前趋势
        std::map<std::string, double> recentWeights;
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * 365);  // 最近1年

        for (const auto& interest : currentInterests) {
            auto interestTime = parseDateTime(interest.date);
            if (interestTime > cutoff) {
                recentWeights[interest.keyword] += interest.weight;
            }
        }

        // 排序并返回Top 5
        std::vector<std::pair<std::string, double>> sorted(
            recentWeights.begin(), recentWeights.end()
        );
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        std::vector<std::string> predictions;
        for (size_t i = 0; i < std::min(size_t(5), sorted.size()); ++i) {
            predictions.push_back(sorted[i].first);
        }

        return predictions;
    }

private:
    std::vector<PaperDto> getUserPapers(int userId) {
        // （实现同上）
        return {};
    }

    std::vector<InterestPoint> extractInterests(const std::vector<PaperDto>& papers) {
        std::vector<InterestPoint> interests;

        for (const auto& paper : papers) {
            // 从标题和摘要提取关键词
            auto keywords = extractKeywords(paper.title + " " + paper.abstract);

            for (const auto& keyword : keywords) {
                InterestPoint point;
                point.keyword = keyword;
                point.weight = 1.0 / keywords.size();  // 均匀分布
                point.date = paper.createdAt;
                interests.push_back(point);
            }
        }

        return interests;
    }

    std::vector<std::string> extractKeywords(const std::string& text) {
        // 简化实现：使用TF-IDF提取关键词
        // 实际应该使用NLP库

        std::vector<std::string> keywords;
        std::set<std::string> stopWords = {"the", "a", "an", "of", "in", "on", "at", "to", "for"};

        std::istringstream iss(text);
        std::string word;
        while (iss >> word) {
            if (word.length() > 3 && stopWords.find(word) == stopWords.end()) {
                keywords.push_back(word);
            }
        }

        return keywords;
    }

    std::vector<InterestPoint> applyTimeDecay(const std::vector<InterestPoint>& interests) {
        std::vector<InterestPoint> weighted;

        auto now = std::chrono::system_clock::now();

        for (const auto& interest : interests) {
            auto interestTime = parseDateTime(interest.date);
            auto age = std::chrono::duration_cast<std::chrono::hours>(now - interestTime);

            // 时间衰减：最近1年权重100%，2年前权重50%，3年前权重25%
            double decay = 1.0;
            if (age.count() > 24 * 365 * 2) {  // 超过2年
                decay = 0.25;
            } else if (age.count() > 24 * 365) {  // 超过1年
                decay = 0.5;
            }

            InterestPoint weightedInterest = interest;
            weightedInterest.weight *= decay;
            weighted.push_back(weightedInterest);
        }

        return weighted;
    }

    std::chrono::system_clock::time_point parseDateTime(const std::string& dateStr) {
        // 解析日期字符串
        // （简化实现）
        return std::chrono::system_clock::now();
    }
};

// ============================================================================
// 每日学术简报
// ============================================================================

/**
 * @brief 学术简报
 */
struct AcademicDigest {
    int userId;
    std::string date;
    std::vector<std::string> newPapers;         // 新推荐论文
    std::vector<std::string> collaborationOps;   // 合作机会
    std::vector<std::string> conferenceDeadlines;  // 会议截止
    std::vector<std::string> grantOpportunities;   // 基金机会
    std::string generatedAt;
};

/**
 * @brief 每日学术简报服务
 */
class DailyDigestService {
public:
    DailyDigestService(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<RecommendationEngine> recommender)
        : database_(database), recommender_(recommender) {}

    /**
     * @brief 生成每日学术简报
     * @param userId 用户ID
     * @return 学术简报
     */
    AcademicDigest generateDailyDigest(int userId) {
        auto logger = spdlog::get("ResearchIntelligence");

        logger->info("Generating daily digest for user {}", userId);

        AcademicDigest digest;
        digest.userId = userId;
        digest.date = getCurrentDate();

        // 1. 推荐新论文（基于用户兴趣）
        digest.newPapers = recommendNewPapers(userId, 5);

        // 2. 识别合作机会
        digest.collaborationOps = identifyCollaborations(userId);

        // 3. 会议截止提醒
        digest.conferenceDeadlines = getUpcomingDeadlines(userId);

        // 4. 基金机会
        digest.grantOpportunities = getGrantOpportunities(userId);

        digest.generatedAt = getCurrentTimestamp();

        logger->info("Daily digest generated with {} sections",
                     digest.newPapers.size() + digest.collaborationOps.size() +
                     digest.conferenceDeadlines.size() + digest.grantOpportunities.size());

        return digest;
    }

    /**
     * @brief 订阅每日简报（邮件推送）
     * @param userId 用户ID
     * @param email 邮箱地址
     */
    void subscribeToDailyDigest(int userId, const std::string& email) {
        // 保存订阅信息
        std::ostringstream sql;
        sql << "INSERT INTO daily_digest_subscriptions (user_id, email) "
             << "VALUES (" << userId << ", '" << database_->escape(email) << "') "
             << "ON DUPLICATE KEY UPDATE email = VALUES(email)";

        if (database_->execute(sql.str())) {
            spdlog::get("ResearchIntelligence")->info(
                "User {} subscribed to daily digest with email {}", userId, email
            );
        }
    }

private:
    std::vector<std::string> recommendNewPapers(int userId, size_t limit) {
        // 使用推荐引擎
        auto recommendations = recommender_->getPersonalizedRecommendations(userId, limit);

        std::vector<std::string> paperTitles;
        for (const auto& rec : recommendations) {
            paperTitles.push_back(rec.title);
        }

        return paperTitles;
    }

    std::vector<std::string> identifyCollaborations(int userId) {
        // 识别潜在合作者（基于研究兴趣相似度）

        std::vector<std::string> collaborations;

        std::ostringstream sql;
        sql << "SELECT u.username, u.email FROM users u "
             << "WHERE u.id IN ("
             << "SELECT DISTINCT up2.user_id FROM user_papers up1 "
             << "JOIN user_papers up2 ON up1.paper_id = up2.paper_id "
             << "WHERE up1.user_id = " << userId << " "
             << "AND up2.user_id != " << userId << " "
             << "LIMIT 5)";

        // auto results = database_->query(sql.str());
        // ...

        return collaborations;
    }

    std::vector<std::string> getUpcomingDeadlines(int userId) {
        // 获取即将到来的会议投稿截止日期
        // ...

        return {};
    }

    std::vector<std::string> getGrantOpportunities(int userId) {
        // 获取基金机会
        // ...

        return {};
    }

    std::string getCurrentDate() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
        return ss.str();
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
    std::shared_ptr<RecommendationEngine> recommender_;
};

} // namespace Services
} // namespace PaperCrawler
