#include "business/AiCoPilotModule.hpp"
#include "core/Router.hpp"
#include "core/EventDrivenIntegration.hpp"
#include "modules/LoggingModule.hpp"
#include "prompts/AIPromptTemplates.hpp"
#include "business/AIResponseParser.hpp"
#include "data/PreparedStatement.hpp"
#include "features/security/SecurityModule.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstdint>

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

    // Auth middleware - check Authorization header
    auto requireAuth = [](const HttpRequest& req) -> bool {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) return false;

        const std::string& authHeader = authIt->second;
        if (authHeader.substr(0, 7) != "Bearer ") return false;

        std::string token = authHeader.substr(7);
        if (token.empty()) return false;

        SecurityModule sec;
        auto result = sec.verifyJWT(token);
        return result.valid;
    };

    auto unauthorizedResp = []() -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 401;
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"success":false,"message":"Unauthorized"})";
        return resp;
    };

    // 1. AI审稿人系统
    router.post(prefix + "/review", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleRequest(req); // 代理到handleRequest
    });

    router.get(prefix + "/reviews/:userId", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        // 处理获取审稿历史
        HttpResponse resp;
        resp.statusCode = 200;
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"success":true,"data":[]})";
        return resp;
    });

    // 2. 文献综述生成器
    router.post(prefix + "/literature-review/generate", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleRequest(req);
    });

    router.get(prefix + "/literature-reviews", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        // 处理获取文献综述列表
        HttpResponse resp;
        resp.statusCode = 200;
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"success":true,"data":[]})";
        return resp;
    });

    // 3. 研究规划助手
    router.post(prefix + "/research-plan/generate", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleRequest(req);
    });

    router.get(prefix + "/research-plans", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        // 处理获取研究计划列表
        HttpResponse resp;
        resp.statusCode = 200;
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"success":true,"data":[]})";
        return resp;
    });

    // 4. 对话助手
    router.post(prefix + "/chat", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleRequest(req);
    });

    router.get(prefix + "/conversations", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        // 处理获取对话列表
        HttpResponse resp;
        resp.statusCode = 200;
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"success":true,"data":[]})";
        return resp;
    });

    // 5. 研究建议
    router.get(prefix + "/recommendations", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleRequest(req);
    });

    // 6. 统计
    router.get(prefix + "/stats", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleRequest(req);
    });

    // 7. SSE streaming endpoint — streams AI response as SSE events
    router.get(prefix + "/stream", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleStreamRequest(req);
    });

    // 8. SSE stream status endpoint
    router.get(prefix + "/stream-status", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        return handleStreamStatus(req);
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
        PreparedStatement paperStmt(database_,
            "SELECT id, title, authors, abstract, content FROM papers WHERE id = ?");
        paperStmt.bind(0, request.paperId);
        auto papers = paperStmt.query();
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
            // 添加相关论文作为对比（待前端对接后从推荐模块获取）
            ragContext.relevantPapers = {}; // 从推荐模块获取

            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt,
                AIModelType::GPT_4,
                ragContext,
                request.userId
            );

            if (aiResult.success) {
                // 使用AIResponseParser解析AI响应（新功能）
                AIReviewResult parsedResult = AIResponseParser::parseReviewResponse(
                    aiResult.content,
                    request.paperId,
                    request.userId
                );

                if (parsedResult.success) {
                    // 解析成功，使用结构化数据
                    result = parsedResult;
                    result.success = true;
                    result.costUsd = aiResult.costUsd;

                    if (auto logging = Services::resolve<LoggingModule>()) {
                        logging->info("Successfully parsed AI review response with score: " +
                                    std::to_string(result.reviewScore));
                    }
                } else {
                    // 解析失败，使用原始响应作为comments
                    result.success = true;
                    result.costUsd = aiResult.costUsd;
                    result.reviewerComments = aiResult.content;
                    result.reviewScore = 5; // 默认中等评分
                    result.acceptanceProbability = 0.5f;

                    if (auto logging = Services::resolve<LoggingModule>()) {
                        logging->warn("Failed to parse AI review response, using raw content");
                    }
                }

                // 保存到数据库
                PreparedStatement insertStmt(database_,
                    "INSERT INTO ai_review_feedback "
                    "(paper_id, user_id, review_score, acceptance_probability, "
                    "improvement_suggestions, reviewer_comments) VALUES (?, ?, ?, ?, ?, ?)");
                insertStmt.bind(0, request.paperId)
                          .bind(1, request.userId)
                          .bind(2, result.reviewScore)
                          .bind(3, static_cast<double>(result.acceptanceProbability))
                          .bind(4, std::string("[\"suggestion1\", \"suggestion2\"]"))
                          .bind(5, result.reviewerComments);
                insertStmt.execute();

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

    // 使用新的AIPromptTemplates生成审稿Prompt（v2.0）
    ReviewPromptContext promptContext;
    promptContext.paperId = std::to_string(request.paperId);
    promptContext.paperTitle = paperData.count("title") ? paperData.at("title") : "";
    promptContext.paperAuthors = paperData.count("authors") ? paperData.at("authors") : "";
    promptContext.paperAbstract = paperData.count("abstract") ? paperData.at("abstract") : "";
    promptContext.paperContent = paperData.count("content") ? paperData.at("content") : "";
    promptContext.targetJournal = request.targetJournal;
    promptContext.researchField = request.researchField;
    promptContext.includeComparison = request.includeComparison;
    promptContext.includeMethodology = true;
    promptContext.includeReferences = true;
    promptContext.maxSuggestions = 5;

    // 根据用户偏好选择审稿风格（默认Balanced）
    ReviewPromptStyle style = ReviewPromptStyle::Balanced;
    // 未来可以根据用户设置或请求参数调整风格

    // 生成高质量的结构化Prompt
    std::string prompt = AIPromptTemplates::generateReviewPrompt(promptContext, style);

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("Generated AI review prompt using AIPromptTemplates v2.0");
        logging->debug("Prompt length: " + std::to_string(prompt.length()) + " characters");
    }

    return prompt;
}

