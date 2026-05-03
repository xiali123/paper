#include "business/UnifiedAIWorkflow.hpp"
#include "data/StringUtil.hpp"
// #include "core/EventDrivenIntegration.hpp"  // EventDrivenIntegration has missing dependencies
// #include "modules/LoggingModule.hpp"        // LoggingModule not implemented yet
#include "features/ai/VectorStore.hpp"
#include "features/ai/EmbeddingGenerator.hpp"
#include "data/PreparedStatement.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <iomanip>
#include <cstdlib>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

class UnifiedAIWorkflow::Impl {
public:
    std::shared_ptr<IDatabase> database_;
    // std::shared_ptr<CacheModule> cache_;  // CacheModule尚未实现，后续集成
    std::shared_ptr<Network::HttpClient> httpClient_;

    // 缓存统计
    uint64_t l1Hits_{0};
    uint64_t l2Hits_{0};
    uint64_t l3Hits_{0};
    uint64_t cacheMisses_{0};

    // 成本统计
    double totalCostUsd_{0.0};
    uint64_t totalRequests_{0};

    // 流式响应
    bool streamingEnabled_{false};
    std::function<void(const std::string&)> streamingCallback_;

    // API配置
    std::string openaiApiKey_;
    std::string openaiBaseUrl_{"https://api.openai.com/v1"};
    std::string claudeApiKey_;

    // L1内存缓存
    std::map<std::string, AIResult> l1Cache_;
    size_t l1CacheSize_{1000};
};

UnifiedAIWorkflow::UnifiedAIWorkflow()
    : impl_(std::make_unique<Impl>()) {
}

UnifiedAIWorkflow::~UnifiedAIWorkflow() = default;

bool UnifiedAIWorkflow::initialize() {
    // 解析服务
    impl_->database_ = Services::resolve<IDatabase>();
    // impl_->cache_ = Services::resolve<CacheModule>();  // CacheModule尚未实现，后续集成
    impl_->httpClient_ = Services::resolve<Network::HttpClient>();

    if (!impl_->database_) {
        // 记录错误
        spdlog::warn("[UnifiedAIWorkflow] Database not available");
    }

    // 从环境变量读取API密钥（可通过config.json或系统环境变量配置）
    const char* openaiKey = std::getenv("OPENAI_API_KEY");
    if (openaiKey && *openaiKey) {
        impl_->openaiApiKey_ = openaiKey;
    }
    const char* openaiBaseUrl = std::getenv("OPENAI_BASE_URL");
    if (openaiBaseUrl && *openaiBaseUrl) {
        impl_->openaiBaseUrl_ = openaiBaseUrl;
    }
    const char* claudeKey = std::getenv("CLAUDE_API_KEY");
    if (claudeKey && *claudeKey) {
        impl_->claudeApiKey_ = claudeKey;
    }

    if (impl_->openaiApiKey_.empty()) {
        spdlog::warn("[UnifiedAIWorkflow] OPENAI_API_KEY not set, cloud AI requests will fail");
    }
    if (impl_->claudeApiKey_.empty()) {
        spdlog::warn("[UnifiedAIWorkflow] CLAUDE_API_KEY not set, Claude requests will fail");
    }

    // 预计算常见查询
    precomputeCommonQueries();

    // 订阅事件
    // EventDrivenIntegration依赖尚未就绪，取消注释即可启用事件订阅
    /*
    auto& eventBus = EventDrivenIntegration::getInstance();
    eventBus.subscribe(EventType::AI_REQUEST_SENT, "UnifiedAIWorkflow",
        [this](const Event& event) {
            // 记录AI请求
            impl_->totalRequests_++;
        }
    );
    */

    spdlog::info("[UnifiedAIWorkflow] Initialized with 3-level caching (L1 memory only)");

    return true;
}

bool UnifiedAIWorkflow::start() {
    // 启动后台任务（预计算缓存预热）
    return true;
}

bool UnifiedAIWorkflow::stop() {
    return true;
}

