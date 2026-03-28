#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace PaperCrawler {

/**
 * @brief 论文信息
 */
struct Paper {
    int id;
    std::string title;
    std::string authors;
    int year;
    std::string abstract;
    std::string journal;
    std::string volume;
    std::string issue;
    std::string pages;
    std::string doi;
    std::string url;
    std::string pdfPath;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;

    // 元数据
    std::vector<std::string> tags;
    std::vector<std::string> keywords;
    int citationCount{0};
    bool isRead{false};
    bool isFavorite{false};
    std::string notes;

    // 序列化为JSON
    std::string toJSON() const;
};

/**
 * @brief 论文搜索条件
 */
struct PaperSearchCriteria {
    std::string query;           // 关键词
    std::string author;          // 作者
    int yearFrom{0};             // 起始年份
    int yearTo{0};               // 结束年份
    std::string journal;         // 期刊
    std::vector<std::string> tags;  // 标签
    bool isRead{false};          // 已读
    bool isFavorite{false};      // 收藏
};

/**
 * @brief 论文统计信息
 */
struct PaperStats {
    uint64_t totalPapers{0};
    uint64_t readPapers{0};
    uint64_t unreadPapers{0};
    uint64_t favoritePapers{0};
    std::map<int, uint64_t> papersByYear;  // 按年份统计
    std::map<std::string, uint64_t> papersByJournal;  // 按期刊统计
    std::map<std::string, uint64_t> papersByAuthor;   // 按作者统计
    std::map<std::string, uint64_t> papersByTag;      // 按标签统计
};

/**
 * @brief 论文API模块
 *
 * 功能：
 * 1. 论文CRUD操作
 * 2. 论文搜索
 * 3. 论文统计
 * 4. 批量导入/导出
 * 5. PDF文件管理
 * 6. 引用管理
 *
 * 端点：
 * - GET    /api/papers           - 列表（分页）
 * - GET    /api/papers/:id       - 详情
 * - POST   /api/papers           - 创建
 * - PUT    /api/papers/:id       - 更新
 * - DELETE /api/papers/:id       - 删除
 * - GET    /api/papers/search    - 搜索
 * - GET    /api/papers/stats     - 统计
 * - POST   /api/papers/import    - 导入
 * - GET    /api/papers/export    - 导出
 * - POST   /api/papers/:id/favorite - 收藏
 */
class PaperApiModule : public IModule {
public:
    PaperApiModule();
    ~PaperApiModule() override;

    std::string getName() const override { return "PaperApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Paper management API";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/papers"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取论文列表（分页）
     */
    std::vector<Paper> listPapers(int page = 1, int limit = 20, const std::string& sortBy = "created_at", bool ascending = false);

    /**
     * @brief 获取论文详情
     */
    std::optional<Paper> getPaper(int id);

    /**
     * @brief 创建论文
     */
    std::optional<Paper> createPaper(const Paper& paper);

    /**
     * @brief 更新论文
     */
    bool updatePaper(int id, const Paper& paper);

    /**
     * @brief 删除论文
     */
    bool deletePaper(int id);

    /**
     * @brief 搜索论文
     */
    std::vector<Paper> searchPapers(const PaperSearchCriteria& criteria, int page = 1, int limit = 20);

    /**
     * @brief 获取论文统计
     */
    PaperStats getStats();

    /**
     * @brief 批量导入论文
     */
    size_t importPapers(const std::vector<Paper>& papers);

    /**
     * @brief 导出论文（JSON/BibTeX）
     */
    std::string exportPapers(const std::vector<int>& ids, const std::string& format = "json");

    /**
     * @brief 标记为已读/未读
     */
    bool markAsRead(int id, bool read = true);

    /**
     * @brief 收藏/取消收藏
     */
    bool markAsFavorite(int id, bool favorite = true);

    /**
     * @brief 添加标签
     */
    bool addTag(int id, const std::string& tag);

    /**
     * @brief 移除标签
     */
    bool removeTag(int id, const std::string& tag);

    /**
     * @brief 上传PDF
     */
    bool uploadPDF(int id, const std::string& filePath);

    /**
     * @brief 下载PDF
     */
    std::string getPDFPath(int id);

    /**
     * @brief 按作者分组
     */
    std::map<std::string, std::vector<Paper>> groupByAuthor(const std::vector<Paper>& papers);

    /**
     * @brief 按年份分组
     */
    std::map<int, std::vector<Paper>> groupByYear(const std::vector<Paper>& papers);

    /**
     * @brief 按标签分组
     */
    std::map<std::string, std::vector<Paper>> groupByTag(const std::vector<Paper>& papers);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleListPapers(const std::map<std::string, std::string>& params);
    std::string handleGetPaper(const std::map<std::string, std::string>& params);
    std::string handleCreatePaper(const std::string& body);
    std::string handleUpdatePaper(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeletePaper(const std::map<std::string, std::string>& params);
    std::string handleSearch(const std::map<std::string, std::string>& params);
    std::string handleStats();
};

} // namespace PaperCrawler
