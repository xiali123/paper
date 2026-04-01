#include "business/AiApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "network/HttpClient.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iomanip>

namespace PaperCrawler {

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

        std::cout << "[AI] Initializing AI module..." << std::endl;
        std::cout << "  Provider: " << config.provider << std::endl;
        std::cout << "  Model: " << config.model << std::endl;
        std::cout << "  Base URL: " << config.baseUrl << std::endl;

        // 验证配置
        if (config.apiKey.empty()) {
            std::cout << "[AI] WARNING: API key not configured" << std::endl;
        }

        std::cout << "[AI] Initialization complete" << std::endl;
        return true;
    }

    /**
     * @brief 从数据库获取论文内容
     */
    std::optional<std::map<std::string, std::string>> fetchPaperFromDatabase(int paperId) {
        if (!database_) {
            return std::nullopt;
        }

        std::string sql = "SELECT id, title, abstract, content, authors, publication_year "
                         "FROM papers WHERE id = " + std::to_string(paperId);

        auto results = database_->query(sql);
        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }

    /**
     * @brief 调用OpenAI API
     */
    std::string callOpenAiApi(const std::string& prompt) {
        if (config_.apiKey.empty()) {
            return "ERROR: API key not configured";
        }

        // 构建请求JSON
        std::stringstream requestJson;
        requestJson << "{";
        requestJson << "\"model\": \"" << config_.model << "\",";
        requestJson << "\"messages\": [";
        requestJson << "{\"role\": \"user\", \"content\": \"" << escapeJson(prompt) << "\"}";
        requestJson << "],";
        requestJson << "\"temperature\": " << config_.temperature << ",";
        requestJson << "\"max_tokens\": " << config_.maxTokens;
        requestJson << "}";

        // 发送HTTP请求
        std::string url = config_.baseUrl + "/chat/completions";

        // TODO: 使用HttpClient发送实际请求
        // 目前返回模拟响应
        return generateMockSummary(prompt);
    }

    /**
     * @brief 生成模拟摘要（用于开发测试）
     */
    std::string generateMockSummary(const std::string& prompt) {
        // 模拟AI响应延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        return R"({
  "summary": "本文提出了一种新颖的深度学习方法，通过引入自注意力机制和残差连接，显著提升了模型在图像分类任务上的性能。实验结果表明，该方法在ImageNet数据集上达到了最先进的结果。",
  "keywords": ["深度学习", "自注意力机制", "图像分类", "残差连接", "ImageNet"],
  "contributions": [
    "提出了新的注意力机制设计",
    "改进了残差连接方式",
    "在ImageNet上达到了SOTA性能",
    "提供了完整的开源实现"
  ]
})";
    }

    /**
     * @brief 转义JSON字符串
     */
    std::string escapeJson(const std::string& str) {
        std::string result;
        result.reserve(str.size() * 1.2);

        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 32) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }

        return result;
    }

    /**
     * @brief 解析JSON响应（简化版）
     */
    std::string parseJsonField(const std::string& json, const std::string& field) {
        // 简化版JSON解析 - 生产环境应使用专业JSON库
        size_t pos = json.find("\"" + field + "\"");
        if (pos == std::string::npos) {
            return "";
        }

        pos = json.find(":\"", pos);
        if (pos == std::string::npos) {
            pos = json.find(": \"", pos);
        }
        if (pos == std::string::npos) {
            return "";
        }

        pos += 2; // 跳过 :"
        size_t endPos = json.find("\"", pos);
        if (endPos == std::string::npos) {
            return "";
        }

        return json.substr(pos, endPos - pos);
    }
};

// ============================================================================
// AiApiModule 实现
// ============================================================================

AiApiModule::AiApiModule()
    : impl_(std::make_unique<Impl>()) {
}

AiApiModule::~AiApiModule() = default;

bool AiApiModule::initialize() {
    std::cout << "AiApiModule::initialize" << std::endl;

    // TODO: 从ServiceContainer获取依赖
    // auto database = ServiceContainer::instance().getService<IDatabase>();
    // auto httpClient = ServiceContainer::instance().getService<HttpClient>();

    AiConfig defaultConfig;
    return impl_->initialize(defaultConfig, nullptr, HttpClientPtr{});
}

bool AiApiModule::start() {
    std::cout << "AiApiModule started" << std::endl;
    return true;
}

bool AiApiModule::stop() {
    std::cout << "AiApiModule stopped" << std::endl;
    return true;
}