void UnifiedAIWorkflow::cleanup() {
    impl_->l1Cache_.clear();
}

AIResult UnifiedAIWorkflow::executeAIRequest(
    const std::string& prompt,
    AIModelType model,
    const std::optional<RAGContext>& ragContext,
    int userId) {

    // 1. 生成缓存键
    std::string cacheKey = generateCacheKey(prompt, model, ragContext);

    // 2. 检查L1缓存（内存）
    auto l1It = impl_->l1Cache_.find(cacheKey);
    if (l1It != impl_->l1Cache_.end()) {
        impl_->l1Hits_++;
        l1It->second.fromCache = true;
        return l1It->second;
    }

    // 3. 检查L2缓存（Redis）
    if (impl_->cache_) {
        std::string cachedResult = impl_->cache_->get("ai_cache:" + cacheKey);
        if (!cachedResult.empty()) {
            impl_->l2Hits_++;
            try {
                auto j = nlohmann::json::parse(cachedResult);
                AIResult result;
                result.success = j.value("success", false);
                result.content = j.value("content", "");
                result.errorMessage = j.value("errorMessage", "");
                result.tokensUsed = j.value("tokensUsed", 0);
                result.costUsd = j.value("costUsd", 0.0);
                result.model = j.value("model", "");
                result.fromCache = true;
                spdlog::debug("[UnifiedAIWorkflow] L2 cache hit for key: {}", cacheKey);
                return result;
            } catch (const nlohmann::json::exception& e) {
                spdlog::warn("[UnifiedAIWorkflow] Failed to deserialize L2 cache entry: {}", e.what());
            }
        }
    }

    // 4. 检查L3缓存（预计算）
    if (impl_->database_) {
        try {
            PreparedStatement stmt(impl_->database_,
                "SELECT content, model, tokens_used, cost_usd FROM ai_precomputed_cache WHERE cache_key = ?");
            stmt.bind(1, cacheKey);
            auto rows = stmt.query();
            if (!rows.empty()) {
                const auto& row = rows[0];
                impl_->l3Hits_++;
                AIResult result;
                result.success = true;
                result.content = row.count("content") ? row.at("content") : "";
                result.model = row.count("model") ? row.at("model") : "";
                result.tokensUsed = row.count("tokens_used") ? std::stoi(row.at("tokens_used")) : 0;
                result.costUsd = row.count("cost_usd") ? std::stod(row.at("cost_usd")) : 0.0;
                result.timestamp = std::chrono::system_clock::now();
                result.fromCache = true;
                spdlog::debug("[UnifiedAIWorkflow] L3 cache hit for key: {}", cacheKey);
                return result;
            }
        } catch (const std::exception& e) {
            spdlog::debug("[UnifiedAIWorkflow] L3 cache query failed (table may not exist): {}", e.what());
        }
    }

    // 缓存未命中，执行AI请求
    impl_->cacheMisses_++;

    // 5. 构建完整Prompt（RAG增强）
    std::string fullPrompt = prompt;
    if (ragContext.has_value()) {
        fullPrompt = enhancePromptWithContext(prompt, ragContext.value());
    }

    // 6. 智能模型选择
    AIModelType selectedModel = model;
    if (model == AIModelType::GPT_4_MINI) {
        selectedModel = intelligentModelSelection(fullPrompt);
    }

    // 7. 调用AI
    AIResult result;
    if (selectedModel == AIModelType::LOCAL_MODEL) {
        result = callLocalModel(fullPrompt);
    } else {
        result = callCloudAPI(fullPrompt, selectedModel);
    }

    // 8. 保存到缓存
    saveToCache(cacheKey, result);

    // 9. 更新统计
    impl_->totalCostUsd_ += result.costUsd;

    return result;
}