std::vector<AIReviewResult> AiCoPilotModule::getReviewHistory(int userId, int page, int limit) {
    std::vector<AIReviewResult> results;

    int offset = (page - 1) * limit;
    PreparedStatement reviewStmt(database_,
        "SELECT * FROM ai_review_feedback WHERE user_id = ? "
        "ORDER BY created_at DESC LIMIT ? OFFSET ?");
    reviewStmt.bind(0, userId).bind(1, limit).bind(2, offset);
    auto rows = reviewStmt.query();

    for (const auto& row : rows) {
        AIReviewResult r;
        r.success = true;
        r.paperId = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
        r.reviewScore = row.count("review_score") ? std::stoi(row.at("review_score")) : 0;
        r.acceptanceProbability = row.count("acceptance_probability")
            ? std::stof(row.at("acceptance_probability")) : 0.0f;
        r.reviewerComments = row.count("reviewer_comments") ? row.at("reviewer_comments") : "";
        r.costUsd = row.count("cost_usd") ? std::stod(row.at("cost_usd")) : 0.0;

        // 解析improvement_suggestions（存储为JSON数组字符串）
        if (row.count("improvement_suggestions") && !row.at("improvement_suggestions").empty()) {
            try {
                auto suggestionsJson = json::parse(row.at("improvement_suggestions"));
                if (suggestionsJson.is_array()) {
                    for (const auto& s : suggestionsJson) {
                        r.improvements.push_back(s.get<std::string>());
                    }
                }
            } catch (...) {
                // JSON解析失败，保留空列表
            }
        }

        // created_at转time_point
        if (row.count("created_at")) {
            std::tm tm = {};
            std::istringstream iss(row.at("created_at"));
            iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (!iss.fail()) {
                r.reviewedAt = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
        }

        results.push_back(r);
    }

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
                // 使用AIResponseParser解析AI响应（新功能）
                LiteratureReviewResult parsedResult =
                    AIResponseParser::parseLiteratureReviewResponse(
                        aiResult.content,
                        request
                    );

                if (parsedResult.success) {
                    // 解析成功，使用结构化数据
                    result = parsedResult;
                    result.success = true;
                    result.title = request.title;
                    result.paperCount = request.paperIds.size();
                    result.costUsd = aiResult.costUsd;

                    if (auto logging = Services::resolve<LoggingModule>()) {
                        logging->info("Successfully parsed literature review with " +
                                    std::to_string(result.researchGaps.size()) + " gaps and " +
                                    std::to_string(result.trends.size()) + " trends");
                    }
                } else {
                    // 解析失败，使用原始响应
                    result.success = true;
                    result.title = request.title;
                    result.paperCount = request.paperIds.size();
                    result.costUsd = aiResult.costUsd;
                    result.reviewContent = aiResult.content;

                    if (auto logging = Services::resolve<LoggingModule>()) {
                        logging->warn("Failed to parse literature review response, using raw content");
                    }
                }

                // 保存到数据库
                PreparedStatement insertLitStmt(database_,
                    "INSERT INTO literature_reviews "
                    "(user_id, title, research_field, paper_ids, paper_count, review_content) "
                    "VALUES (?, ?, ?, ?, ?, ?)");
                insertLitStmt.bind(0, request.userId)
                              .bind(1, request.title)
                              .bind(2, request.researchField)
                              .bind(3, std::string("[\"ids...\"]"))
                              .bind(4, static_cast<int>(request.paperIds.size()))
                              .bind(5, result.reviewContent);
                insertLitStmt.execute();

                impl_->totalLiteratureReviews_++;
            }
        }

    } catch (const std::exception& e) {
        result.success = false;
    }

    return result;
}

