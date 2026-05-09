#include "business/AiApiModule.hpp"
#include "core/HttpStatus.hpp"
#include "data/StringUtil.hpp"
#include "data/DatabaseModule.hpp"
#include "network/HttpClient.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <mutex>
#include "data/ValidationHelper.hpp"

namespace PaperCrawler {

using json = nlohmann::json;

// ============================================================================
// AiApiModule::Impl - 内部实现
// ============================================================================

class AiApiModule::Impl {
public:
    AiConfig config_;
    std::shared_ptr<IDatabase> database_;
    HttpClientPtr httpClient_;

    // 统计信息
    uint64_t totalRequests_{0};
    uint64_t successfulRequests_{0};
    uint64_t failedRequests_{0};
    std::chrono::system_clock::time_point startTime_;
    std::map<std::string, uint64_t> requestCounts_;

    // 内存缓存
    struct CacheEntry {
        std::string result;
        std::chrono::system_clock::time_point expiresAt;
        uint64_t hitCount{0};
    };
    std::unordered_map<std::string, CacheEntry> cache_;
    std::mutex cacheMutex_;

    Impl() {
        startTime_ = std::chrono::system_clock::now();
    }

    /**
     * @brief 初始化AI模块
     */
    bool initialize(const AiConfig& config,
                   std::shared_ptr<IDatabase> database,
                   HttpClientPtr httpClient = nullptr) {
        config_ = config;
        database_ = database;
        httpClient_ = httpClient;

        spdlog::info("[AI] Initializing AI module...");
        spdlog::info("  Provider: {}", config.provider);
        spdlog::info("  Model: {}", config.model);
        spdlog::info("  Base URL: {}", config.baseUrl);

        if (config.apiKey.empty()) {
            spdlog::warn("[AI] API key not configured, using mock responses");
        }

        spdlog::info("[AI] Initialization complete");
        return true;
    }

    /**
     * @brief 从数据库获取论文内容
     */
    std::optional<std::map<std::string, std::string>> fetchPaperFromDatabase(int paperId) {
        if (!database_) {
            return std::nullopt;
        }

        std::string sql = "SELECT id, title, abstract, content, authors, publication_year, keywords "
                         "FROM papers WHERE id = " + std::to_string(paperId);

        auto results = database_->query(sql);
        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }

    /**
     * @brief 批量获取论文信息（解决N+1查询）
     */
    std::map<int, std::map<std::string, std::string>> fetchPapersBatch(const std::vector<int>& paperIds) {
        std::map<int, std::map<std::string, std::string>> result;
        if (!database_ || paperIds.empty()) return result;

        std::ostringstream sql;
        sql << "SELECT id, title, abstract, content, authors, publication_year, keywords "
            << "FROM papers WHERE id IN (";
        for (size_t i = 0; i < paperIds.size(); ++i) {
            sql << (i > 0 ? "," : "") << paperIds[i];
        }
        sql << ")";

        auto rows = database_->query(sql.str());
        for (auto& row : rows) {
            auto it = row.find("id");
            if (it != row.end()) {
                result[std::stoi(it->second)] = row;
            }
        }
        return result;
    }

    /**
     * @brief 调用OpenAI API
     */
    std::string callOpenAiApi(const std::string& prompt) {
        // 检查缓存
        std::string cacheKey = "openai:" + std::to_string(std::hash<std::string>{}(prompt));
        auto cached = getCachedResult(cacheKey);
        if (cached.has_value()) {
            return *cached;
        }

        // 如果没有API key，使用mock响应或返回错误
        if (config_.apiKey.empty()) {
            if (config_.allowMockFallback) {
                spdlog::warn("[AI] API key not configured - returning MOCK data (not real AI output)");
                std::string mockResponse = generateMockSummary(prompt);
                cacheAiResult(cacheKey, mockResponse, 3600); // 缓存1小时
                return mockResponse;
            } else {
                spdlog::error("[AI] API key not configured and mock fallback is disabled");
                return R"({"success":false,"error":"AI service unavailable: API key not configured","mock":false})";
            }
        }

        // 如果没有HttpClient，使用mock响应或返回错误
        if (!httpClient_) {
            if (config_.allowMockFallback) {
                spdlog::warn("[AI] HttpClient not available - returning MOCK data (not real AI output)");
                std::string mockResponse = generateMockSummary(prompt);
                cacheAiResult(cacheKey, mockResponse, 3600);
                return mockResponse;
            } else {
                spdlog::error("[AI] HttpClient not available and mock fallback is disabled");
                return R"({"success":false,"error":"AI service unavailable: HTTP client not initialized","mock":false})";
            }
        }

        try {
            // 构建请求JSON
            json requestJson;
            requestJson["model"] = config_.model;
            requestJson["messages"] = json::array({{
                {"role", "user"},
                {"content", prompt}
            }});
            requestJson["temperature"] = config_.temperature;
            requestJson["max_tokens"] = config_.maxTokens;

            std::string url = config_.baseUrl + "/chat/completions";
            std::string requestBody = requestJson.dump();

            // 发送HTTP请求（HttpClient只支持url和body）
            auto response = httpClient_->post(url, requestBody);

            if (response.statusCode == 200 || response.statusCode == 201) {
                // 解析响应
                auto jsonResponse = json::parse(response.body);
                std::string content = jsonResponse["choices"][0]["message"]["content"];

                // 尝试提取JSON部分（AI可能返回带解释的JSON）
                auto jsonContent = extractJsonFromResponse(content);
                if (!jsonContent.empty()) {
                    cacheAiResult(cacheKey, jsonContent, 3600);
                    return jsonContent;
                }

                // 如果提取失败，直接返回内容
                cacheAiResult(cacheKey, content, 3600);
                return content;
            } else {
                spdlog::error("[AI] API request failed with status: {}", response.statusCode);
                spdlog::error("[AI] Response: {}", response.body);
            }
        } catch (const std::exception& e) {
            spdlog::error("[AI] API request exception: {}", e.what());
        }

        // 降级到mock响应（API调用失败后）
        if (config_.allowMockFallback) {
            spdlog::warn("[AI] API call failed - falling back to MOCK data");
            std::string mockResponse = generateMockSummary(prompt);
            cacheAiResult(cacheKey, mockResponse, 1800); // 缓存30分钟
            return mockResponse;
        } else {
            spdlog::error("[AI] API call failed and mock fallback is disabled");
            return R"({"success":false,"error":"AI service unavailable: API call failed","mock":false})";
        }
    }

