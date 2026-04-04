#pragma once

#include <string>
#include <vector>
#include <optional>
#include "domain/models/Paper.hpp"

namespace PaperCrawler::Domain {

/**
 * @brief 论文查询条件
 */
struct PaperQuery {
    std::string searchQuery;           // 搜索关键词
    std::vector<std::string> authors;  // 作者筛选
    std::string yearFrom;              // 起始年份
    std::string yearTo;                // 结束年份
    std::string venue;                 // 发表场所
    std::vector<std::string> keywords; // 关键词筛选
    int limit{100};                    // 结果数量限制
    int offset{0};                     // 偏移量（分页）

    /**
     * @brief 验证查询条件
     */
    bool isValid() const {
        return limit > 0 && limit <= 1000 && offset >= 0;
    }
};

/**
 * @brief 论文仓储接口
 *
 * 负责论文数据的持久化和查询
 * 遵循仓储模式（Repository Pattern）
 */
class IPaperRepository {
public:
    virtual ~IPaperRepository() = default;

    /**
     * @brief 保存论文
     *
     * @param paper 论文对象
     * @return 成功返回true
     *
     * 示例：
     * @code
     * Paper paper;
     * paper.id = "2301.00001";
     * paper.title = "Deep Learning for Paper Crawling";
     *
     * bool success = repository->save(paper);
     * @endcode
     */
    virtual bool save(const Paper& paper) = 0;

    /**
     * @brief 批量保存论文
     *
     * @param papers 论文列表
     * @return 成功保存的数量
     */
    virtual size_t batchSave(const std::vector<Paper>& papers) = 0;

    /**
     * @brief 根据ID查找论文
     *
     * @param id 论文ID
     * @return 论文对象（如果存在）
     *
     * 示例：
     * @code
     * auto paper = repository->findById("2301.00001");
     * if (paper.has_value()) {
     *     std::cout << paper->title << std::endl;
     * }
     * @endcode
     */
    virtual std::optional<Paper> findById(const std::string& id) = 0;

    /**
     * @brief 根据多个ID批量查找论文
     *
     * @param ids 论文ID列表
     * @return 论文列表
     */
    virtual std::vector<Paper> findByIds(const std::vector<std::string>& ids) = 0;

    /**
     * @brief 根据作者查找论文
     *
     * @param author 作者姓名
     * @param limit 结果数量限制
     * @return 论文列表
     *
     * 示例：
     * @code
     * auto papers = repository->findByAuthor("Geoffrey Hinton", 50);
     * for (const auto& paper : papers) {
     *     std::cout << paper.title << std::endl;
     * }
     * @endcode
     */
    virtual std::vector<Paper> findByAuthor(
        const std::string& author,
        int limit = 100
    ) = 0;

    /**
     * @brief 搜索论文
     *
     * @param query 查询条件
     * @return 论文列表
     *
     * 示例：
     * @code
     * PaperQuery query;
     * query.searchQuery = "deep learning";
     * query.authors = {"Geoffrey Hinton"};
     * query.yearFrom = "2015";
     * query.limit = 50;
     *
     * auto papers = repository->search(query);
     * @endcode
     */
    virtual std::vector<Paper> search(const PaperQuery& query) = 0;

    /**
     * @brief 全文搜索
     *
     * @param searchText 搜索文本
     * @param limit 结果数量限制
     * @return 论文列表
     */
    virtual std::vector<Paper> fullTextSearch(
        const std::string& searchText,
        int limit = 100
    ) = 0;

    /**
     * @brief 更新论文
     *
     * @param paper 论文对象
     * @return 成功返回true
     */
    virtual bool update(const Paper& paper) = 0;

    /**
     * @brief 删除论文
     *
     * @param id 论文ID
     * @return 成功返回true
     */
    virtual bool deleteById(const std::string& id) = 0;

    /**
     * @brief 批量删除论文
     *
     * @param ids 论文ID列表
     * @return 成功删除的数量
     */
    virtual size_t batchDelete(const std::vector<std::string>& ids) = 0;

    /**
     * @brief 获取论文总数
     */
    virtual size_t count() const = 0;

    /**
     * @brief 检查论文是否存在
     *
     * @param id 论文ID
     * @return 存在返回true
     */
    virtual bool exists(const std::string& id) const = 0;

    /**
     * @brief 获取最近添加的论文
     *
     * @param limit 数量限制
     * @return 论文列表
     */
    virtual std::vector<Paper> getRecent(int limit = 10) = 0;

    /**
     * @brief 获取热门论文（按引用数）
     *
     * @param limit 数量限制
     * @return 论文列表
     */
    virtual std::vector<Paper> getTopCited(int limit = 10) = 0;

    /**
     * @brief 获取统计信息
     */
    struct RepositoryStats {
        size_t totalPapers{0};
        size_t totalAuthors{0};
        size_t totalVenues{0};
        uint64_t totalCitations{0};
    };
    virtual RepositoryStats getStats() const = 0;

    /**
     * @brief 开始事务
     */
    virtual void beginTransaction() = 0;

    /**
     * @brief 提交事务
     */
    virtual void commitTransaction() = 0;

    /**
     * @brief 回滚事务
     */
    virtual void rollbackTransaction() = 0;
};

} // namespace PaperCrawler::Domain