std::string AiCoPilotModule::buildLiteratureReviewPrompt(const LiteratureReviewRequest& request) {
    // 使用新的AIPromptTemplates生成文献综述Prompt（v2.0）
    LiteratureReviewContext promptContext;
    promptContext.userId = std::to_string(request.userId);
    promptContext.title = request.title;
    promptContext.researchField = request.researchField;
    promptContext.paperIds = request.paperIds;
    promptContext.reviewType = "systematic"; // 默认系统性综述
    promptContext.maxLength = request.maxLength;
    promptContext.includeGaps = request.includeGaps;
    promptContext.includeTrends = request.includeTrends;
    promptContext.includeMethodology = request.includeMethodology;
    promptContext.includeKeyFindings = true;
    promptContext.includeFutureDirections = true;
    promptContext.themeCount = 5;

    // 生成高质量的结构化Prompt
    std::string prompt = AIPromptTemplates::generateLiteratureReviewPrompt(promptContext);

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("Generated literature review prompt using AIPromptTemplates v2.0");
        logging->debug("Prompt length: " + std::to_string(prompt.length()) + " characters");
    }

    return prompt;
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
                // 使用AIResponseParser解析AI响应（新功能）
                ResearchPlanResult parsedResult =
                    AIResponseParser::parseResearchPlanResponse(
                        aiResult.content,
                        request
                    );

                if (parsedResult.success) {
                    // 解析成功，使用结构化数据
                    result = parsedResult;
                    result.success = true;
                    result.title = request.title;
                    result.researchQuestion = request.researchQuestion;
                    result.costUsd = aiResult.costUsd;

                    if (auto logging = Services::resolve<LoggingModule>()) {
                        logging->info("Successfully parsed research plan with " +
                                    std::to_string(result.objectives.size()) + " objectives, " +
                                    "feasibility: " + std::to_string(result.feasibilityScore) + "/10, " +
                                    "innovation: " + std::to_string(result.innovationScore) + "/10");
                    }
                } else {
                    // 解析失败，使用原始响应
                    result.success = true;
                    result.title = request.title;
                    result.researchQuestion = request.researchQuestion;
                    result.costUsd = aiResult.costUsd;
                    result.methodology = aiResult.content; // 保存原始响应

                    if (auto logging = Services::resolve<LoggingModule>()) {
                        logging->warn("Failed to parse research plan response, using raw content");
                    }
                }

                // 保存到数据库
                PreparedStatement insertPlanStmt(database_,
                    "INSERT INTO research_plans "
                    "(user_id, title, research_question, feasibility_score, innovation_score) "
                    "VALUES (?, ?, ?, ?, ?)");
                insertPlanStmt.bind(0, request.userId)
                               .bind(1, request.title)
                               .bind(2, request.researchQuestion)
                               .bind(3, result.feasibilityScore)
                               .bind(4, result.innovationScore);
                insertPlanStmt.execute();

                impl_->totalResearchPlans_++;
            }
        }

    } catch (const std::exception& e) {
        result.success = false;
    }

    return result;
}