std::string UnifiedAIWorkflow::generateCacheKey(
    const std::string& prompt,
    AIModelType model,
    const std::optional<RAGContext>& context) {

    std::ostringstream oss;
    oss << static_cast<int>(model) << ":";

    // 简单哈希（生产环境应使用SHA256）
    std::hash<std::string> hasher;
    oss << hasher(prompt);

    if (context.has_value()) {
        oss << ":";
        for (const auto& paperId : context->relevantPapers) {
            oss << paperId << ",";
        }
    }

    return oss.str();
}

void UnifiedAIWorkflow::saveToCache(const std::string& cacheKey, const AIResult& result) {
    // 保存到L1缓存（内存）
    if (impl_->l1Cache_.size() >= impl_->l1CacheSize_) {
        // LRU淘汰：移除最旧的条目
        impl_->l1Cache_.erase(impl_->l1Cache_.begin());
    }
    impl_->l1Cache_[cacheKey] = result;

    // 保存到L2缓存（Redis）
    if (impl_->cache_) {
        try {
            nlohmann::json j;
            j["success"] = result.success;
            j["content"] = result.content;
            j["errorMessage"] = result.errorMessage;
            j["tokensUsed"] = result.tokensUsed;
            j["costUsd"] = result.costUsd;
            j["model"] = result.model;
            impl_->cache_->set("ai_cache:" + cacheKey, j.dump(), 3600); // 1小时TTL
        } catch (const std::exception& e) {
            spdlog::warn("[UnifiedAIWorkflow] Failed to serialize AIResult to L2 cache: {}", e.what());
        }
    }

    // 保存到L3缓存（数据库预计算）
    if (impl_->database_ && result.success) {
        try {
            PreparedStatement stmt(impl_->database_,
                "INSERT OR REPLACE INTO ai_precomputed_cache (cache_key, content, model, tokens_used, cost_usd, created_at) "
                "VALUES (?, ?, ?, ?, ?, datetime('now'))");
            stmt.bind(1, cacheKey);
            stmt.bind(2, result.content);
            stmt.bind(3, result.model);
            stmt.bind(4, result.tokensUsed);
            stmt.bind(5, result.costUsd);
            stmt.execute();
        } catch (const std::exception& e) {
            spdlog::debug("[UnifiedAIWorkflow] L3 cache insert failed (table may not exist): {}", e.what());
        }
    }
}

std::map<std::string, std::string> UnifiedAIWorkflow::getCacheStats() {
    std::map<std::string, std::string> stats;

    uint64_t totalHits = impl_->l1Hits_ + impl_->l2Hits_ + impl_->l3Hits_;
    uint64_t totalAccess = totalHits + impl_->cacheMisses_;
    double hitRate = totalAccess > 0 ? (double)totalHits / totalAccess : 0.0;

    stats["l1_hits"] = std::to_string(impl_->l1Hits_);
    stats["l2_hits"] = std::to_string(impl_->l2Hits_);
    stats["l3_hits"] = std::to_string(impl_->l3Hits_);
    stats["cache_misses"] = std::to_string(impl_->cacheMisses_);
    stats["hit_rate"] = std::to_string(hitRate * 100) + "%";
    stats["l1_cache_size"] = std::to_string(impl_->l1Cache_.size());

    return stats;
}

