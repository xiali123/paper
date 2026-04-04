#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace PaperCrawler {

/**
 * @brief 学术影响力指标
 */
struct AcademicImpactMetrics {
    int userId;
    std::string metricType;  // citations, h_index, impact_factor, downloads
    double metricValue;
    double comparisonValue;  // 同行平均值
    float percentile;  // 百分位
    std::string recordedAt;
    double trend;  // 趋势（正数表示上升）
};

/**
 * @brief 研究兴趣演化数据
 */
struct ResearchInterest {
    int userId;
    std::string keyword;
    std::string category;  // field, topic, method, author
    double weight;  // TF-IDF权重
    float trendScore;  // 趋势分数（-1到1）
    int occurrenceCount;
    std::string firstSeenAt;
    std::string lastSeenAt;
};

/**
 * @brief 每日学术简报
 */
struct DailyBriefing {
    int userId;
    std::string briefingDate;
    std::string summary;
    std::vector<std::string> highlights;
    std::vector<int> recommendedPapers;
    std::vector<std::string> trendingTopics;
    std::vector<std::string> collaborationOpportunities;
    bool isSent;
    std::string sentAt;
};

/**
 * @brief 学术基因节点
 */
struct AcademicGeneNode {
    int paperId;
    std::string title;
    std::string authors;
    int year;
    std::string relationshipType;  // cites, cited_by, similar_to
    float strength;
    int depth;
};

/**
 * @brief 同行对比数据
 */
struct PeerComparison {
    int userId;
    std::string comparisonGroup;  // institution, field, career_stage
    std::string metricName;
    double userValue;
    double peerAverage;
    double peerMedian;
    float percentile;
    int ranking;
    int totalPeers;
};

/**
 * @brief 预测分析结果
 */
struct PredictionResult {
    int userId;
    std::string predictionType;
    std::string targetDate;
    double predictedValue;
    double confidenceLower;
    double confidenceUpper;
    float confidenceLevel;
    std::string recommendation;
};

/**
 * @brief 智能研究情报模块
 *
 * 功能：
 * 1. 学术影响力仪表盘 - 实时追踪学术影响力
 * 2. 研究兴趣演化图 - 可视化研究兴趣变化
 * 3. 每日学术简报 - 个性化研究动态推送
 * 4. 学术基因图谱 - 可视化论文传承关系
 * 5. 同行对比分析 - 与同领域研究者对比
 * 6. 预测性分析 - 预测未来研究方向和影响力
 *
 * ROI: 8.5/10
 * 开发周期: 5-6周
 */
class AnalyticsIntelligenceModule : public BusinessModuleBase {
public:
    explicit AnalyticsIntelligenceModule(std::shared_ptr<IDatabase> database);
    ~AnalyticsIntelligenceModule() override;

    std::string getName() const override { return "AnalyticsIntelligence"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Intelligent analytics for academic research";
    }

    // ========================================================================
    // 1. 学术影响力仪表盘
    // ========================================================================

    /**
     * @brief 获取学术影响力指标
     * GET /api/analytics-intelligence/impact/:userId
     */
    std::vector<AcademicImpactMetrics> getImpactMetrics(int userId, const std::string& timeframe = "last_6_months");

    /**
     * @brief 更新影响力指标
     * POST /api/analytics-intelligence/impact/update
     */
    bool updateImpactMetrics(int userId, const std::string& metricType, double value);

    /**
     * @brief 获取影响力趋势图数据
     * GET /api/analytics-intelligence/impact/trend/:userId
     */
    std::map<std::string, std::vector<double>> getImpactTrend(int userId, const std::string& metricType);

    // ========================================================================
    // 2. 研究兴趣演化
    // ========================================================================

    /**
     * @brief 获取研究兴趣列表
     * GET /api/analytics-intelligence/interests/:userId
     */
    std::vector<ResearchInterest> getResearchInterests(int userId, int limit = 20);

    /**
     * @brief 分析研究兴趣演化
     * GET /api/analytics-intelligence/interests/evolution/:userId
     */
    std::map<std::string, std::vector<ResearchInterest>> getInterestEvolution(int userId);

    /**
     * @brief 获取兴趣可视化数据（用于图表）
     * GET /api/analytics-intelligence/interests/visualization/:userId
     */
    std::map<std::string, std::string> getInterestVisualizationData(int userId);

    // ========================================================================
    // 3. 每日学术简报
    // ========================================================================

    /**
     * @brief 生成每日学术简报
     * POST /api/analytics-intelligence/briefings/generate
     */
    DailyBriefing generateDailyBriefing(int userId, const std::string& date = "");

    /**
     * @brief 获取简报历史
     * GET /api/analytics-intelligence/briefings/:userId
     */
    std::vector<DailyBriefing> getBriefingHistory(int userId, int page = 1, int limit = 30);

    /**
     * @brief 发送简报（邮件/Push）
     * POST /api/analytics-intelligence/briefings/:id/send
     */
    bool sendBriefing(int briefingId, const std::string& method = "email");

    // ========================================================================
    // 4. 学术基因图谱
    // ========================================================================

    /**
     * @brief 构建学术基因图谱
     * GET /api/analytics-intelligence/genealogy/:paperId
     */
    std::vector<AcademicGeneNode> buildAcademicGenealogy(int paperId, int maxDepth = 3);

    /**
     * @brief 获取论文的引用传承路径
     * GET /api/analytics-intelligence/genealogy/path/:paperId
     */
    std::vector<std::vector<int>> getCitationPaths(int paperId);

    // ========================================================================
    // 5. 同行对比分析
    // ========================================================================

    /**
     * @brief 获取同行对比数据
     * GET /api/analytics-intelligence/comparison/:userId
     */
    std::vector<PeerComparison> getPeerComparison(int userId, const std::string& comparisonGroup);

    /**
     * @brief 生成对比报告
     * POST /api/analytics-intelligence/comparison/report
     */
    std::map<std::string, std::string> generateComparisonReport(int userId);

    // ========================================================================
    // 6. 预测性分析
    // ========================================================================

    /**
     * @brief 获取预测分析
     * GET /api/analytics-intelligence/predictions/:userId
     */
    std::vector<PredictionResult> getPredictions(int userId);

    /**
     * @brief 生成未来预测
     * POST /api/analytics-intelligence/predictions/generate
     */
    PredictionResult generatePrediction(
        int userId,
        const std::string& predictionType,
        const std::string& targetDate
    );

    /**
     * @brief 获取研究趋势预测
     * GET /api/analytics-intelligence/trends/:userId
     */
    std::map<std::string, std::string> getResearchTrendPrediction(int userId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;

    void registerRoutes() override;

    // HTTP处理器
    std::string handleGetImpactMetrics(const std::map<std::string, std::string>& params);
    std::string handleGetInterests(const std::map<std::string, std::string>& params);
    std::string handleGenerateBriefing(const std::map<std::string, std::string>& params);
    std::string handleGetGenealogy(const std::map<std::string, std::string>& params);
    std::string handleGetComparison(const std::map<std::string, std::string>& params);
    std::string handleGetPredictions(const std::map<std::string, std::string>& params);

    // 辅助方法
    std::vector<AcademicImpactMetrics> calculateImpactMetrics(int userId, const std::string& timeframe);
    std::vector<ResearchInterest> analyzeResearchInterests(int userId);
    DailyBriefing buildDailyBriefing(int userId, const std::string& date);
    std::vector<AcademicGeneNode> buildGenealogyGraph(int paperId, int maxDepth);
    double calculateTFIDF(const std::string& term, int userId);
    float calculateTrendScore(const std::string& keyword, int userId);
};

} // namespace PaperCrawler
