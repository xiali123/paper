#include "business/AnalyticsIntelligenceModule.hpp"
#include "core/Router.hpp"
#include "core/EventDrivenIntegration.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace PaperCrawler {

class AnalyticsIntelligenceModule::Impl {
public:
    std::shared_ptr<UnifiedAIWorkflow> aiWorkflow_;
};

AnalyticsIntelligenceModule::AnalyticsIntelligenceModule(std::shared_ptr<IDatabase> database)
    : database_(database), impl_(std::make_unique<Impl>()) {

    // 解析AI工作流
    impl_->aiWorkflow_ = Services::resolve<UnifiedAIWorkflow>();
}

AnalyticsIntelligenceModule::~AnalyticsIntelligenceModule() = default;

void AnalyticsIntelligenceModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // 1. 学术影响力仪表盘
    router.get(prefix + "/impact/:userId", [this](const HttpRequest& req) {
        // 提取userId
        std::string userIdStr = req.pathParams.at("userId");
        int userId = std::stoi(userIdStr);

        // 提取timeframe查询参数
        std::string timeframe = req.getQuery("timeframe", "last_6_months");

        auto metrics = getImpactMetrics(userId, timeframe);

        // 构建JSON响应
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < metrics.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"metricType\":\"" << metrics[i].metricType << "\",";
            json << "\"metricValue\":" << metrics[i].metricValue << ",";
            json << "\"comparisonValue\":" << metrics[i].comparisonValue << ",";
            json << "\"percentile\":" << metrics[i].percentile << ",";
            json << "\"trend\":" << metrics[i].trend;
            json << "}";
        }
        json << "]";

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(json.str());
        return response;
    });

    // 2. 研究兴趣演化
    router.get(prefix + "/interests/:userId", [this](const HttpRequest& req) {
        std::string userIdStr = req.pathParams.at("userId");
        int userId = std::stoi(userIdStr);

        auto interests = getResearchInterests(userId);

        // 构建JSON响应
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < interests.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"keyword\":\"" << interests[i].keyword << "\",";
            json << "\"category\":\"" << interests[i].category << "\",";
            json << "\"weight\":" << interests[i].weight << ",";
            json << "\"trendScore\":" << interests[i].trendScore << ",";
            json << "\"occurrenceCount\":" << interests[i].occurrenceCount;
            json << "}";
        }
        json << "]";

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(json.str());
        return response;
    });

    // 3. 每日学术简报
    router.post(prefix + "/briefings/generate", [this](const HttpRequest& req) {
        // 解析请求体
        // TODO: 使用JsonUtils解析JSON
        int userId = 1; // 示例
        std::string date = ""; // 使用默认（今天）

        auto briefing = generateDailyBriefing(userId, date);

        // 构建JSON响应
        std::ostringstream json;
        json << "{";
        json << "\"userId\":" << briefing.userId << ",";
        json << "\"briefingDate\":\"" << briefing.briefingDate << "\",";
        json << "\"summary\":\"" << escapeJson(briefing.summary) << "\",";
        json << "\"highlights\":[";
        for (size_t i = 0; i < briefing.highlights.size(); ++i) {
            if (i > 0) json << ",";
            json << "\"" << escapeJson(briefing.highlights[i]) << "\"";
        }
        json << "],";
        json << "\"isSent\":" << (briefing.isSent ? "true" : "false");
        json << "}";

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(json.str());
        return response;
    });

    // 其他路由...
    // （省略以节省空间）
}

// ============================================================================
// 1. 学术影响力仪表盘实现
// ============================================================================

std::vector<AcademicImpactMetrics> AnalyticsIntelligenceModule::getImpactMetrics(
    int userId, const std::string& timeframe) {

    return calculateImpactMetrics(userId, timeframe);
}

std::vector<AcademicImpactMetrics> AnalyticsIntelligenceModule::calculateImpactMetrics(
    int userId, const std::string& timeframe) {

    std::vector<AcademicImpactMetrics> metrics;

    // 计算时间范围
    std::string dateCondition;
    if (timeframe == "last_6_months") {
        dateCondition = "recorded_at >= DATE_SUB(CURDATE(), INTERVAL 6 MONTH)";
    } else if (timeframe == "last_1_year") {
        dateCondition = "recorded_at >= DATE_SUB(CURDATE(), INTERVAL 1 YEAR)";
    } else {
        dateCondition = "1=1"; // all time
    }

    // 查询用户的影响力指标
    std::ostringstream sql;
    sql << "SELECT metric_type, AVG(metric_value) as avg_value, ";
    sql << "AVG(comparison_value) as avg_comparison, AVG(percentile) as avg_percentile ";
    sql << "FROM academic_impact_metrics ";
    sql << "WHERE user_id = " << userId << " AND " << dateCondition << " ";
    sql << "GROUP BY metric_type";

    auto rows = database_->query(sql.str());

    for (const auto& row : rows) {
        AcademicImpactMetrics metric;
        metric.userId = userId;
        metric.metricType = row.at("metric_type");
        metric.metricValue = std::stod(row.at("avg_value"));
        metric.comparisonValue = std::stod(row.at("avg_comparison"));
        metric.percentile = std::stof(row.at("avg_percentile"));

        // 计算趋势（简化实现：与上次对比）
        // TODO: 实现真实趋势计算
        metric.trend = 0.05; // 示例：5%增长

        metrics.push_back(metric);
    }

    return metrics;
}

