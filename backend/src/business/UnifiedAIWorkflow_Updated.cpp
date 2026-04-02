#include "business/UnifiedAIWorkflow.hpp"
#include "business/AIClients.hpp"
#include "common/JsonUtils.hpp"
#include "core/EventDrivenIntegration.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <regex>
#include <iomanip>

namespace PaperCrawler {

class UnifiedAIWorkflow::Impl {
public:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<CacheModule> cache_;

    // AI客户端
    std::unique_ptr<OpenAIClient> openaiClient_;
    std::unique_ptr<ClaudeClient> claudeClient_;
    std::unique_ptr<LocalLLMClient> localClient_;

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
    std::string claudeApiKey_;
};

// 在UnifiedAIWorkflow::initialize()中初始化AI客户端
bool UnifiedAIWorkflow::initialize() {
    // 解析服务
    impl_->database_ = Services::resolve<IDatabase>();
    impl_->cache_ = Services::resolve<CacheModule>();

    if (!impl_->database_) {
        return false;
    }

    // 从环境变量或配置读取API密钥
    const char* openaiKey = std::getenv("OPENAI_API_KEY");
    if (openaiKey) {
        impl_->openaiApiKey_ = openaiKey;
        impl_->openaiClient_ = std::make_unique<OpenAIClient>(
            impl_->openaiApiKey_,
            "https://api.openai.com/v1"
        );
    }

    const char* claudeKey = std::getenv("CLAUDE_API_KEY");
    if (claudeKey) {
        impl_->claudeApiKey_ = claudeKey;
        impl_->claudeClient_ = std::make_unique<ClaudeClient>(
            impl_->claudeApiKey_
        );
    }

    // 初始化本地模型客户端
    impl_->localClient_ = std::make_unique<LocalLLMClient>(
        "./models/llama-model.gguf"  // 本地模型路径
    );

    // 预计算常见查询
    precomputeCommonQueries();

    // 订阅事件
    auto& eventBus = EventDrivenIntegration::getInstance();
    eventBus.subscribe(EventType::AI_REQUEST_SENT, "UnifiedAIWorkflow",
        [this](const Event& event) {
            impl_->totalRequests_++;
        }
    );

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("UnifiedAIWorkflow initialized with real AI clients");
    }

    return true;
}

AIResult UnifiedAIWorkflow::callCloudAPI(const std::string& prompt, AIModelType model) {
    AIResult result;
    result.timestamp = std::chrono::system_clock::now();
    result.fromCache = false;

    try {
        // 构建消息格式
        std::vector<std::map<std::string, std::string>> messages;
        messages.push_back({{"role", "user"}, {"content", prompt}});

        std::string response;

        if (model == AIModelType::GPT_4 || model == AIModelType::GPT_4_MINI) {
            std::string modelName = (model == AIModelType::GPT_4) ? "gpt-4" : "gpt-4.1-mini";

            if (impl_->openaiClient_) {
                response = impl_->openaiClient_->chatCompletion(
                    modelName,
                    messages,
                    0.7,  // temperature
                    2000  // maxTokens
                );

                result.model = modelName;
                result.success = true;
            } else {
                result.success = false;
                result.errorMessage = "OpenAI client not initialized";
                return result;
            }

        } else if (model == AIModelType::CLAUDE_3_5_SONNET) {
            if (impl_->claudeClient_) {
                response = impl_->claudeClient_->sendMessage(
                    "claude-3-5-sonnet-20241022",
                    prompt,
                    0.7,
                    2000
                );

                result.model = "claude-3-5-sonnet-20241022";
                result.success = true;
            } else {
                result.success = false;
                result.errorMessage = "Claude client not initialized";
                return result;
            }
        } else {
            result.success = false;
            result.errorMessage = "Unknown model type";
            return result;
        }

        result.content = response;

        // 估算token和成本
        int inputTokens = prompt.length() / 4;
        int outputTokens = response.length() / 4;
        result.tokensUsed = inputTokens + outputTokens;

        // 定价（2026年价格）
        if (model == AIModelType::GPT_4) {
            result.costUsd = (inputTokens * 0.03 + outputTokens * 0.06) / 1000;
        } else if (model == AIModelType::GPT_4_MINI) {
            result.costUsd = (inputTokens * 0.0015 + outputTokens * 0.002) / 1000;
        } else if (model == AIModelType::CLAUDE_3_5_SONNET) {
            result.costUsd = (inputTokens * 0.003 + outputTokens * 0.015) / 1000;
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }

    return result;
}

AIResult UnifiedAIWorkflow::callLocalModel(const std::string& prompt) {
    AIResult result;
    result.timestamp = std::chrono::system_clock::now();
    result.fromCache = false;

    if (!impl_->localClient_ || !impl_->localClient_->isAvailable()) {
        result.success = false;
        result.errorMessage = "Local model not available";
        return result;
    }

    try {
        std::string response = impl_->localClient_->generate(
            prompt,
            500,   // maxTokens
            0.7    // temperature
        );

        result.success = true;
        result.content = response;
        result.model = "Local-LLM";
        result.costUsd = 0.0;  // 本地模型免费
        result.tokensUsed = prompt.length() / 4;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }

    return result;
}

} // namespace PaperCrawler
