#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <optional>

namespace PaperCrawler {

// 向量嵌入
struct Embedding {
    std::string id;           // 论文/文档ID
    std::string content;      // 原始文本（标题+摘要）
    std::vector<float> vector; // 嵌入向量
    std::map<std::string, std::string> metadata; // 元数据
};

// 相似度搜索结果
struct SearchResult {
    std::string id;
    float score;              // 余弦相似度
    std::string content;
    std::map<std::string, std::string> metadata;
};

// 向量存储 -- SQLite-backed
class VectorStore {
public:
    VectorStore();
    ~VectorStore() = default;

    // 存储嵌入
    bool storeEmbedding(const Embedding& embedding);
    bool storeEmbeddings(const std::vector<Embedding>& embeddings);

    // 相似度搜索（余弦相似度）
    std::vector<SearchResult> search(const std::vector<float>& queryVector,
                                      int topK = 5,
                                      float minScore = 0.5f);

    // 获取嵌入
    std::optional<Embedding> getEmbedding(const std::string& id);

    // 删除嵌入
    bool deleteEmbedding(const std::string& id);

    // 批量索引（从数据库论文表生成嵌入）
    int indexPapersFromDatabase(std::shared_ptr<class IDatabase> database,
                                 int limit = 1000);

    // 工具方法
    static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    static std::string vectorToJson(const std::vector<float>& vec);
    static std::vector<float> jsonToVector(const std::string& json);

    // 设置数据库
    void setDatabase(std::shared_ptr<class IDatabase> db);

    // 初始化表
    bool initializeTable();

private:
    std::shared_ptr<class IDatabase> database_;
    std::map<std::string, Embedding> cache_; // L1缓存

    // 内存搜索（当数据库不可用时）
    std::vector<SearchResult> searchInMemory(const std::vector<float>& queryVector,
                                               int topK, float minScore);

    // 从数据库加载所有嵌入到缓存
    void loadAllToCache();
};

} // namespace PaperCrawler
