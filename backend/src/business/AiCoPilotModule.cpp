#include "business/AiCoPilotModule.hpp"
#include "core/Router.hpp"
#include "core/EventDrivenIntegration.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <iomanip>
#include <random>

namespace PaperCrawler {

class AiCoPilotModule::Impl {
public:
    std::shared_ptr<UnifiedAIWorkflow> aiWorkflow_;
    std::map<std::string, std::vector<std::string>> conversationHistory_; // sessionId -> messages

    // 统计数据
    uint64_t totalReviews_{0};
    uint64_t totalLiteratureReviews_{0};
    uint64_t totalResearchPlans_{0};
    uint64_t totalChats_{0};
};

AiCoPilotModule::AiCoPilotModule(std::shared_ptr<IDatabase> database)
    : database_(database), impl_(std::make_unique<Impl>()) {

    // 解析AI工作流
    impl_->aiWorkflow_ = Services::resolve<UnifiedAIWorkflow>();
}

AiCoPilotModule::~AiCoPilotModule() = default;

void AiCoPilotModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // 1. AI审稿人系统
    router.post(prefix + "/review", [this](const HttpRequest& req) {
        return handleRequest(req); // 代理到handleRequest
    });

    router.get(prefix + "/reviews/:userId", [this](const HttpRequest& req) {
        // 处理获取审稿历史
    });

    // 2. 文献综述生成器
    router.post(prefix + "/literature-review/generate", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    router.get(prefix + "/literature-reviews", [this](const HttpRequest& req) {
        // 处理获取文献综述列表
    });

    // 3. 研究规划助手
    router.post(prefix + "/research-plan/generate", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    router.get(prefix + "/research-plans", [this](const HttpRequest& req) {
        // 处理获取研究计划列表
    });

    // 4. 对话助手
    router.post(prefix + "/chat", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    router.get(prefix + "/conversations", [this](const HttpRequest& req) {
        // 处理获取对话列表
    });

    // 5. 研究建议
    router.get(prefix + "/recommendations", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    // 6. 统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        return handleRequest(req);
    });
}

// ============================================================================
// 1. AI审稿人系统实现
// ============================================================================

AIReviewResult AiCoPilotModule::generateReview(const AIReviewRequest& request) {
    AIReviewResult result;
    result.paperId = request.paperId;
    result.reviewedAt = std::chrono::system_clock::now();

    try {
        // 1. 从数据库获取论文数据
        std::ostringstream sql;
        sql << "SELECT id, title, authors, abstract, content FROM papers WHERE id = "
            << request.paperId;

        auto papers = database_->query(sql.str());
        if (papers.empty()) {
            result.success = false;
            return result;
        }

        auto paper = papers[0];

        // 2. 构建审稿Prompt
        std::string prompt = buildReviewPrompt(request, paper);

        // 3. 调用AI生成审稿意见
        if (impl_->aiWorkflow_) {
            RAGContext ragContext;
            // TODO: 添加相关论文作为对比
            ragContext.relevantPapers = {}; // 从推荐模块获取

            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt,
                AIModelType::GPT_4,
                ragContext,
                request.userId
            );

            if (aiResult.success) {
                result.success = true;
                result.costUsd = aiResult.costUsd;

                // TODO: 解析AI响应，提取结构化数据
                // 简化版：假设AI返回JSON格式
                result.reviewScore = 7; // 示例
                result.acceptanceProbability = 0.65f;
                result.strengths = {"Novel approach", "Good methodology"};
                result.weaknesses = {"Limited experiments", "Missing related work"};
                result.improvements = {"Add more experiments", "Compare with X et al."};
                result.reviewerComments = aiResult.content;

                // 保存到数据库
                std::ostringstream insertSql;
                insertSql << "INSERT INTO ai_review_feedback "
                    << "(paper_id, user_id, review_score, acceptance_probability, "
                    << "improvement_suggestions, reviewer_comments) VALUES ("
                    << request.paperId << ", "
                    << request.userId << ", "
                    << result.reviewScore << ", "
                    << result.acceptanceProbability << ", "
                    << "'[\"suggestion1\", \"suggestion2\"]', "
                    << "\"" << escapeSql(result.reviewerComments) << "\")";

                database_->execute(insertSql.str());

                // 发布事件
                EventPublisher::aiResponseReceived(
                    "review_" + std::to_string(request.paperId),
                    result.reviewerComments
                );

                impl_->totalReviews_++;

            } else {
                result.success = false;
            }
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("AI Review generation failed: " + std::string(e.what()));
        }
        result.success = false;
    }

    return result;
}

std::string AiCoPilotModule::buildReviewPrompt(
    const AIReviewRequest& request,
    const std::map<std::string, std::string>& paperData) {

    std::ostringstream prompt;

    prompt << "You are an expert reviewer for " << request.targetJournal << ".\n\n";
    prompt << "Please review the following paper:\n\n";
    prompt << "Title: " << paperData.at("title") << "\n";
    prompt << "Authors: " << paperData.at("authors") << "\n";
    prompt << "Abstract: " << paperData.at("abstract") << "\n\n";

    prompt << "Please provide a comprehensive review including:\n";
    prompt << "1. Overall score (1-10)\n";
    prompt << "2. Acceptance probability (0-1)\n";
    prompt << "3. Key strengths (3-5 points)\n";
    prompt << "4. Major weaknesses (3-5 points)\n";
    prompt << "5. Specific improvement suggestions\n";
    prompt << "6. Detailed reviewer comments\n";

    if (request.includeComparison) {
        prompt << "7. Comparison with similar papers in the field\n";
    }

    prompt << "\nPlease respond in JSON format.";

    return prompt.str();
}

std::vector<AIReviewResult> AiCoPilotModule::getReviewHistory(int userId, int page, int limit) {
    std::vector<AIReviewResult> results;

    std::ostringstream sql;
    sql << "SELECT * FROM ai_review_feedback WHERE user_id = " << userId
        << " ORDER BY created_at DESC LIMIT " << limit << " OFFSET " << ((page - 1) * limit);

    auto rows = database_->query(sql.str());

    // TODO: 解析行数据为AIReviewResult

    return results;
}

// ============================================================================
// 2. AI文献综述生成器实现
// ============================================================================

LiteratureReviewResult AiCoPilotModule::generateLiteratureReview(
    const LiteratureReviewRequest& request) {

    LiteratureReviewResult result;
    result.generatedAt = std::chrono::system_clock::now();

    try {
        // 1. 构建Prompt
        std::string prompt = buildLiteratureReviewPrompt(request);

        // 2. 调用AI
        if (impl_->aiWorkflow_) {
            RAGContext ragContext;
            ragContext.relevantPapers.reserve(request.paperIds.size());
            for (int paperId : request.paperIds) {
                ragContext.relevantPapers.push_back(std::to_string(paperId));
            }

            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt,
                AIModelType::GPT_4, // 文献综述需要高质量模型
                ragContext,
                request.userId
            );

            if (aiResult.success) {
                result.success = true;
                result.title = request.title;
                result.paperCount = request.paperIds.size();
                result.costUsd = aiResult.costUsd;
                result.reviewContent = aiResult.content;

                // TODO: 解析结构化数据
                result.researchGaps = {"Gap 1", "Gap 2"};
                result.trends = {"Trend 1", "Trend 2"};

                // 保存到数据库
                std::ostringstream insertSql;
                insertSql << "INSERT INTO literature_reviews "
                    << "(user_id, title, research_field, paper_ids, paper_count, review_content) VALUES ("
                    << request.userId << ", "
                    << "\"" << escapeSql(request.title) << "\", "
                    << "\"" << escapeSql(request.researchField) << "\", "
                    << "'[\"ids...\"]', "
                    << request.paperIds.size() << ", "
                    << "\"" << escapeSql(result.reviewContent) << "\")";

                database_->execute(insertSql.str());

                impl_->totalLiteratureReviews_++;
            }
        }

    } catch (const std::exception& e) {
        result.success = false;
    }

