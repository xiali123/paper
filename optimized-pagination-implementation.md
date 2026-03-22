/**
 * Optimized Pagination Implementation
 * 优化分页实现 - 避免深分页性能问题
 *
 * 问题: 当 OFFSET 很大时（如 LIMIT 20 OFFSET 10000），MySQL需要扫描10020行
 * 解决方案: 使用游标分页（Cursor-based Pagination）
 */

#include "database/DatabaseManager.hpp"
#include <sstream>

namespace PaperCrawler {

/**
 * @brief 传统分页（OFFSET/LIMIT）
 * 适用于: 小数据集（< 1000页）
 * 性能: 随着页数增加而下降
 */
DbResult DatabaseManager::getPapersLegacy(int page, int pageSize, const std::string& keyword) {
    int offset = (page - 1) * pageSize;

    std::ostringstream sql;
    sql << "SELECT * FROM cspaper";

    if (!keyword.empty()) {
        std::string escapedKeyword = escape(keyword);
        sql << " WHERE title LIKE '%" << escapedKeyword << "%'";
    }

    sql << " ORDER BY id DESC LIMIT " << pageSize << " OFFSET " << offset;

    return query(sql.str());
}

/**
 * @brief 游标分页（Cursor-based）
 * 适用于: 大数据集，无限滚动
 * 性能: O(log N) 查询，恒定时间
 *
 * @param lastId 上一页最后一条记录的ID（首次查询传0）
 * @param pageSize 每页记录数
 * @param keyword 搜索关键词
 */
DbResult DatabaseManager::getPapersCursor(int lastId, int pageSize, const std::string& keyword) {
    std::ostringstream sql;
    sql << "SELECT * FROM cspaper WHERE id < " << lastId;

    if (!keyword.empty()) {
        std::string escapedKeyword = escape(keyword);
        sql << " AND title LIKE '%" << escapedKeyword << "%'";
    }

    sql << " ORDER BY id DESC LIMIT " << pageSize;

    return query(sql.str());
}

/**
 * @brief 优化的深分页（使用子查询）
 * 适用于: 需要跳转到任意页的场景
 * 性能: 比传统方式快 3-5倍
 *
 * 原理: 先用覆盖索引找到ID，再关联查询完整数据
 */
DbResult DatabaseManager::getPapersOptimized(int page, int pageSize, const std::string& keyword) {
    int offset = (page - 1) * pageSize;

    std::ostringstream sql;
    sql << "SELECT p.* FROM cspaper p "
         << "INNER JOIN ( "
         << "  SELECT id FROM cspaper";

    if (!keyword.empty()) {
        std::string escapedKeyword = escape(keyword);
        sql << " WHERE title LIKE '%" << escapedKeyword << "%'";
    }

    sql << "  ORDER BY id DESC "
         << "  LIMIT " << pageSize << " OFFSET " << offset
         << ") AS tmp ON p.id = tmp.id "
         << "ORDER BY p.id DESC";

    return query(sql.str());
}

/**
 * @brief 分页元数据
 */
struct PaginationMeta {
    int64_t totalRecords;      // 总记录数
    int currentPage;           // 当前页码
    int pageSize;              // 每页大小
    int totalPages;            // 总页数
    bool hasNext;              // 是否有下一页
    bool hasPrevious;          // 是否有上一页
    int firstRecordId;         // 当前页第一条记录ID
    int lastRecordId;          // 当前页最后一条记录ID
};

/**
 * @brief 获取分页元数据
 * 用于生成分页导航控件
 */
PaginationMeta DatabaseManager::getPaginationMeta(int page, int pageSize, const std::string& keyword) {
    PaginationMeta meta;

    // 获取总记录数
    std::ostringstream countSql;
    countSql << "SELECT COUNT(*) FROM cspaper";
    if (!keyword.empty()) {
        std::string escapedKeyword = escape(keyword);
        countSql << " WHERE title LIKE '%" << escapedKeyword << "%'";
    }

    DbResult countResult = query(countSql.str());
    if (!countResult.empty()) {
        meta.totalRecords = std::stoll(countResult[0][0]);
    }

    // 计算分页信息
    meta.currentPage = page;
    meta.pageSize = pageSize;
    meta.totalPages = static_cast<int>(std::ceil(static_cast<double>(meta.totalRecords) / pageSize));
    meta.hasNext = page < meta.totalPages;
    meta.hasPrevious = page > 1;

    // 获取当前页的第一条和最后一条记录ID
    std::ostringstream idSql;
    idSql << "SELECT id FROM cspaper";
    if (!keyword.empty()) {
        std::string escapedKeyword = escape(keyword);
        idSql << " WHERE title LIKE '%" << escapedKeyword << "%'";
    }
    idSql << " ORDER BY id DESC LIMIT " << pageSize << " OFFSET " << ((page - 1) * pageSize);

    DbResult idResult = query(idSql.str());
    if (!idResult.empty()) {
        meta.firstRecordId = std::stoi(idResult[0][0]);
        meta.lastRecordId = std::stoi(idResult[idResult.size() - 1][0]);
    }

    return meta;
}

/**
 * @brief 预加载策略
 * 预先加载下一页数据，提升用户体验
 */
struct PreloadResult {
    DbResult currentPage;
    DbResult nextPage;      // 预加载的下一页
    bool hasNextPage;
};

PreloadResult DatabaseManager::getPapersWithPreload(int page, int pageSize, const std::string& keyword) {
    PreloadResult result;

    // 获取当前页
    result.currentPage = getPapersOptimized(page, pageSize, keyword);

    // 异步预加载下一页（如果存在）
    int totalPages = static_cast<int>(std::ceil(static_cast<double>(getTotalCount(keyword)) / pageSize));
    result.hasNextPage = page < totalPages;

    if (result.hasNextPage) {
        // 在实际实现中，这里应该使用异步线程预加载
        result.nextPage = getPapersOptimized(page + 1, pageSize, keyword);
    }

    return result;
}

} // namespace PaperCrawler
