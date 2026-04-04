#include "business/AIClients.hpp"
#include "common/JsonUtils.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <regex>

namespace PaperCrawler {

// ============================================================================
// OpenAI客户端实现
// ============================================================================

OpenAIClient::OpenAIClient(const std::string& apiKey, const std::string& baseUrl)
    : apiKey_(apiKey), baseUrl_(baseUrl) {
    httpClient_ = Services::resolve<Network::HttpClient>();
}

std::string OpenAIClient::chatCompletion(
    const std::string& model,
    const std::vector<std::map<std::string, std::string>>& messages,
    double temperature,
    int maxTokens) {

    if (!httpClient_) {
        throw std::runtime_error("HttpClient not available");
    }

    // 构建请求JSON
    json requestJson;
    requestJson["model"] = model;
    requestJson["messages"] = json::array();

    for (const auto& msg : messages) {
        json messageJson;
        messageJson["role"] = msg.at("role");
        messageJson["content"] = msg.at("content");
        requestJson["messages"].push_back(messageJson);
    }

    requestJson["temperature"] = temperature;
    requestJson["max_tokens"] = maxTokens;

    std::string requestBody = requestJson.dump();

    // 构建请求头
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + apiKey_;

    // 发送HTTP POST请求
    auto response = httpClient_->postWithHeaders(
        baseUrl_ + "/chat/completions",
        requestBody,
        headers
    );

    if (response.statusCode != 200) {
        throw std::runtime_error("OpenAI API error: " + response.body);
    }

    // 解析响应
    return parseResponse(response.body);
}

std::string OpenAIClient::buildMessagesJson(
    const std::vector<std::map<std::string, std::string>>& messages) {

    json messagesJson = json::array();
    for (const auto& msg : messages) {
        json messageJson;
        messageJson["role"] = msg.at("role");
        messageJson["content"] = msg.at("content");
        messagesJson.push_back(messageJson);
    }
    return messagesJson.dump();
}

std::string OpenAIClient::parseResponse(const std::string& responseBody) {
    try {
        json responseJson = json::parse(responseBody);

        if (responseJson.contains("choices") && !responseJson["choices"].empty()) {
            auto choice = responseJson["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                return choice["message"]["content"];
            }
        }

        return "";
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse OpenAI response: " + std::string(e.what()));
    }
}

void OpenAIClient::streamChatCompletion(
    const std::string& model,
    const std::vector<std::map<std::string, std::string>>& messages,
    std::function<void(const std::string&)> callback,
    double temperature,
    int maxTokens) {

    if (!httpClient_) {
        throw std::runtime_error("HttpClient not available");
    }

    // 构建请求JSON
    json requestJson;
    requestJson["model"] = model;
    requestJson["messages"] = json::array();

    for (const auto& msg : messages) {
        json messageJson;
        messageJson["role"] = msg.at("role");
        messageJson["content"] = msg.at("content");
        requestJson["messages"].push_back(messageJson);
    }

    requestJson["temperature"] = temperature;
    requestJson["max_tokens"] = maxTokens;
    requestJson["stream"] = true;  // 启用流式响应

    std::string requestBody = requestJson.dump();

    // 构建请求头
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + apiKey_;

    // 发送流式请求（TODO: 需要HttpClient支持SSE）
    // 简化实现：使用非流式API
    std::string response = chatCompletion(model, messages, temperature, maxTokens);
    callback(response);
}

std::vector<std::string> OpenAIClient::batchChatCompletion(
    const std::vector<std::vector<std::map<std::string, std::string>>>& batches,
    const std::string& model) {

    // OpenAI Batch API需要先将请求上传到文件，然后提交批处理
    // 简化实现：循环调用
    std::vector<std::string> results;
    results.reserve(batches.size());

    for (const auto& batch : batches) {
        std::string result = chatCompletion(model, batch, 0.7, 2000);
        results.push_back(result);
    }

    return results;
}

// ============================================================================
// Claude客户端实现
// ============================================================================

ClaudeClient::ClaudeClient(const std::string& apiKey)
    : apiKey_(apiKey) {
    httpClient_ = Services::resolve<Network::HttpClient>();
}

std::string ClaudeClient::sendMessage(
    const std::string& model,
    const std::string& message,
    double temperature,
    int maxTokens) {

    if (!httpClient_) {
        throw std::runtime_error("HttpClient not available");
    }

    // 构建请求JSON
    json requestJson;
    requestJson["model"] = model;
    requestJson["max_tokens"] = maxTokens;
    requestJson["messages"] = json::array({
        {{"role", "user"}, {"content", message}}
    });
    requestJson["temperature"] = temperature;
    requestJson["anthropic_version"] = "2023-06-01";

    std::string requestBody = requestJson.dump();

    // 构建请求头
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["x-api-key"] = apiKey_;
    headers["anthropic-version"] = "2023-06-01";

    // 发送HTTP POST请求
    auto response = httpClient_->postWithHeaders(
        "https://api.anthropic.com/v1/messages",
        requestBody,
        headers
    );

    if (response.statusCode != 200) {
        throw std::runtime_error("Claude API error: " + response.body);
    }

    // 解析响应
    try {
        json responseJson = json::parse(response.body);

        if (responseJson.contains("content") && !responseJson["content"].empty()) {
            return responseJson["content"][0]["text"];
        }

        return "";
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse Claude response: " + std::string(e.what()));
    }
}

// ============================================================================
// 本地LLM客户端实现
// ============================================================================

LocalLLMClient::LocalLLMClient(const std::string& modelPath)
    : modelPath_(modelPath), available_(false) {
    httpClient_ = Services::resolve<Network::HttpClient>();

    // 检查本地模型是否可用
    // TODO: 实现真实的本地模型检测
    available_ = true; // 占位符
}

std::string LocalLLMClient::generate(
    const std::string& prompt,
    int maxTokens,
    double temperature) {

    if (!isAvailable()) {
        throw std::runtime_error("Local LLM not available");
    }

    // 调用本地模型（例如llama.cpp HTTP服务器）
    if (!httpClient_) {
        throw std::runtime_error("HttpClient not available");
    }

    // 构建请求
    json requestJson;
    requestJson["prompt"] = prompt;
    requestJson["n_predict"] = maxTokens;
    requestJson["temperature"] = temperature;
    requestJson["stream"] = false;

    std::string requestBody = requestJson.dump();

    // 发送到本地模型服务器（假设运行在localhost:8080）
    auto response = httpClient_->post(
        "http://localhost:8080/completion",
        requestBody
    );

    if (response.statusCode != 200) {
        throw std::runtime_error("Local LLM error: " + response.body);
    }

    // 解析响应
    try {
        json responseJson = json::parse(response.body);
        if (responseJson.contains("content")) {
            return responseJson["content"];
        }
        return "";
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse local LLM response: " + std::string(e.what()));
    }
}

bool LocalLLMClient::isAvailable() const {
    return available_;
}

} // namespace PaperCrawler