std::string AiCoPilotModule::buildResearchPlanPrompt(const ResearchPlanRequest& request) {
    // 使用新的AIPromptTemplates生成研究计划Prompt（v2.0）
    ResearchPlanContext promptContext;
    promptContext.userId = std::to_string(request.userId);
    promptContext.title = request.title;
    promptContext.researchQuestion = request.researchQuestion;
    promptContext.researchField = request.researchField;
    promptContext.keywords = {}; // 从request中获取
    promptContext.durationMonths = request.durationMonths;
    promptContext.budgetLevel = request.budgetLevel;
    promptContext.includeTimeline = true;
    promptContext.includeBudget = true;
    promptContext.includeRisks = true;
    promptContext.includeTeam = true;
    promptContext.includeEthics = true;

    // 生成高质量的结构化Prompt
    std::string prompt = AIPromptTemplates::generateResearchPlanPrompt(promptContext);

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("Generated research plan prompt using AIPromptTemplates v2.0");
        logging->debug("Prompt length: " + std::to_string(prompt.length()) + " characters");
    }

    return prompt;
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
            try {
                auto now = std::chrono::system_clock::now();
                auto now_time_t = std::chrono::system_clock::to_time_t(now);
                std::ostringstream timeStr;
                timeStr << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");

                PreparedStatement stmt(database_,
                    "INSERT INTO ai_conversations (user_id, session_id, role, content, created_at) "
                    "VALUES (?, ?, ?, ?, ?)");
                stmt.bind(0, userId);
                stmt.bind(1, actualSessionId);
                stmt.bind(2, std::string("user"));
                stmt.bind(3, message);
                stmt.bind(4, timeStr.str());
                stmt.execute();

                stmt.clear();
                stmt.bind(0, userId);
                stmt.bind(1, actualSessionId);
                stmt.bind(2, std::string("assistant"));
                stmt.bind(3, response);
                stmt.bind(4, timeStr.str());
                stmt.execute();
            } catch (const std::exception& e) {
                if (auto logging = Services::resolve<LoggingModule>()) {
                    logging->error("Failed to persist conversation: " + std::string(e.what()));
                }
            }

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
    // 解析JSON请求体（当前使用硬编码示例数据）
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
    try {
        // 审稿统计
        PreparedStatement reviewStmt(database_,
            "SELECT COUNT(*) AS cnt, AVG(review_score) AS avg_score "
            "FROM ai_review_feedback WHERE user_id = ?");
        reviewStmt.bind(0, userId);
        auto reviewRows = reviewStmt.query();
        if (!reviewRows.empty()) {
            stats["db_review_count"] = reviewRows[0].count("cnt") ? reviewRows[0].at("cnt") : "0";
            stats["db_avg_review_score"] = reviewRows[0].count("avg_score") ? reviewRows[0].at("avg_score") : "0";
        }

        // 文献综述统计
        PreparedStatement litStmt(database_,
            "SELECT COUNT(*) AS cnt FROM literature_reviews WHERE user_id = ?");
        litStmt.bind(0, userId);
        auto litRows = litStmt.query();
        if (!litRows.empty()) {
            stats["db_literature_review_count"] = litRows[0].count("cnt") ? litRows[0].at("cnt") : "0";
        }

        // 研究计划统计
        PreparedStatement planStmt(database_,
            "SELECT COUNT(*) AS cnt FROM research_plans WHERE user_id = ?");
        planStmt.bind(0, userId);
        auto planRows = planStmt.query();
        if (!planRows.empty()) {
            stats["db_research_plan_count"] = planRows[0].count("cnt") ? planRows[0].at("cnt") : "0";
        }

        // 对话统计
        PreparedStatement chatStmt(database_,
            "SELECT COUNT(DISTINCT session_id) AS session_count, COUNT(*) AS message_count "
            "FROM ai_conversations WHERE user_id = ?");
        chatStmt.bind(0, userId);
        auto chatRows = chatStmt.query();
        if (!chatRows.empty()) {
            stats["db_session_count"] = chatRows[0].count("session_count") ? chatRows[0].at("session_count") : "0";
            stats["db_message_count"] = chatRows[0].count("message_count") ? chatRows[0].at("message_count") : "0";
        }
    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to query usage statistics: " + std::string(e.what()));
        }
    }

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

// ============================================================================
// 5. SSE Streaming 实现
// ============================================================================

