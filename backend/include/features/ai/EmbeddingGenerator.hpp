#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace PaperCrawler {

// 嵌入生成器（多Provider支持）
class EmbeddingGenerator {
public:
    EmbeddingGenerator();
    ~EmbeddingGenerator() = default;

    // 生成嵌入向量
    std::vector<float> generate(const std::string& text);
    std::vector<std::vector<float>> generateBatch(const std::vector<std::string>& texts);

    // 配置
    void setProvider(const std::string& provider); // "local" | "openai"
    void setApiKey(const std::string& key);
    void setModel(const std::string& model);
    void setDimension(int dim);

    // 本地嵌入生成（TF-hash, 无需外部API）
    static std::vector<float> generateLocalEmbedding(const std::string& text, int dimensions = 384);

private:
    std::string provider_{"local"};
    std::string apiKey_;
    std::string model_{"text-embedding-3-small"};
    int dimensions_{384};

    // OpenAI embedding API
    std::vector<float> generateOpenAI(const std::string& text);

    // 简单hash嵌入
    static uint32_t fnv1aHash(const std::string& str);
    static float hashToFloat(uint32_t hash);
};

} // namespace PaperCrawler