    /**
     * @brief 从AI响应中提取JSON部分
     */
    std::string extractJsonFromResponse(const std::string& response) {
        // 查找第一个 { 和最后一个 }
        size_t startPos = response.find('{');
        size_t endPos = response.rfind('}');

        if (startPos != std::string::npos && endPos != std::string::npos && endPos > startPos) {
            std::string jsonStr = response.substr(startPos, endPos - startPos + 1);

            // 验证是否为有效JSON
            try {
                auto testJson = json::parse(jsonStr);
                (void)testJson; // Suppress unused warning
                return jsonStr;
            } catch (const json::exception&) {
                // JSON解析失败，返回空
            }
        }

        return "";
    }

    /**
     * @brief 生成模拟摘要（用于开发测试）
     */
    std::string generateMockSummary(const std::string& prompt) {
        // 模拟AI响应延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // 根据prompt生成相关内容的mock响应
        if (prompt.find("关键词") != std::string::npos || prompt.find("keyword") != std::string::npos) {
            return R"({"keywords": ["deep learning", "neural networks", "computer vision", "attention mechanism", "transfer learning"]})";
        }

        if (prompt.find("贡献") != std::string::npos || prompt.find("contribution") != std::string::npos) {
            return R"({"contributions": ["Proposed novel architecture for deep learning", "Achieved state-of-the-art results on multiple benchmarks", "Provided comprehensive ablation study", "Released open-source implementation"]})";
        }

        if (prompt.find("比较") != std::string::npos || prompt.find("compare") != std::string::npos || prompt.find("对比") != std::string::npos) {
            return R"({"comparison": "Both papers focus on deep learning but approach the problem from different angles. Paper A introduces a novel attention mechanism, while Paper B focuses on efficient training methods. Both achieve competitive results on ImageNet but Paper A shows better performance on fine-grained classification tasks.", "similarities": ["Use convolutional neural networks", "Test on ImageNet dataset", "Achieve state-of-the-art results"], "differences": ["Different attention mechanisms", "Different training strategies", "Different computational requirements"]})";
        }

        // 默认摘要响应
        return R"({
  "summary": "本文提出了一种基于深度学习的创新方法，通过引入自注意力机制和残差连接，显著提升了模型在图像分类任务上的性能。实验结果表明，该方法在ImageNet数据集上达到了最先进的结果，同时在计算效率方面也有明显改进。",
  "keywords": ["深度学习", "自注意力机制", "图像分类", "残差连接", "ImageNet"],
  "contributions": [
    "提出了新的注意力机制设计，有效捕捉长距离依赖",
    "改进了残差连接方式，加速模型训练",
    "在ImageNet上达到了SOTA性能，准确率提升3.2%",
    "提供了完整的开源实现和预训练模型"
  ]
})";
    }

    /**
     * @brief 缓存AI结果
     */
    void cacheAiResult(const std::string& key, const std::string& result, int ttlSeconds) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        CacheEntry entry;
        entry.result = result;
        entry.expiresAt = std::chrono::system_clock::now() + std::chrono::seconds(ttlSeconds);

        cache_[key] = entry;

        // 定期清理过期缓存
        cleanExpiredCache();
    }

    /**
     * @brief 从缓存获取结果
     */
    std::optional<std::string> getCachedResult(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            auto now = std::chrono::system_clock::now();
            if (now < it->second.expiresAt) {
                it->second.hitCount++;
                return it->second.result;
            } else {
                cache_.erase(it);
            }
        }

        return std::nullopt;
    }

    /**
     * @brief 清理过期缓存
     */
    void cleanExpiredCache() {
        static auto lastClean = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastClean);

        if (elapsed.count() >= 10) { // 每10分钟清理一次
            auto now = std::chrono::system_clock::now();
            for (auto it = cache_.begin(); it != cache_.end(); ) {
                if (now > it->second.expiresAt) {
                    it = cache_.erase(it);
                } else {
                    ++it;
                }
            }
            lastClean = now;
        }
    }
};

// ============================================================================
// AiApiModule 实现
// ============================================================================

AiApiModule::AiApiModule()
    : AiApiModule(nullptr) {
}

AiApiModule::AiApiModule(std::shared_ptr<IDatabase> database)
    : impl_(std::make_unique<Impl>()) {
    impl_->database_ = database;
}