HttpResponse AiCoPilotModule::handleStreamRequest(const HttpRequest& req) {
    // Extract prompt from query params
    std::string prompt = req.getQuery("prompt", "");
    if (prompt.empty()) {
        HttpResponse resp;
        resp.statusCode = 400;
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"success":false,"message":"Missing 'prompt' query parameter"})";
        return resp;
    }

    spdlog::info("[AiCoPilot] SSE stream request received, prompt length: {}",
                 prompt.size());

    // Generate a unique connection ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(1, UINT64_MAX);
    std::string connectionId = "sse_" + std::to_string(dis(gen));

    // Build the HttpResponse with SSE headers.
    // The body will contain all SSE events concatenated because the underlying
    // HTTP infrastructure is synchronous (no chunked transfer).  We simulate
    // streaming by chunking the full AI response into discrete SSE events
    // written into the response body.
    HttpResponse resp;
    resp.statusCode = 200;
    resp.headers["Content-Type"] = "text/event-stream";
    resp.headers["Cache-Control"] = "no-cache";
    resp.headers["Connection"] = "keep-alive";
    resp.headers["X-Accel-Buffering"] = "no";  // Disable nginx buffering

    // ---- Obtain the full AI response synchronously ----
    std::string aiFullResponse;
    bool aiSuccess = false;

    try {
        if (impl_->aiWorkflow_) {
            // Determine user id from query (default 0 for anonymous)
            int userId = 0;
            std::string userIdStr = req.getQuery("userId", "0");
            try { userId = std::stoi(userIdStr); } catch (...) {}

            // Determine model type (default GPT_4_MINI for streaming chat)
            RAGContext ragContext = impl_->aiWorkflow_->buildRAGContext(prompt, userId);

            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt,
                AIModelType::GPT_4_MINI,
                ragContext,
                userId
            );

            if (aiResult.success) {
                aiFullResponse = aiResult.content;
                aiSuccess = true;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AiCoPilot] SSE stream AI call failed: {}", e.what());
        aiFullResponse = "Error generating AI response: " + std::string(e.what());
    }

    if (!aiSuccess && aiFullResponse.empty()) {
        aiFullResponse = "Sorry, I could not generate a response at this time.";
    }

    // ---- Chunk the response and write SSE events into body ----
    activeStreamCount_++;
    totalStreamedRequests_++;

    // Use a string stream to build all SSE events
    std::ostringstream sseBody;

    // 1. Send an initial "connected" event
    {
        SseEvent connectedEvent;
        connectedEvent.id = "0";
        connectedEvent.event = "connected";
        connectedEvent.data = "{\"connectionId\":\"" + connectionId + "\"}";
        sseBody << connectedEvent.format();
    }

    // 2. Chunk the AI response into word-level SSE events
    streamAiResponseInto(connectionId, aiFullResponse, sseBody);

    // 3. Send the [DONE] sentinel event
    {
        SseEvent doneEvent;
        doneEvent.id = "done";
        doneEvent.event = "done";
        doneEvent.data = "[DONE]";
        sseBody << doneEvent.format();
    }

    resp.body = sseBody.str();

    activeStreamCount_--;

    spdlog::info("[AiCoPilot] SSE stream completed for connection {}, body size: {} bytes",
                 connectionId, resp.body.size());

    return resp;
}

void AiCoPilotModule::streamAiResponseInto(
    const std::string& connectionId,
    const std::string& fullResponse,
    std::ostringstream& sseBody) {

    // Chunk the response into segments of approximately chunkSize characters,
    // splitting at word boundaries to avoid breaking mid-word.
    const size_t chunkSize = 20;  // characters per chunk
    size_t pos = 0;
    int eventId = 1;

    while (pos < fullResponse.size()) {
        size_t end = std::min(pos + chunkSize, fullResponse.size());

        // Try to extend to the next space or newline to avoid splitting mid-word
        if (end < fullResponse.size()) {
            size_t spacePos = fullResponse.find_first_of(" \n\r\t", end);
            if (spacePos != std::string::npos && spacePos <= end + 15) {
                end = spacePos + 1;
            }
        }

        std::string chunk = fullResponse.substr(pos, end - pos);

        // Build a JSON payload for each chunk
        std::string jsonData = "{\"content\":\"" + escapeJson(chunk) + "\"}";

        SseEvent chunkEvent;
        chunkEvent.id = std::to_string(eventId++);
        chunkEvent.event = "delta";
        chunkEvent.data = jsonData;
        sseBody << chunkEvent.format();

        pos = end;
    }
}

HttpResponse AiCoPilotModule::handleStreamStatus(const HttpRequest& req) {
    HttpResponse resp;
    resp.statusCode = 200;
    resp.headers["Content-Type"] = "application/json";

    std::ostringstream json;
    json << "{";
    json << "\"success\":true,";
    json << "\"activeStreamCount\":" << activeStreamCount_.load() << ",";
    json << "\"totalStreamedRequests\":" << totalStreamedRequests_.load() << ",";
    json << "\"activeSseConnections\":" << sseBroadcaster_.connectionCount();
    json << "}";

    resp.body = json.str();

    spdlog::debug("[AiCoPilot] Stream status queried: active={}, total={}, connections={}",
                  activeStreamCount_.load(), totalStreamedRequests_.load(),
                  sseBroadcaster_.connectionCount());

    return resp;
}

} // namespace PaperCrawler