bool AnalyticsIntelligenceModule::updateImpactMetrics(
    int userId, const std::string& metricType, double value) {

    try {
        // 获取同行平均值（从peer_comparison_analysis表）
        std::ostringstream sql;
        sql << "SELECT AVG(user_value) as peer_avg FROM peer_comparison_analysis ";
        sql << "WHERE metric_name = '" << metricType << "' ";
        sql << "AND comparison_group = 'field'";

        auto rows = database_->query(sql.str());
        double peerAverage = value; // 默认值
        if (!rows.empty()) {
            peerAverage = std::stod(rows[0]["peer_avg"]);
        }

        // 计算百分位（简化实现）
        float percentile = 0.5f; // TODO: 实现真实百分位计算

        // 插入新记录
        PreparedStatement stmt(database_,
            "INSERT INTO academic_impact_metrics "
            "(user_id, metric_type, metric_value, comparison_value, percentile, recorded_at) "
            "VALUES (?, ?, ?, ?, ?, CURDATE())"
        );

        stmt.bind(1, userId);
        stmt.bind(2, metricType);
        stmt.bind(3, value);
        stmt.bind(4, peerAverage);
        stmt.bind(5, percentile);

        return stmt.execute();

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to update impact metrics: " + std::string(e.what()));
        }
        return false;
    }
}

// ============================================================================
// 2. 研究兴趣演化实现
// ============================================================================

std::vector<ResearchInterest> AnalyticsIntelligenceModule::getResearchInterests(
    int userId, int limit) {

    return analyzeResearchInterests(userId);
}

std::vector<ResearchInterest> AnalyticsIntelligenceModule::analyzeResearchInterests(int userId) {
    std::vector<ResearchInterest> interests;

    // 查询用户的研究兴趣（按权重排序）
    QueryBuilder queryBuilder(database_);
    queryBuilder.select()
        .from("research_interest_evolution")
        .where("user_id", "=", userId)
        .orderBy("weight", false)  // DESC
        .limit(20);

    std::string sql = queryBuilder.buildSQL();
    auto rows = database_->query(sql);

    for (const auto& row : rows) {
        ResearchInterest interest;
        interest.userId = userId;
        interest.keyword = row.at("interest_keyword");
        interest.category = row["category"]; // 可能为空
        interest.weight = std::stod(row.at("weight"));
        interest.trendScore = std::stof(row.at("trend_score"));
        interest.occurrenceCount = std::stoi(row.at("occurrence_count"));
        interest.firstSeenAt = row.at("first_seen_at");
        interest.lastSeenAt = row.at("last_seen_at");

        interests.push_back(interest);
    }

    return interests;
}

double AnalyticsIntelligenceModule::calculateTFIDF(const std::string& term, int userId) {
    // 简化的TF-IDF计算
    // TF (Term Frequency) = 该词在用户文档中出现的次数 / 用户总词数
    // IDF (Inverse Document Frequency) = log(总用户数 / 包含该词的用户数)

    // TODO: 实现完整的TF-IDF计算
    return 1.0; // 占位符
}

float AnalyticsIntelligenceModule::calculateTrendScore(const std::string& keyword, int userId) {
    // 计算趋势分数（-1到1）
    // 基于最近30天与之前30天的对比

    // TODO: 实现真实趋势计算
    return 0.1f; // 占位符：轻微上升
}

// ============================================================================
// 3. 每日学术简报实现
// ============================================================================