    return result;
}

std::string AiCoPilotModule::buildLiteratureReviewPrompt(const LiteratureReviewRequest& request) {
    std::ostringstream prompt;

    prompt << "Generate a comprehensive literature review on: " << request.title << "\n\n";
    prompt << "Field: " << request.researchField << "\n";
    prompt << "Number of papers: " << request.paperIds.size() << "\n\n";

    prompt << "Please include:\n";
    if (request.includeGaps) {
        prompt << "- Research gaps and limitations\n";
    }
    if (request.includeTrends) {
        prompt << "- Current trends and future directions\n";
    }
    if (request.includeMethodology) {
        prompt << "- Methodology summary across studies\n";
    }

    prompt << "- Key findings and contributions\n";
    prompt << "\nMax length: " << request.maxLength << " words\n";
    prompt << "\nPlease provide a well-structured, academic literature review.";

    return prompt.str();
}

// ============================================================================
// 3. AI研究规划助手实现
// ============================================================================

ResearchPlanResult AiCoPilotModule::generateResearchPlan(const ResearchPlanRequest& request) {
    ResearchPlanResult result;
    result.plannedAt = std::chrono::system_clock::now();

    try {
        // 1. 构建Prompt
        std::string prompt = buildResearchPlanPrompt(request);

        // 2. 调用AI
        if (impl_->aiWorkflow_) {
            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt,
                AIModelType::GPT_4,
                std::nullopt, // 不需要RAG
                request.userId
            );

            if (aiResult.success) {
                result.success = true;
                result.title = request.title;
                result.researchQuestion = request.researchQuestion;
                result.costUsd = aiResult.costUsd;

                // TODO: 解析结构化数据
                result.objectives = {"Objective 1", "Objective 2"};
                result.feasibilityScore = 7;
                result.innovationScore = 8;

                // 保存到数据库
                std::ostringstream insertSql;
                insertSql << "INSERT INTO research_plans "
                    << "(user_id, title, research_question, feasibility_score, innovation_score) VALUES ("
                    << request.userId << ", "
                    << "\"" << escapeSql(request.title) << "\", "
                    << "\"" << escapeSql(request.researchQuestion) << "\", "
                    << result.feasibilityScore << ", "
                    << result.innovationScore << ")";

                database_->execute(insertSql.str());

                impl_->totalResearchPlans_++;
            }
        }

    } catch (const std::exception& e) {
        result.success = false;
    }

    return result;
}