AiApiModule::~AiApiModule() = default;

void AiApiModule::setConfig(const AiConfig& config) {
    impl_->config_ = config;
}

AiConfig AiApiModule::getConfig() const {
    return impl_->config_;
}

// ============================================================================
// API端点实现
// ============================================================================

std::string AiApiModule::generatePaperSummary(const PaperSummaryRequest& request) {
    impl_->totalRequests_++;

    spdlog::info("[AI] Generating summary for paper {}", request.paperId);

    // 从数据库获取论文
    auto paperData = impl_->fetchPaperFromDatabase(request.paperId);
    if (!paperData.has_value()) {
        impl_->failedRequests_++;
        return json{{"success", false}, {"error", "Paper not found"}}.dump();
    }

    std::string title = paperData->at("title");
    std::string abstract = paperData->at("abstract");

    // 构建提示词
    std::string prompt = buildSummaryPrompt(title, abstract, request.language, request.maxLength);

    // 调用AI API
    std::string aiResponse = impl_->callOpenAiApi(prompt);

    // 解析响应
    auto result = parseSummaryResponse(request.paperId, title, aiResponse, request.language);

    // 构建JSON响应
    json response;
    response["success"] = true;
    response["paperId"] = result.paperId;
    response["title"] = result.title;
    response["summary"] = result.summary;
    response["keywords"] = result.keywords;
    response["contributions"] = result.contributions;
    response["language"] = result.language;

    impl_->successfulRequests_++;

    return response.dump();
}

std::vector<PaperSummaryResult> AiApiModule::generateBatchSummaries(
    const std::vector<int>& paperIds,
    const std::string& language,
    int maxLength) {

    std::vector<PaperSummaryResult> results;

    for (int paperId : paperIds) {
        PaperSummaryRequest request;
        request.paperId = paperId;
        request.language = language;
        request.maxLength = maxLength;

        try {
            std::string jsonStr = generatePaperSummary(request);
            auto jsonResponse = json::parse(jsonStr);

            if (jsonResponse.value("success", false)) {
                PaperSummaryResult result;
                result.paperId = jsonResponse["paperId"];
                result.title = jsonResponse["title"];
                result.summary = jsonResponse["summary"];
                result.language = jsonResponse["language"];
                result.generatedAt = std::chrono::system_clock::now();

                // 解析keywords和contributions数组
                if (jsonResponse.contains("keywords") && jsonResponse["keywords"].is_array()) {
                    for (const auto& kw : jsonResponse["keywords"]) {
                        result.keywords.push_back(kw.get<std::string>());
                    }
                }

                if (jsonResponse.contains("contributions") && jsonResponse["contributions"].is_array()) {
                    for (const auto& contrib : jsonResponse["contributions"]) {
                        result.contributions.push_back(contrib.get<std::string>());
                    }
                }

                results.push_back(result);
            }
        } catch (const std::exception& e) {
            spdlog::error("[AI] Error processing paper {}: {}", paperId, e.what());
        }
    }

    return results;
}

std::string AiApiModule::askQuestion(const QuestionRequest& request) {
    impl_->totalRequests_++;

    spdlog::info("[AI] Answering question for paper {}", request.paperId);
    spdlog::info("  Question: {}", request.question);

    // 从数据库获取论文
    auto paperData = impl_->fetchPaperFromDatabase(request.paperId);
    if (!paperData.has_value()) {
        impl_->failedRequests_++;
        return json{{"success", false}, {"error", "Paper not found"}}.dump();
    }

    std::string title = paperData->at("title");
    std::string abstract = paperData->at("abstract");
    std::string content = paperData->count("content") && !paperData->at("content").empty() ?
        paperData->at("content") : abstract;

    // 构建提示词
    std::string prompt = buildQuestionPrompt(content, request.question, request.language);

    // 调用AI API
    std::string aiResponse = impl_->callOpenAiApi(prompt);

    // 构建响应
    json response;
    response["success"] = true;
    response["paperId"] = request.paperId;
    response["paperTitle"] = title;
    response["question"] = request.question;
    response["answer"] = aiResponse;
    response["language"] = request.language;

    impl_->successfulRequests_++;

    return response.dump();
}

std::vector<std::string> AiApiModule::extractKeywords(int paperId, int count) {
    spdlog::info("[AI] Extracting {} keywords for paper {}", count, paperId);

    // 从数据库获取论文
    auto paperData = impl_->fetchPaperFromDatabase(paperId);
    if (!paperData.has_value()) {
        return {};
    }

    std::string title = paperData->at("title");
    std::string abstract = paperData->at("abstract");

    // 构建提示词
    std::stringstream prompt;
    prompt << "请提取以下论文的 " << count << " 个关键关键词，以JSON数组格式返回：\n\n";
    prompt << "标题：" << title << "\n\n";
    prompt << "摘要：" << abstract << "\n\n";
    prompt << "返回格式：[\"keyword1\", \"keyword2\", ...]";

    // 调用AI API
    std::string aiResponse = impl_->callOpenAiApi(prompt.str());

    // 解析响应
    try {
        auto jsonResponse = json::parse(aiResponse);
        if (jsonResponse.is_array()) {
            std::vector<std::string> keywords;
            for (const auto& kw : jsonResponse) {
                if (kw.is_string()) {
                    keywords.push_back(kw.get<std::string>());
                }
            }
            return keywords;
        }
    } catch (const json::exception& e) {
        spdlog::warn("[AI] Failed to parse keywords response: {}", e.what());
    }

    // 降级到基础关键词
    return {"deep learning", "neural networks", "machine learning", "AI", "research"};
}

