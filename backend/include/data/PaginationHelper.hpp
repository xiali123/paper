#pragma once

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <sstream>

namespace PaperCrawler {

// 分页请求参数
struct PaginationRequest {
    size_t page = 1;
    size_t pageSize = 20;
    std::string sortBy;
    std::string sortOrder = "desc"; // asc | desc

    // 从查询参数解析
    static PaginationRequest fromQuery(const std::map<std::string, std::string>& params) {
        PaginationRequest req;
        auto it = params.find("page");
        if (it != params.end()) {
            try { req.page = std::stoul(it->second); } catch (...) {}
        }
        if (req.page == 0) req.page = 1;

        it = params.find("pageSize");
        if (it != params.end()) {
            try { req.pageSize = std::stoul(it->second); } catch (...) {}
        }
        it = params.find("page_size");
        if (it != params.end()) {
            try { req.pageSize = std::stoul(it->second); } catch (...) {}
        }
        if (req.pageSize == 0) req.pageSize = 20;
        if (req.pageSize > 100) req.pageSize = 100; // cap

        it = params.find("sortBy");
        if (it != params.end()) req.sortBy = it->second;

        it = params.find("sortOrder");
        if (it != params.end() && (it->second == "asc" || it->second == "desc")) {
            req.sortOrder = it->second;
        }

        return req;
    }

    size_t offset() const { return (page - 1) * pageSize; }

    std::string limitClause() const {
        std::ostringstream ss;
        ss << "LIMIT " << pageSize << " OFFSET " << offset();
        return ss.str();
    }

    std::string orderByClause(const std::string& defaultColumn = "id") const {
        std::ostringstream ss;
        ss << "ORDER BY " << (sortBy.empty() ? defaultColumn : sortBy);
        ss << " " << (sortOrder == "asc" ? "ASC" : "DESC");
        return ss.str();
    }
};

// 分页响应元数据
struct PaginationMeta {
    size_t total = 0;
    size_t page = 1;
    size_t pageSize = 20;
    size_t totalPages = 0;
    bool hasNext = false;
    bool hasPrev = false;

    static PaginationMeta create(size_t total, const PaginationRequest& req) {
        PaginationMeta meta;
        meta.total = total;
        meta.page = req.page;
        meta.pageSize = req.pageSize;
        meta.totalPages = (total == 0) ? 0 : static_cast<size_t>(std::ceil(static_cast<double>(total) / req.pageSize));
        meta.hasNext = req.page < meta.totalPages;
        meta.hasPrev = req.page > 1;
        return meta;
    }

    std::string toJson() const {
        std::ostringstream ss;
        ss << "\"pagination\":{\"total\":" << total
           << ",\"page\":" << page
           << ",\"pageSize\":" << pageSize
           << ",\"totalPages\":" << totalPages
           << ",\"hasNext\":" << (hasNext ? "true" : "false")
           << ",\"hasPrev\":" << (hasPrev ? "true" : "false")
           << "}";
        return ss.str();
    }
};

// 分页结果模板
template<typename T>
struct PaginatedResult {
    std::vector<T> items;
    PaginationMeta meta;

    // 快捷构造：从查询结果 + 总数
    static PaginatedResult<T> create(std::vector<T>&& items, size_t total, const PaginationRequest& req) {
        PaginatedResult<T> result;
        result.items = std::move(items);
        result.meta = PaginationMeta::create(total, req);
        return result;
    }

    bool empty() const { return items.empty(); }
    size_t size() const { return items.size(); }
};

} // namespace PaperCrawler
