#pragma once

#include "network/HttpClient.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include <string>
#include <map>
#include <memory>
#include <optional>

namespace PaperCrawler {

/**
 * @brief OpenAI API客户端
 */
class OpenAIClient {
public:
    OpenAIClient(const std::string& apiKey, const std::string& baseUrl = "https://api.openai.com/v1");
    ~OpenAIClient() = default;

    /**
     * @brief 聊天完成API
     */
    std::string chatCompletion(
        const std::string& model,
        const std::vector<std::map<std::string, std::string>>& messages,
        double temperature = 0.7,
        int maxTokens = 2000
    );

    /**
     * @brief 流式聊天完成（SSE）
     */
    void streamChatCompletion(
        const std::string& model,
        const std::vector<std::map<std::string, std::string>>& messages,
        std::function<void(const std::string&)> callback,
        double temperature = 0.7,
        int maxTokens = 2000
    );

    /**
     * @brief 批量API（50%折扣）
     */
    std::vector<std::string> batchChatCompletion(
        const std::vector<std::vector<std::map<std::string, std::string>>>& batches,
        const std::string& model
    );

private:
    std::string apiKey_;
    std::string baseUrl_;
    std::shared_ptr<Network::HttpClient> httpClient_;

    std::string buildMessagesJson(const std::vector<std::map<std::string, std::string>>& messages);
    std::string parseResponse(const std::string& responseBody);
};

/**
 * @brief Claude API客户端
 */
class ClaudeClient {
public:
    ClaudeClient(const std::string& apiKey);
    ~ClaudeClient() = default;

    std::string sendMessage(
        const std::string& model,
        const std::string& message,
        double temperature = 0.7,
        int maxTokens = 2000
    );

private:
    std::string apiKey_;
    std::shared_ptr<Network::HttpClient> httpClient_;
};

/**
 * @brief 本地LLM客户端（llama.cpp）
 */
class LocalLLMClient {
public:
    LocalLLMClient(const std::string& modelPath);
    ~LocalLLMClient() = default;

    std::string generate(
        const std::string& prompt,
        int maxTokens = 500,
        double temperature = 0.7
    );

    bool isAvailable() const;

private:
    std::string modelPath_;
    std::shared_ptr<Network::HttpClient> httpClient_;
    bool available_;
};

} // namespace PaperCrawler