RAGContext UnifiedAIWorkflow::buildRAGContext(const std::string& query, int userId) {
    RAGContext context;

    try {
        // 1. 生成查询嵌入向量
        EmbeddingGenerator embeddingGen;
        std::vector<float> queryVector = embeddingGen.generate(query);

        // 2. 向量搜索相关论文
        VectorStore vectorStore;
        if (impl_->database_) {
            vectorStore.setDatabase(impl_->database_);
            vectorStore.initializeTable();

            // 首次使用时自动索引论文（仅当嵌入表为空时）
            auto existingRows = impl_->database_->query(
                "SELECT COUNT(*) as cnt FROM paper_embeddings");
            int count = 0;
            if (!existingRows.empty() && existingRows[0].count("cnt")) {
                try { count = std::stoi(existingRows[0].at("cnt")); } catch (...) { spdlog::warn("[UnifiedAIWorkflow] Failed to parse count from cache"); }
            }
            if (count == 0) {
                spdlog::info("[UnifiedAIWorkflow] Embeddings table empty, indexing papers...");
                vectorStore.indexPapersFromDatabase(impl_->database_, 1000);
            }

            // 搜索相似论文（余弦相似度 >= 0.3，取 top 5）
            auto results = vectorStore.search(queryVector, 5, 0.3f);
            for (const auto& result : results) {
                context.relevantPapers.push_back(result.id);
                // 提取标题作为知识图谱实体候选
                auto titleIt = result.metadata.find("title");
                if (titleIt != result.metadata.end() && !titleIt->second.empty()) {
                    context.knowledgeGraphEntities.push_back(titleIt->second);
                }
            }
        }

        // 3. 获取用户上下文（书签、阅读历史、研究兴趣）
        if (impl_->database_ && userId > 0) {
            // 用户研究兴趣
            try {
                auto interests = impl_->database_->query(
                    "SELECT interest FROM user_research_interests WHERE user_id = " + std::to_string(userId));
                for (const auto& row : interests) {
                    if (row.count("interest")) {
                        context.userContext["research_interest"] += row.at("interest") + "; ";
                    }
                }
            } catch (...) {
                // Table may not exist yet
            }

            // 用户书签
            try {
                auto bookmarks = impl_->database_->query(
                    "SELECT paper_id FROM user_bookmarks WHERE user_id = " + std::to_string(userId) +
                    " ORDER BY created_at DESC LIMIT 10");
                std::string bookmarkIds;
                for (const auto& row : bookmarks) {
                    if (row.count("paper_id")) {
                        bookmarkIds += row.at("paper_id") + ",";
                    }
                }
                if (!bookmarkIds.empty()) {
                    context.userContext["bookmarked_papers"] = bookmarkIds;
                }
            } catch (...) {
                // Table may not exist yet
            }

            // 用户阅读历史
            try {
                auto history = impl_->database_->query(
                    "SELECT paper_id FROM user_reading_history WHERE user_id = " + std::to_string(userId) +
                    " ORDER BY read_at DESC LIMIT 10");
                std::string historyIds;
                for (const auto& row : history) {
                    if (row.count("paper_id")) {
                        historyIds += row.at("paper_id") + ",";
                    }
                }
                if (!historyIds.empty()) {
                    context.userContext["recently_read"] = historyIds;
                }
            } catch (...) {
                // Table may not exist yet
            }
        }

        // 4. 获取对话历史
        if (impl_->database_ && userId > 0) {
            try {
                auto conversations = impl_->database_->query(
                    "SELECT role, content FROM ai_conversations WHERE user_id = " + std::to_string(userId) +
                    " ORDER BY created_at DESC LIMIT 10");
                // Reverse to get chronological order
                for (auto it = conversations.rbegin(); it != conversations.rend(); ++it) {
                    const auto& row = *it;
                    std::string role = row.count("role") ? row.at("role") : "user";
                    std::string content = row.count("content") ? row.at("content") : "";
                    if (!content.empty()) {
                        context.conversationHistory.push_back("[" + role + "] " + content);
                    }
                }
            } catch (...) {
                // Table may not exist yet
            }
        }

        spdlog::info("[UnifiedAIWorkflow] RAG context built: {} papers, {} entities, {} history messages, user context size: {}",
            context.relevantPapers.size(),
            context.knowledgeGraphEntities.size(),
            context.conversationHistory.size(),
            context.userContext.size());

    } catch (const std::exception& e) {
        spdlog::error("[UnifiedAIWorkflow] Failed to build RAG context: {}", e.what());
        // Return partial context -- graceful degradation
    }

    return context;
}

