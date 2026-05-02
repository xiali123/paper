#pragma once

#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"
#include "data/PaginationHelper.hpp"
#include <optional>
#include <vector>
#include <string>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

template<typename T>
class Repository {
public:
    Repository(std::shared_ptr<IDatabase> db, const std::string& tableName)
        : db_(db), tableName_(tableName) {}

    virtual ~Repository() = default;

    // 子类必须实现: 从数据库行转换为实体
    virtual T fromRow(const std::map<std::string, std::string>& row) const = 0;

    std::optional<T> findById(int id) {
        if (!db_) return std::nullopt;
        try {
            PreparedStatement stmt(db_, "SELECT * FROM " + tableName_ + " WHERE id = ?");
            stmt.bind(0, id);
            auto results = stmt.query();
            if (results.empty()) return std::nullopt;
            return fromRow(results[0]);
        } catch (const std::exception& e) {
            spdlog::error("[Repository] findById failed: {}", e.what());
            return std::nullopt;
        }
    }

    std::vector<T> findAll(int page = 1, int pageSize = 20) {
        if (!db_) return {};
        try {
            int offset = (page - 1) * pageSize;
            PreparedStatement stmt(db_, "SELECT * FROM " + tableName_ + " ORDER BY id DESC LIMIT ? OFFSET ?");
            stmt.bind(0, pageSize);
            stmt.bind(1, offset);
            auto results = stmt.query();
            std::vector<T> items;
            items.reserve(results.size());
            for (const auto& row : results) {
                items.push_back(fromRow(row));
            }
            return items;
        } catch (const std::exception& e) {
            spdlog::error("[Repository] findAll failed: {}", e.what());
            return {};
        }
    }

    bool existsById(int id) {
        if (!db_) return false;
        try {
            PreparedStatement stmt(db_, "SELECT COUNT(*) as cnt FROM " + tableName_ + " WHERE id = ?");
            stmt.bind(0, id);
            auto results = stmt.query();
            return !results.empty() && results[0].at("cnt") != "0";
        } catch (const std::exception& e) {
            spdlog::error("[Repository] existsById failed: {}", e.what());
            return false;
        }
    }

    int count() {
        if (!db_) return 0;
        try {
            auto results = db_->query("SELECT COUNT(*) as cnt FROM " + tableName_);
            if (results.empty()) return 0;
            return std::stoi(results[0].at("cnt"));
        } catch (const std::exception& e) {
            spdlog::error("[Repository] count failed: {}", e.what());
            return 0;
        }
    }

    bool deleteById(int id) {
        if (!db_) return false;
        try {
            PreparedStatement stmt(db_, "DELETE FROM " + tableName_ + " WHERE id = ?");
            stmt.bind(0, id);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[Repository] deleteById failed: {}", e.what());
            return false;
        }
    }

protected:
    std::shared_ptr<IDatabase> db_;
    std::string tableName_;
};

} // namespace PaperCrawler