std::string AiCoPilotModule::buildResearchPlanPrompt(const ResearchPlanRequest& request) {
    std::ostringstream prompt;

    prompt << "Create a detailed research plan for the following project:\n\n";
    prompt << "Title: " << request.title << "\n";
    prompt << "Research Question: " << request.researchQuestion << "\n";
    prompt << "Field: " << request.researchField << "\n";
    prompt << "Duration: " << request.durationMonths << " months\n";
    prompt << "Budget: " << request.budgetLevel << "\n\n";

    prompt << "Please provide:\n";
    prompt << "1. Specific research objectives (3-5)\n";
    prompt << "2. Detailed methodology\n";
    prompt << "3. Timeline with milestones\n";
    prompt << "4. Required resources (equipment, personnel, data)\n";
    prompt << "5. Potential challenges and mitigation strategies\n";
    prompt << "6. Expected outcomes and impact\n";
    prompt << "7. Feasibility score (1-10) with justification\n";
    prompt << "8. Innovation score (1-10) with justification\n";

    return prompt.str();
}

// ============================================================================
// 4. 多轮对话式智能助手实现
// ============================================================================

std::string AiCoPilotModule::chat(int userId, const std::string& message, const std::string& sessionId) {
    std::string actualSessionId = sessionId;

    // 如果没有sessionId，生成新的
    if (actualSessionId.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000, 999999);
        actualSessionId = "session_" + std::to_string(userId) + "_" + std::to_string(dis(gen));
    }

    // 添加到对话历史
    impl_->conversationHistory_[actualSessionId].push_back("User: " + message);

    // 构建Prompt（包含对话历史）
    std::ostringstream prompt;
    prompt << "You are an AI research assistant. Help the user with their academic research.\n\n";
    prompt << "Conversation history:\n";
    for (const auto& msg : impl_->conversationHistory_[actualSessionId]) {
        prompt << msg << "\n";
    }
    prompt << "\nUser's latest message: " << message;

    // 调用AI
    if (impl_->aiWorkflow_) {
        // 构建RAG上下文（用户的研究兴趣）
        RAGContext ragContext = impl_->aiWorkflow_->buildRAGContext(message, userId);

        auto aiResult = impl_->aiWorkflow_->executeAIRequest(
            prompt.str(),
            AIModelType::GPT_4_MINI, // 对话可以使用较便宜的模型
            ragContext,
            userId
        );

        if (aiResult.success) {
            std::string response = aiResult.content;

            // 添加AI响应到历史
            impl_->conversationHistory_[actualSessionId].push_back("Assistant: " + response);

            // 保存到数据库
            // TODO: INSERT INTO ai_conversations ...

            impl_->totalChats_++;

            return response;
        }
    }

    return "Sorry, I'm having trouble processing your request right now.";
}

// ============================================================================
// HTTP处理器实现
// ============================================================================

std::string AiCoPilotModule::handleGenerateReview(const std::string& body) {
    // TODO: 解析JSON请求体
    // 简化版：假设已解析为AIReviewRequest

    AIReviewRequest request;
    request.paperId = 1; // 示例
    request.userId = 1;
    request.targetJournal = "Nature";
    request.includeComparison = true;

    auto result = generateReview(request);

    // 构建JSON响应
    std::ostringstream json;
    json << "{";
    json << "\"success\":" << (result.success ? "true" : "false") << ",";
    if (result.success) {
        json << "\"reviewScore\":" << result.reviewScore << ",";
        json << "\"acceptanceProbability\":" << result.acceptanceProbability << ",";
        json << "\"reviewerComments\":\"" << escapeJson(result.reviewerComments) << "\",";
        json << "\"costUsd\":" << result.costUsd;
    }
    json << "}";

    return json.str();
}

// 其他HTTP处理器的实现类似...
// (省略以节省空间)

std::map<std::string, std::string> AiCoPilotModule::getUsageStats(int userId) {
    std::map<std::string, std::string> stats;

    stats["total_reviews"] = std::to_string(impl_->totalReviews_);
    stats["total_literature_reviews"] = std::to_string(impl_->totalLiteratureReviews_);
    stats["total_research_plans"] = std::to_string(impl_->totalResearchPlans_);
    stats["total_chats"] = std::to_string(impl_->totalChats_);

    // 从数据库查询用户的详细使用统计
    // TODO: SELECT * FROM ai_usage_statistics WHERE user_id = userId

    return stats;
}

std::string AiCoPilotModule::escapeSql(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

std::string AiCoPilotModule::escapeJson(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

} // namespace PaperCrawler