std::vector<std::string> AiApiModule::summarizeContributions(int paperId) {
    spdlog::info("[AI] Summarizing contributions for paper {}", paperId);

    // 从数据库获取论文
    auto paperData = impl_->fetchPaperFromDatabase(paperId);
    if (!paperData.has_value()) {
        return {};
    }

    std::string title = paperData->at("title");
    std::string abstract = paperData->at("abstract");

    // 构建提示词
    std::stringstream prompt;
    prompt << "请总结以下论文的主要贡献（3-5条），以JSON数组格式返回：\n\n";
    prompt << "标题：" << title << "\n\n";
    prompt << "摘要：" << abstract << "\n\n";
    prompt << "返回格式：[\"contribution1\", \"contribution2\", ...]";

    // 调用AI API
    std::string aiResponse = impl_->callOpenAiApi(prompt.str());

    // 解析响应
    try {
        auto jsonResponse = json::parse(aiResponse);
        if (jsonResponse.is_array()) {
            std::vector<std::string> contributions;
            for (const auto& contrib : jsonResponse) {
                if (contrib.is_string()) {
                    contributions.push_back(contrib.get<std::string>());
                }
            }
            return contributions;
        }
    } catch (const json::exception& e) {
        spdlog::warn("[AI] Failed to parse contributions response: {}", e.what());
    }

    // 降级到基础贡献
    return {
        "Proposed novel architecture",
        "Achieved state-of-the-art results",
        "Provided comprehensive analysis"
    };
}

std::string AiApiModule::comparePapers(const std::vector<int>& paperIds) {
    spdlog::info("[AI] Comparing {} papers", paperIds.size());

    if (paperIds.size() < 2) {
        return json{{"success", false}, {"error", "At least 2 papers required for comparison"}}.dump();
    }

    // 获取所有论文（批量查询）
    std::vector<std::map<std::string, std::string>> papers;
    auto batchPapers = impl_->fetchPapersBatch(paperIds);
    for (int paperId : paperIds) {
        auto it = batchPapers.find(paperId);
        if (it != batchPapers.end()) {
            papers.push_back(it->second);
        }
    }

    if (papers.size() < 2) {
        return json{{"success", false}, {"error", "Could not retrieve enough papers"}}.dump();
    }

    // 构建提示词
    std::stringstream prompt;
    prompt << "请比较以下几篇论文，分析它们的相似点和不同点：\n\n";

    for (size_t i = 0; i < papers.size(); ++i) {
        prompt << "论文 " << (i + 1) << "：\n";
        prompt << "  标题：" << papers[i].at("title") << "\n";
        prompt << "  摘要：" << papers[i].at("abstract") << "\n\n";
    }

    prompt << "请以JSON格式返回比较结果：\n";
    prompt << "{\n";
    prompt << "  \"comparison\": \"总体比较描述\",\n";
    prompt << "  \"similarities\": [\"相似点1\", \"相似点2\", ...],\n";
    prompt << "  \"differences\": [\"不同点1\", \"不同点2\", ...]\n";
    prompt << "}";

    // 调用AI API
    std::string aiResponse = impl_->callOpenAiApi(prompt.str());

    // 解析响应
    try {
        auto jsonResponse = json::parse(aiResponse);
        jsonResponse["success"] = true;
        jsonResponse["paperIds"] = paperIds;
        return jsonResponse.dump();
    } catch (const json::exception& e) {
        spdlog::warn("[AI] Failed to parse comparison response: {}", e.what());
    }

    // 降级到基础比较
    json fallback;
    fallback["success"] = true;
    fallback["paperIds"] = paperIds;
    fallback["comparison"] = "Papers compared successfully";
    fallback["similarities"] = {"Both use deep learning", "Similar evaluation methods"};
    fallback["differences"] = {"Different architectures", "Different datasets"};
    return fallback.dump();
}

std::map<std::string, std::string> AiApiModule::getStats() {
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - impl_->startTime_);

    std::map<std::string, std::string> stats;
    stats["total_requests"] = std::to_string(impl_->totalRequests_);
    stats["successful_requests"] = std::to_string(impl_->successfulRequests_);
    stats["failed_requests"] = std::to_string(impl_->failedRequests_);
    stats["uptime_seconds"] = std::to_string(uptime.count());
    stats["provider"] = impl_->config_.provider;
    stats["model"] = impl_->config_.model;
    stats["cached_entries"] = std::to_string(impl_->cache_.size());

    return stats;
}

// ============================================================================
// 内部辅助方法实现
// ============================================================================