std::string UnifiedAIWorkflow::enhancePromptWithContext(
    const std::string& originalPrompt,
    const RAGContext& context) {

    std::ostringstream enhanced;
    enhanced << "Context:\n";

    // 添加相关论文（尝试从VectorStore获取完整内容）
    if (!context.relevantPapers.empty()) {
        enhanced << "\nRelevant Papers:\n";
        VectorStore vectorStore;
        if (impl_->database_) {
            vectorStore.setDatabase(impl_->database_);
        }
        for (const auto& paperId : context.relevantPapers) {
            auto embedding = vectorStore.getEmbedding(paperId);
            if (embedding.has_value() && !embedding->content.empty()) {
                // Truncate content to keep prompt size manageable
                std::string content = embedding->content;
                if (content.size() > 500) {
                    content = content.substr(0, 500) + "...";
                }
                enhanced << "- Paper " << paperId << ": " << content << "\n";
            } else {
                enhanced << "- Paper ID: " << paperId << "\n";
            }
        }
    }

    // 添加知识图谱实体
    if (!context.knowledgeGraphEntities.empty()) {
        enhanced << "\nRelated Concepts:\n";
        for (const auto& entity : context.knowledgeGraphEntities) {
            enhanced << "- " << entity << "\n";
        }
    }

    // 添加对话历史
    if (!context.conversationHistory.empty()) {
        enhanced << "\nConversation History:\n";
        for (const auto& msg : context.conversationHistory) {
            enhanced << msg << "\n";
        }
    }

    enhanced << "\nUser Question:\n" << originalPrompt << "\n";
    enhanced << "\nPlease answer based on the provided context.";

    return enhanced.str();
}

AIResult UnifiedAIWorkflow::callCloudAPI(const std::string& prompt, AIModelType model) {
    AIResult result;

    try {
        std::string response;
        std::string modelName;

        if (model == AIModelType::GPT_4 || model == AIModelType::GPT_4_MINI) {
            modelName = (model == AIModelType::GPT_4) ? "gpt-4" : "gpt-4.1-mini";
            response = callOpenAIAPI(prompt, modelName);
        } else if (model == AIModelType::CLAUDE_3_5_SONNET) {
            modelName = "claude-3-5-sonnet-20241022";
            response = callClaudeAPI(prompt, modelName);
        } else {
            result.success = false;
            result.errorMessage = "Unknown model type";
            return result;
        }

        result.success = true;
        result.content = response;
        result.model = modelName;
        result.timestamp = std::chrono::system_clock::now();
        result.fromCache = false;

        // 估算token和成本
        // GPT-4: $0.03/1K input tokens, $0.06/1K output tokens
        // GPT-4 Mini: $0.0015/1K input, $0.002/1K output
        int estimatedTokens = prompt.length() / 4 + response.length() / 4;
        result.tokensUsed = estimatedTokens;

        if (model == AIModelType::GPT_4) {
            result.costUsd = estimatedTokens * 0.06 / 1000;
        } else {
            result.costUsd = estimatedTokens * 0.002 / 1000;
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }

    return result;
}

std::string UnifiedAIWorkflow::callOpenAIAPI(const std::string& prompt, const std::string& model) {
    if (impl_->openaiApiKey_.empty()) {
        throw std::runtime_error("OpenAI API key not configured (set OPENAI_API_KEY environment variable)");
    }

    // Build request JSON using nlohmann/json for proper escaping
    nlohmann::json requestBody;
    requestBody["model"] = model;
    requestBody["messages"] = nlohmann::json::array({
        {{"role", "user"}, {"content", prompt}}
    });
    requestBody["temperature"] = 0.7;
    requestBody["max_tokens"] = 2000;

    std::string jsonBody = requestBody.dump();

    // Send HTTP request with Authorization header
    if (!impl_->httpClient_) {
        throw std::runtime_error("HttpClient not available");
    }

    impl_->httpClient_->setDefaultHeader("Content-Type", "application/json");
    impl_->httpClient_->setDefaultHeader("Authorization", "Bearer " + impl_->openaiApiKey_);

    auto response = impl_->httpClient_->post(
        impl_->openaiBaseUrl_ + "/chat/completions",
        jsonBody
    );

    if (response.statusCode != 200) {
        throw std::runtime_error("OpenAI API error: " + response.body);
    }

    // 解析响应JSON using nlohmann/json
    try {
        auto j = nlohmann::json::parse(response.body);

        // Extract content from OpenAI response structure:
        // {"choices": [{"message": {"content": "..."}}]}
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            const auto& choice = j["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                return choice["message"]["content"].get<std::string>();
            }
        }

        // Fallback: try "content" at top level
        if (j.contains("content") && j["content"].is_string()) {
            return j["content"].get<std::string>();
        }

        spdlog::warn("[UnifiedAIWorkflow] Unexpected OpenAI response structure: {}", response.body.substr(0, 200));
        return response.body;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[UnifiedAIWorkflow] Failed to parse OpenAI response JSON: {}", e.what());
        throw std::runtime_error("Failed to parse OpenAI response: " + std::string(e.what()));
    }
}