void AiApiModule::cleanup() {
    // 清理资源
}

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

    std::cout << "[AI] Generating summary for paper " << request.paperId << std::endl;

    // 从数据库获取论文
    auto paperData = impl_->fetchPaperFromDatabase(request.paperId);
    if (!paperData.has_value()) {
        impl_->failedRequests_++;
        return R"({"success": false, "error": "Paper not found"})";
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
    std::stringstream response;
    response << "{";
    response << "\"success\": true,";
    response << "\"paperId\": " << result.paperId << ",";
    response << "\"title\": \"" << impl_->escapeJson(result.title) << "\",";
    response << "\"summary\": \"" << impl_->escapeJson(result.summary) << "\",";
    response << "\"keywords\": [";

    for (size_t i = 0; i < result.keywords.size(); ++i) {
        if (i > 0) response << ",";
        response << "\"" << impl_->escapeJson(result.keywords[i]) << "\"";
    }

    response << "],";
    response << "\"contributions\": [";

    for (size_t i = 0; i < result.contributions.size(); ++i) {
        if (i > 0) response << ",";
        response << "\"" << impl_->escapeJson(result.contributions[i]) << "\"";
    }

    response << "],";
    response << "\"language\": \"" << result.language << "\"";
    response << "}";

    impl_->successfulRequests_++;

    return response.str();
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

        std::string json = generatePaperSummary(request);
        // TODO: 解析JSON并添加到results
    }

    return results;
}

std::string AiApiModule::askQuestion(const QuestionRequest& request) {
    impl_->totalRequests_++;

    std::cout << "[AI] Answering question for paper " << request.paperId << std::endl;
    std::cout << "  Question: " << request.question << std::endl;

    // 从数据库获取论文
    auto paperData = impl_->fetchPaperFromDatabase(request.paperId);
    if (!paperData.has_value()) {
        impl_->failedRequests_++;
        return R"({"success": false, "error": "Paper not found"})";
    }

    std::string content = paperData->at("abstract");
    // 实际应该包含完整内容，这里简化为使用摘要

    // 构建提示词
    std::string prompt = buildQuestionPrompt(content, request.question, request.language);

    // 调用AI API
    std::string aiResponse = impl_->callOpenAiApi(prompt);

    // 构建响应
    std::stringstream response;
    response << "{";
    response << "\"success\": true,";
    response << "\"paperId\": " << request.paperId << ",";
    response << "\"question\": \"" << impl_->escapeJson(request.question) << "\",";
    response << "\"answer\": \"" << impl_->escapeJson(aiResponse) << "\",";
    response << "\"language\": \"" << request.language << "\"";
    response << "}";

    impl_->successfulRequests_++;

    return response.str();
}

std::vector<std::string> AiApiModule::extractKeywords(int paperId, int count) {
    std::cout << "[AI] Extracting " << count << " keywords for paper " << paperId << std::endl;

    // TODO: 实现关键词提取
    return {"deep learning", "neural networks", "computer vision"};
}

std::vector<std::string> AiApiModule::summarizeContributions(int paperId) {
    std::cout << "[AI] Summarizing contributions for paper " << paperId << std::endl;

    // TODO: 实现贡献点总结
    return {
        "Proposed novel architecture",
        "Achieved state-of-the-art results",
        "Provided comprehensive ablation study"
    };
}

std::string AiApiModule::comparePapers(const std::vector<int>& paperIds) {
    std::cout << "[AI] Comparing " << paperIds.size() << " papers" << std::endl;

    // TODO: 实现论文比较
    return R"({"success": true, "comparison": "Papers compared successfully"})";
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

    if (language == "zh") {
        prompt << "基于以下论文内容回答问题：\n\n";
        prompt << "论文内容：" << paperContent << "\n\n";
        prompt << "问题：" << question << "\n\n";
        prompt << "请提供准确、详细的答案，并引用论文中的相关内容支持你的回答。";
    } else {
        prompt << "Answer the following question based on the paper content:\n\n";
        prompt << "Paper Content: " << paperContent << "\n\n";
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

    // 简化解析 - 生产环境应使用专业JSON库
    result.summary = impl_->parseJsonField(aiResponse, "summary");

    // TODO: 解析keywords和contributions数组
    result.keywords = {"keyword1", "keyword2", "keyword3"};
    result.contributions = {"contribution1", "contribution2"};

    return result;
}

void AiApiModule::cacheAiResult(const std::string& key, const std::string& result) {
    // TODO: 实现缓存
}

std::optional<std::string> AiApiModule::getCachedResult(const std::string& key) {
    // TODO: 实现缓存查询
    return std::nullopt;
}

} // namespace PaperCrawler

// ============================================================================
// 模块导出函数（用于动态加载）
// ============================================================================

extern "C" {

using namespace PaperCrawler;

/**
 * @brief 创建模块实例
 */
PAPERCRAWLER_API IModule* createModule() {
    return new AiApiModule();
}

/**
 * @brief 销毁模块实例
 */
PAPERCRAWLER_API void destroyModule(IModule* module) {
    delete module;
}

/**
 * @brief 获取模块版本
 */
PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

} // extern "C"