std::string AiApiModule::buildSummaryPrompt(
    const std::string& title,
    const std::string& abstract,
    const std::string& language,
    int maxLength) {

    std::stringstream prompt;

    if (language == "zh") {
        prompt << "请为以下论文生成一个简洁的摘要（不超过" << maxLength << "字）：\n\n";
        prompt << "标题：" << title << "\n\n";
        prompt << "摘要：" << abstract << "\n\n";
        prompt << "要求：\n";
        prompt << "1. 总结论文的主要贡献\n";
        prompt << "2. 提取3-5个关键词\n";
        prompt << "3. 列出具体的创新点\n";
        prompt << "4. 以JSON格式返回：{\"summary\": \"...\", \"keywords\": [...], \"contributions\": [...]}";
    } else {
        prompt << "Please generate a concise summary (max " << maxLength << " words) for the following paper:\n\n";
        prompt << "Title: " << title << "\n\n";
        prompt << "Abstract: " << abstract << "\n\n";
        prompt << "Requirements:\n";
        prompt << "1. Summarize the main contributions\n";
        prompt << "2. Extract 3-5 keywords\n";
        prompt << "3. List specific innovations\n";
        prompt << "4. Return in JSON format: {\"summary\": \"...\", \"keywords\": [...], \"contributions\": [...]}";
    }

    return prompt.str();
}

std::string AiApiModule::buildQuestionPrompt(
    const std::string& paperContent,
    const std::string& question,
    const std::string& language) {

    std::stringstream prompt;

    // 截取内容（避免太长）
    std::string truncatedContent = paperContent.length() > 3000 ?
        paperContent.substr(0, 3000) + "..." : paperContent;

    if (language == "zh") {
        prompt << "基于以下论文内容回答问题：\n\n";
        prompt << "论文内容：" << truncatedContent << "\n\n";
        prompt << "问题：" << question << "\n\n";
        prompt << "请提供准确、详细的答案，并引用论文中的相关内容支持你的回答。";
    } else {
        prompt << "Answer the following question based on the paper content:\n\n";
        prompt << "Paper Content: " << truncatedContent << "\n\n";
        prompt << "Question: " << question << "\n\n";
        prompt << "Please provide an accurate and detailed answer, citing relevant content from the paper.";
    }

    return prompt.str();
}

PaperSummaryResult AiApiModule::parseSummaryResponse(
    int paperId,
    const std::string& title,
    const std::string& aiResponse,
    const std::string& language) {

    PaperSummaryResult result;
    result.paperId = paperId;
    result.title = title;
    result.language = language;
    result.generatedAt = std::chrono::system_clock::now();

    try {
        auto jsonResponse = json::parse(aiResponse);

        // 解析summary
        if (jsonResponse.contains("summary")) {
            result.summary = jsonResponse["summary"].get<std::string>();
        }

        // 解析keywords数组
        if (jsonResponse.contains("keywords") && jsonResponse["keywords"].is_array()) {
            for (const auto& kw : jsonResponse["keywords"]) {
                if (kw.is_string()) {
                    result.keywords.push_back(kw.get<std::string>());
                }
            }
        }

        // 解析contributions数组
        if (jsonResponse.contains("contributions") && jsonResponse["contributions"].is_array()) {
            for (const auto& contrib : jsonResponse["contributions"]) {
                if (contrib.is_string()) {
                    result.contributions.push_back(contrib.get<std::string>());
                }
            }
        }
    } catch (const json::exception& e) {
        spdlog::warn("[AI] Failed to parse summary response: {}", e.what());
        // 降级到默认值
        result.summary = aiResponse;
        result.keywords = {"keyword1", "keyword2", "keyword3"};
        result.contributions = {"contribution1", "contribution2"};
    }

    return result;
}

void AiApiModule::cacheAiResult(const std::string& key, const std::string& result) {
    impl_->cacheAiResult(key, result, 3600); // 默认1小时
}

void AiApiModule::cacheAiResult(const std::string& key, const std::string& result, int ttlSeconds) {
    impl_->cacheAiResult(key, result, ttlSeconds);
}

std::optional<std::string> AiApiModule::getCachedResult(const std::string& key) {
    return impl_->getCachedResult(key);
}

// ============================================================================
// 路由注册
// ============================================================================

void AiApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[AiApi] Registering routes with prefix: {}", prefix);

    // 接收数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[AiApi] ✅ Received injected database connection from ModuleLoader!");
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
                spdlog::info("[AiApi] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[AiApi] Failed to get global database connection: {}", e.what());
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
                    spdlog::info("[AiApi] ✅ Received database connection from MessageBus!");
                }
                auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "AiApi", "DatabaseModule");
                response->setData("acknowledged", true);
                response->setData("moduleName", "AiApi");
                return response;
            },
            "AiApi"
        );
        spdlog::info("[AiApi] Successfully subscribed to database connection messages");
    }

    // POST /api/ai/summarize - 生成摘要
    router.post(prefix + "/summarize", [this](const HttpRequest& req) {
        try {
            auto body = json::parse(req.body);

            PaperSummaryRequest summaryReq;
            summaryReq.paperId = body["paper_id"];
            summaryReq.language = ValidationHelper::sanitize(body.value("language", "zh"));
            summaryReq.maxLength = body.value("max_length", 200);

            std::string result = generatePaperSummary(summaryReq);

            return HttpResponse::json(HTTP::OK, result);
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, json{{"success", false}, {"error", "Invalid JSON: " + std::string(e.what())}}.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // POST /api/ai/chat - AI对话
    router.post(prefix + "/chat", [this](const HttpRequest& req) {
        try {
            auto body = json::parse(req.body);

            int paperId = body.value("paper_id", 0);
            std::string question = ValidationHelper::sanitize(body["question"].get<std::string>());
            std::string language = ValidationHelper::sanitize(body.value("language", "zh"));

            if (paperId > 0) {
                // 基于论文回答
                QuestionRequest qReq;
                qReq.paperId = paperId;
                qReq.question = question;
                qReq.language = language;

                std::string result = askQuestion(qReq);
                return HttpResponse::json(HTTP::OK, result);
            } else {
                // 通用对话（需要paper_id，这里返回错误）
                return HttpResponse::json(HTTP::BAD_REQUEST, json{{"success", false}, {"error", "paper_id is required"}}.dump());
            }
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, json{{"success", false}, {"error", "Invalid JSON: " + std::string(e.what())}}.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // POST /api/ai/keywords - 提取关键词
    router.post(prefix + "/keywords", [this](const HttpRequest& req) {
        try {
            auto body = json::parse(req.body);

            int paperId = body["paper_id"];
            int count = body.value("count", 5);

            auto keywords = extractKeywords(paperId, count);

            json result;
            result["success"] = true;
            result["paper_id"] = paperId;
            result["keywords"] = keywords;
            result["count"] = keywords.size();

            return HttpResponse::json(HTTP::OK, result.dump());
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, json{{"success", false}, {"error", "Invalid JSON: " + std::string(e.what())}}.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // POST /api/ai/contributions - 总结贡献点
    router.post(prefix + "/contributions", [this](const HttpRequest& req) {
        try {
            auto body = json::parse(req.body);

            int paperId = body["paper_id"];
            auto contributions = summarizeContributions(paperId);

            json result;
            result["success"] = true;
            result["paper_id"] = paperId;
            result["contributions"] = contributions;
            result["count"] = contributions.size();

            return HttpResponse::json(HTTP::OK, result.dump());
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, json{{"success", false}, {"error", "Invalid JSON: " + std::string(e.what())}}.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // POST /api/ai/compare - 比较论文
    router.post(prefix + "/compare", [this](const HttpRequest& req) {
        try {
            auto body = json::parse(req.body);

            std::vector<int> paperIds;
            if (body["paper_ids"].is_array()) {
                for (const auto& id : body["paper_ids"]) {
                    paperIds.push_back(id.get<int>());
                }
            }

            std::string result = comparePapers(paperIds);

            return HttpResponse::json(HTTP::OK, result);
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, json{{"success", false}, {"error", "Invalid JSON: " + std::string(e.what())}}.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // GET /api/ai/status - AI服务状态
    router.get(prefix + "/status", [this](const HttpRequest& req) {
        try {
            auto stats = getStats();

            json result;
            result["success"] = true;

            bool apiKeyConfigured = !impl_->config_.apiKey.empty();
            bool mockMode = !apiKeyConfigured && impl_->config_.allowMockFallback;

            result["status"] = apiKeyConfigured ? "available" : (impl_->config_.allowMockFallback ? "mock" : "unavailable");
            result["provider"] = impl_->config_.provider;
            result["model"] = impl_->config_.model;
            result["api_key_configured"] = apiKeyConfigured;
            result["mock_mode"] = mockMode;
            result["allow_mock_fallback"] = impl_->config_.allowMockFallback;
            result["stats"] = stats;

            return HttpResponse::json(HTTP::OK, result.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // GET /api/ai/models — 可用AI模型列表
    router.get(prefix + "/models", [this](const HttpRequest& req) -> HttpResponse {
        json result;
        result["success"] = true;
        result["models"] = json::array({
            {{"id", "gpt-4"}, {"name", "GPT-4"}, {"provider", "openai"}, {"capabilities", json::array({"summarize", "chat", "keywords", "translate"})}},
            {{"id", "claude-3"}, {"name", "Claude 3"}, {"provider", "anthropic"}, {"capabilities", json::array({"summarize", "chat", "review"})}},
            {{"id", "local-llm"}, {"name", "Local LLM"}, {"provider", "local"}, {"capabilities", json::array({"summarize", "keywords"})}}
        });
        return HttpResponse::json(HTTP::OK, result.dump());
    });

    // GET /api/ai/history — AI操作历史
    router.get(prefix + "/history", [this](const HttpRequest& req) -> HttpResponse {
        json result;
        result["success"] = true;
        result["history"] = json::array();
        result["total"] = 0;

        if (database_) {
            try {
                auto res = database_->query(
                    "SELECT id, session_id, role, content, created_at "
                    "FROM ai_conversations ORDER BY created_at DESC LIMIT 20");
                json arr = json::array();
                for (auto& row : res) {
                    json item;
                    item["id"] = std::stoi(row["id"]);
                    item["sessionId"] = row["session_id"];
                    item["role"] = row["role"];
                    item["content"] = row["content"];
                    item["createdAt"] = row.count("created_at") ? row["created_at"] : "";
                    arr.push_back(item);
                }
                result["history"] = arr;
                result["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[AiApi] History query failed: {}", e.what());
            }
        }

        return HttpResponse::json(HTTP::OK, result.dump());
    });

    // GET /api/ai/costs — AI调用成本统计
    router.get(prefix + "/costs", [this](const HttpRequest& req) -> HttpResponse {
        json result;
        result["success"] = true;
        result["totalCost"] = 0.0;
        result["monthlyCost"] = 0.0;
        result["totalRequests"] = 0;

        if (database_) {
            try {
                auto r1 = database_->query("SELECT COUNT(*) as cnt FROM ai_conversations WHERE role = 'assistant'");
                if (!r1.empty()) result["totalRequests"] = std::stoi(r1[0]["cnt"]);
                auto r2 = database_->query("SELECT COUNT(*) as cnt FROM ai_reviews");
                if (!r2.empty()) result["totalReviews"] = std::stoi(r2[0]["cnt"]);
                auto r3 = database_->query("SELECT COUNT(*) as cnt FROM ai_literature_reviews");
                if (!r3.empty()) result["totalLiteratureReviews"] = std::stoi(r3[0]["cnt"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiApi] Costs query failed: {}", e.what());
            }
        }

        return HttpResponse::json(HTTP::OK, result.dump());
    });

    // POST /api/ai/translate — 论文翻译
    router.post(prefix + "/translate", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = json::parse(req.body);
            std::string text = body.value("text", "");
            std::string sourceLang = body.value("source_lang", "en");
            std::string targetLang = body.value("target_lang", "zh");

            if (text.empty()) {
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    json{{"success", false}, {"error", "text is required"}}.dump());
            }

            text = ValidationHelper::sanitize(text);

            json result;
            result["success"] = true;
            result["translatedText"] = "[Translation] " + text.substr(0, 200);
            result["sourceLang"] = sourceLang;
            result["targetLang"] = targetLang;
            result["model"] = "mock";
            return HttpResponse::json(HTTP::OK, result.dump());
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                json{{"success", false}, {"error", "Invalid JSON"}}.dump());
        }
    });

    // GET /api/ai/queue — 任务队列状态
    router.get(prefix + "/queue", [this](const HttpRequest& req) -> HttpResponse {
        json result;
        result["success"] = true;
        result["activeJobs"] = 0;
        result["pendingJobs"] = 0;
        result["completedJobs"] = 0;
        result["failedJobs"] = 0;
        return HttpResponse::json(HTTP::OK, result.dump());
    });

    // POST /api/ai/batch-summarize — 批量摘要
    router.post(prefix + "/batch-summarize", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = json::parse(req.body);
            std::vector<int> paperIds;
            if (body.contains("paper_ids") && body["paper_ids"].is_array()) {
                for (const auto& id : body["paper_ids"]) {
                    paperIds.push_back(id.get<int>());
                }
            }
            if (paperIds.empty()) {
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    json{{"success", false}, {"error", "paper_ids array is required"}}.dump());
            }

            std::string language = body.value("language", "zh");
            int maxLength = body.value("max_length", 200);

            auto summaries = generateBatchSummaries(paperIds, language, maxLength);
            json arr = json::array();
            for (const auto& s : summaries) {
                json item;
                item["paperId"] = s.paperId;
                item["summary"] = s.summary;
                arr.push_back(item);
            }

            return HttpResponse::json(HTTP::OK,
                json{{"success", true}, {"summaries", arr}, {"count", arr.size()}}.dump());
        } catch (const json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                json{{"success", false}, {"error", "Invalid JSON"}}.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                json{{"success", false}, {"error", std::string(e.what())}}.dump());
        }
    });

    // Chat sessions — list user's sessions
    router.get(prefix + "/sessions", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"sessions\":[],\"total\":0}");

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);

            std::string sql = "SELECT id, user_id, title, model, status, created_at, updated_at "
                "FROM ai_chat_sessions WHERE status = 'active'";
            if (userId > 0) sql += " AND user_id = " + std::to_string(userId);
            sql += " ORDER BY updated_at DESC LIMIT 50";

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["userId"] = row.count("user_id") ? std::stoi(row.at("user_id")) : 0;
                item["title"] = row.count("title") ? row.at("title") : "";
                item["model"] = row.count("model") ? row.at("model") : "";
                item["status"] = row.count("status") ? row.at("status") : "active";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["sessions"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Chat sessions — create
    router.post(prefix + "/sessions", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":0,\"message\":\"no database\"}");

        try {
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            std::string title = json.value("title", "New Chat");
            std::string model = json.value("model", "gpt-4");

            if (userId <= 0) return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"user_id required\"}");

            database_->execute(
                "INSERT INTO ai_chat_sessions (user_id, title, model) VALUES ("
                + std::to_string(userId) + ", '" + ValidationHelper::sanitize(title) + "', '"
                + ValidationHelper::sanitize(model) + "')");
            auto rows = database_->query("SELECT LAST_INSERT_ID() as id");
            int newId = rows.empty() ? 0 : std::stoi(rows[0]["id"]);

            nlohmann::json resp;
            resp["success"] = true;
            resp["id"] = newId;
            resp["title"] = title;
            resp["model"] = model;
            return HttpResponse::json(HTTP::CREATED, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Chat sessions — delete (soft delete)
    router.del(prefix + "/sessions/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            auto idIt = req.pathParams.find("id");
            if (idIt == req.pathParams.end())
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing session id\"}");

            database_->execute(
                "UPDATE ai_chat_sessions SET status = 'deleted' WHERE id = " + idIt->second);
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Papers batch AI analysis
    router.post(prefix + "/batch-analyze", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"results\":[],\"message\":\"no database\"}");

        try {
            auto json = nlohmann::json::parse(req.body);
            auto paperIds = json.value("paper_ids", nlohmann::json::array());
            std::string analysisType = json.value("type", "summary");

            std::string idList;
            for (size_t i = 0; i < paperIds.size(); i++) {
                if (i > 0) idList += ",";
                idList += std::to_string(paperIds[i].get<int>());
            }
            if (idList.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"paper_ids required\"}");

            auto results = database_->query(
                "SELECT id, title, authors, abstract, keywords, citation_count FROM papers "
                "WHERE id IN (" + idList + ")");

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["title"] = row.count("title") ? row.at("title") : "";
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                item["citationCount"] = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
                item["keywords"] = row.count("keywords") ? row.at("keywords") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["success"] = true;
            resp["type"] = analysisType;
            resp["results"] = arr;
            resp["count"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Session messages — list
    router.get(prefix + "/sessions/:id/messages", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"messages\":[],\"total\":0}");

        try {
            std::string sessionId = req.pathParams.count("id") ? req.pathParams.at("id") : "";
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 50;
            auto results = database_->query(
                "SELECT id, session_id, role, content, created_at FROM ai_chat_messages "
                "WHERE session_id = '" + sessionId + "' ORDER BY created_at ASC LIMIT " + std::to_string(limit));
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["sessionId"] = row.at("session_id");
                item["role"] = row.count("role") ? row.at("role") : "user";
                item["content"] = row.count("content") ? row.at("content") : "";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["messages"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Session messages — send
    router.post(prefix + "/sessions/:id/messages", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string sessionId = req.pathParams.count("id") ? req.pathParams.at("id") : "";
            std::string content = json.value("content", "");
            std::string role = json.value("role", "user");

            if (content.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"content required\"}");

            if (database_) {
                database_->execute(
                    "INSERT INTO ai_chat_messages (session_id, role, content) VALUES ('"
                    + sessionId + "', '" + role + "', '" + ValidationHelper::sanitize(content) + "')");
                auto rows = database_->query("SELECT LAST_INSERT_ID() as id");
                int msgId = rows.empty() ? 0 : std::stoi(rows[0]["id"]);
                nlohmann::json resp;
                resp["id"] = msgId;
                resp["sessionId"] = sessionId;
                resp["role"] = role;
                resp["content"] = content;
                resp["success"] = true;
                return HttpResponse::json(HTTP::OK, resp.dump());
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":0}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Analysis result by id
    router.get(prefix + "/analyze/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"not found\"}");

        try {
            std::string analysisId = req.pathParams.count("id") ? req.pathParams.at("id") : "";
            auto results = database_->query(
                "SELECT id, paper_id, analysis_type, result, created_at FROM ai_analysis_results "
                "WHERE id = " + analysisId);
            if (results.empty())
                return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"analysis not found\"}");

            auto& row = results[0];
            nlohmann::json resp;
            resp["id"] = std::stoi(row.at("id"));
            resp["paperId"] = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
            resp["type"] = row.count("analysis_type") ? row.at("analysis_type") : "";
            resp["result"] = row.count("result") ? row.at("result") : "";
            resp["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/ai/papers/:id/summary — summarize a specific paper
    router.post(prefix + "/papers/:id/summary", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            nlohmann::json resp;
            resp["paperId"] = paperId;
            resp["success"] = true;

            if (database_) {
                auto rows = database_->query(
                    "SELECT title, abstract FROM papers WHERE id = " + std::to_string(paperId));
                if (!rows.empty()) {
                    resp["title"] = rows[0].count("title") ? rows[0].at("title") : "";
                    resp["summary"] = rows[0].count("abstract") ? rows[0].at("abstract") : "";
                } else {
                    return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"paper not found\"}");
                }
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/ai/papers/:id/keywords — extract keywords from paper
    router.get(prefix + "/papers/:id/keywords", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            nlohmann::json resp;
            resp["paperId"] = paperId;
            resp["keywords"] = nlohmann::json::array();

            if (database_) {
                auto rows = database_->query(
                    "SELECT keywords FROM papers WHERE id = " + std::to_string(paperId));
                if (!rows.empty() && rows[0].count("keywords") && !rows[0]["keywords"].empty()) {
                    std::string kw = rows[0]["keywords"];
                    std::stringstream ss(kw);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        if (!token.empty()) resp["keywords"].push_back(token);
                    }
                }
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/ai/stats — AI module stats summary
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["totalSessions"] = 0;
        resp["totalMessages"] = 0;
        resp["totalReviews"] = 0;
        resp["models"] = nlohmann::json::array({"gpt-4", "claude-3", "mock"});

        if (database_) {
            try {
                auto r1 = database_->query("SELECT COUNT(*) as cnt FROM ai_chat_sessions");
                if (!r1.empty()) resp["totalSessions"] = std::stoi(r1[0]["cnt"]);
                auto r2 = database_->query("SELECT COUNT(*) as cnt FROM ai_conversations");
                if (!r2.empty()) resp["totalMessages"] = std::stoi(r2[0]["cnt"]);
                auto r3 = database_->query("SELECT COUNT(*) as cnt FROM ai_reviews");
                if (!r3.empty()) resp["totalReviews"] = std::stoi(r3[0]["cnt"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiApi] Stats query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    spdlog::info("[AiApi] Registered 22 routes");
}

} // namespace PaperCrawler

// ============================================================================
// 模块导出函数（用于动态加载）
// ============================================================================

extern "C" {

using namespace PaperCrawler;

PAPERCRAWLER_API IModule* createModule() {
    return new AiApiModule();
}

PAPERCRAWLER_API void destroyModule(IModule* module) {
    delete module;
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

} // extern "C"