std::string UnifiedAIWorkflow::callClaudeAPI(const std::string& prompt, const std::string& model) {
    if (impl_->claudeApiKey_.empty()) {
        throw std::runtime_error("Claude API key not configured (set CLAUDE_API_KEY environment variable)");
    }

    // Build Claude Messages API request JSON
    nlohmann::json requestBody;
    requestBody["model"] = model;
    requestBody["max_tokens"] = 2000;
    requestBody["messages"] = nlohmann::json::array({
        {{"role", "user"}, {"content", prompt}}
    });

    std::string jsonBody = requestBody.dump();

    // Send HTTP request with Authorization header
    if (!impl_->httpClient_) {
        throw std::runtime_error("HttpClient not available");
    }

    impl_->httpClient_->setDefaultHeader("Content-Type", "application/json");
    impl_->httpClient_->setDefaultHeader("x-api-key", impl_->claudeApiKey_);
    impl_->httpClient_->setDefaultHeader("anthropic-version", "2023-06-01");

    auto response = impl_->httpClient_->post(
        "https://api.anthropic.com/v1/messages",
        jsonBody
    );

    if (response.statusCode != 200) {
        throw std::runtime_error("Claude API error (HTTP " + std::to_string(response.statusCode) + "): " + response.body);
    }

    // Parse Claude response JSON
    // {"content": [{"type": "text", "text": "..."}]}
    try {
        auto j = nlohmann::json::parse(response.body);

        if (j.contains("content") && j["content"].is_array() && !j["content"].empty()) {
            const auto& firstBlock = j["content"][0];
            if (firstBlock.contains("text") && firstBlock["text"].is_string()) {
                return firstBlock["text"].get<std::string>();
            }
        }

        spdlog::warn("[UnifiedAIWorkflow] Unexpected Claude response structure: {}", response.body.substr(0, 200));
        return response.body;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[UnifiedAIWorkflow] Failed to parse Claude response JSON: {}", e.what());
        throw std::runtime_error("Failed to parse Claude response: " + std::string(e.what()));
    }
}