DailyBriefing AnalyticsIntelligenceModule::generateDailyBriefing(
    int userId, const std::string& date) {

    DailyBriefing briefing;
    briefing.userId = userId;

    // 使用今天的日期
    if (date.empty()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time_t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        briefing.briefingDate = oss.str();
    } else {
        briefing.briefingDate = date;
    }

    try {
        // 1. 生成简报摘要（使用AI）
        std::ostringstream prompt;
        prompt << "Generate a daily academic research briefing for user " << userId << ".\n";
        prompt << "Include:\n";
        prompt << "- 3-5 key highlights from their research area\n";
        prompt << "- Recommended papers based on their interests\n";
        prompt << "- Trending topics\n";
        prompt << "- Potential collaboration opportunities\n";

        if (impl_->aiWorkflow_) {
            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt.str(),
                AIModelType::GPT_4_MINI
            );

            if (aiResult.success) {
                briefing.summary = aiResult.content;

                // TODO: 解析AI响应，提取结构化数据
                briefing.highlights = {"Highlight 1", "Highlight 2", "Highlight 3"};
                briefing.recommendedPapers = {1, 2, 3};
                briefing.trendingTopics = {"Topic 1", "Topic 2"};
                briefing.collaborationOpportunities = {"Opportunity 1"};
            }
        }

        // 2. 保存到数据库
        PreparedStatement stmt(database_,
            "INSERT INTO daily_briefings "
            "(user_id, briefing_date, content, summary, highlights, recommended_papers, trending_topics, collaboration_opportunities) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        // 序列化为JSON
        // TODO: 使用JsonUtils

        stmt.bind(1, userId);
        stmt.bind(2, briefing.briefingDate);
        stmt.bind(3, briefing.summary); // content JSON
        stmt.bind(4, briefing.summary);
        stmt.bind(5, std::string("{}")); // highlights JSON
        stmt.bind(6, std::string("{}")); // recommended_papers JSON
        stmt.bind(7, std::string("{}")); // trending_topics JSON
        stmt.bind(8, std::string("{}")); // collaboration_opportunities JSON

        stmt.execute();

        // 3. 发布事件
        EventPublisher::analyticsEventTracked(
            userId,
            "daily_briefing_generated",
            {{"date", briefing.briefingDate}}
        );

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to generate daily briefing: " + std::string(e.what()));
        }
    }

    return briefing;
}

std::vector<DailyBriefing> AnalyticsIntelligenceModule::getBriefingHistory(
    int userId, int page, int limit) {

    std::vector<DailyBriefing> briefings;

    QueryBuilder queryBuilder(database_);
    queryBuilder.select()
        .from("daily_briefings")
        .where("user_id", "=", userId)
        .orderBy("briefing_date", false)
        .limit(limit)
        .offset((page - 1) * limit);

    auto rows = queryBuilder.query();

    // TODO: 解析行数据为DailyBriefing对象

    return briefings;
}

bool AnalyticsIntelligenceModule::sendBriefing(int briefingId, const std::string& method) {
    // TODO: 实现邮件/Push通知发送
    // 1. 查询简报内容
    // 2. 构建邮件/Push消息
    // 3. 发送
    // 4. 更新is_sent和sent_at字段

    return true;
}

// ============================================================================
// 4. 学术基因图谱实现
// ============================================================================

std::vector<AcademicGeneNode> AnalyticsIntelligenceModule::buildAcademicGenealogy(
    int paperId, int maxDepth) {

    std::vector<AcademicGeneNode> genealogy;

    // BFS构建引用图谱
    std::set<int> visited;
    std::queue<std::pair<int, int>> queue; // (paperId, depth)
    queue.push({paperId, 0});
    visited.insert(paperId);

    while (!queue.empty()) {
        auto [currentId, depth] = queue.front();
        queue.pop();

        if (depth > maxDepth) continue;

        // 查询当前论文
        std::ostringstream sql;
        sql << "SELECT id, title, authors, year FROM papers WHERE id = " << currentId;
        auto papers = database_->query(sql.str());

        if (!papers.empty()) {
            AcademicGeneNode node;
            node.paperId = currentId;
            node.title = papers[0]["title"];
            node.authors = papers[0]["authors"];
            node.year = std::stoi(papers[0]["year"]);
            node.depth = depth;

            // 查询引用关系
            std::ostringstream citationSql;
            citationSql << "SELECT target_paper_id, relationship_type FROM citation_relationships ";
            citationSql << "WHERE source_paper_id = " << currentId;

            auto citations = database_->query(citationSql.str());

            for (const auto& citation : citations) {
                int targetId = std::stoi(citation["target_paper_id"]);
                std::string relationshipType = citation["relationship_type"];

                // 添加未访问的节点到队列
                if (visited.find(targetId) == visited.end()) {
                    visited.insert(targetId);
                    queue.push({targetId, depth + 1});
                }

                // TODO: 添加边关系
            }

            genealogy.push_back(node);
        }
    }

    return genealogy;
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string AnalyticsIntelligenceModule::escapeJson(const std::string& str) {
    json j = str;
    return j.dump();
}

} // namespace PaperCrawler