AIResult UnifiedAIWorkflow::callLocalModel(const std::string& prompt) {
    AIResult result;

    // Local model integration point (llama.cpp, Ollama, etc.)
    // Check for local inference endpoint via environment variable
    const char* localModelUrl = std::getenv("LOCAL_MODEL_URL");
    if (localModelUrl && *localModelUrl && impl_->httpClient_) {
        try {
            nlohmann::json requestBody;
            requestBody["prompt"] = prompt;
            requestBody["max_tokens"] = 2000;
            requestBody["temperature"] = 0.7;

            impl_->httpClient_->setDefaultHeader("Content-Type", "application/json");
            auto response = impl_->httpClient_->post(
                std::string(localModelUrl) + "/generate",
                requestBody.dump()
            );

            if (response.statusCode == 200) {
                auto j = nlohmann::json::parse(response.body);
                result.success = true;
                result.content = j.value("content", j.value("response", j.value("text", "")));
                result.model = j.value("model", "local-inference");
                result.costUsd = 0.0;
                result.tokensUsed = j.value("tokens_evaluated", static_cast<int>(prompt.length() / 4));
                result.timestamp = std::chrono::system_clock::now();
                result.fromCache = false;
                spdlog::info("[UnifiedAIWorkflow] Local model responded, {} chars", result.content.size());
                return result;
            } else {
                spdlog::warn("[UnifiedAIWorkflow] Local model returned HTTP {}, falling back to stub", response.statusCode);
            }
        } catch (const std::exception& e) {
            spdlog::warn("[UnifiedAIWorkflow] Local model request failed: {}, falling back to stub", e.what());
        }
    } else {
        spdlog::debug("[UnifiedAIWorkflow] No LOCAL_MODEL_URL configured, using stub response");
    }

    // Placeholder stub when no local model is available
    result.success = true;
    result.content = "Local model response (stub - configure LOCAL_MODEL_URL for real inference)";
    result.model = "Local-Model";
    result.costUsd = 0.0;
    result.tokensUsed = static_cast<int>(prompt.length() / 4);
    result.timestamp = std::chrono::system_clock::now();
    result.fromCache = false;

    return result;
}

AIModelType UnifiedAIWorkflow::intelligentModelSelection(const std::string& content) {
    // 简单规则：根据内容长度和复杂度选择模型
    size_t length = content.length();

    if (length < 500) {
        // 简单查询，使用本地模型
        return AIModelType::LOCAL_MODEL;
    } else if (length < 2000) {
        // 中等复杂度，使用GPT-4 Mini
        return AIModelType::GPT_4_MINI;
    } else {
        // 复杂查询，使用GPT-4
        return AIModelType::GPT_4;
    }
}

void UnifiedAIWorkflow::precomputeCommonQueries() {
    // 预计算常见查询并缓存到L3
    // Query high-frequency queries from the database and pre-generate AI responses
    spdlog::info("[UnifiedAIWorkflow] Precomputing common AI queries...");

    if (impl_->database_) {
        try {
            auto rows = impl_->database_->query(
                "SELECT DISTINCT prompt, model_type FROM ai_query_log "
                "WHERE created_at > datetime('now', '-7 days') "
                "GROUP BY prompt ORDER BY COUNT(*) DESC LIMIT 20");

            if (rows.empty()) {
                spdlog::debug("[UnifiedAIWorkflow] No frequent queries found to precompute");
                return;
            }

            spdlog::info("[UnifiedAIWorkflow] Found {} frequent queries to precompute", rows.size());
            int precomputed = 0;

            for (const auto& row : rows) {
                try {
                    std::string prompt = row.count("prompt") ? row.at("prompt") : "";
                    int modelInt = row.count("model_type") ? std::stoi(row.at("model_type")) : 1;
                    AIModelType modelType = static_cast<AIModelType>(modelInt);

                    if (prompt.empty()) continue;

                    std::string cacheKey = generateCacheKey(prompt, modelType, std::nullopt);

                    // Skip if already cached
                    if (impl_->l1Cache_.find(cacheKey) != impl_->l1Cache_.end()) continue;

                    AIResult result = executeAIRequest(prompt, modelType);
                    if (result.success) {
                        precomputed++;
                    }
                } catch (const std::exception& e) {
                    spdlog::debug("[UnifiedAIWorkflow] Failed to precompute query: {}", e.what());
                }
            }

            spdlog::info("[UnifiedAIWorkflow] Precomputed {}/{} queries", precomputed, rows.size());
        } catch (const std::exception& e) {
            spdlog::debug("[UnifiedAIWorkflow] ai_query_log table may not exist: {}", e.what());
        }
    }
}

std::string UnifiedAIWorkflow::escapeJson(const std::string& str) {
    return StringUtil::escapeJson(str);
}

} // namespace PaperCrawler
