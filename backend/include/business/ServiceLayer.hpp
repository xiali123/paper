// 服务层架构 - 分离业务逻辑
// 文件位置：backend/include/business/ServiceLayer.hpp

#pragma once

#include "data/IDatabase.hpp"
#include "core/ErrorHandler.hpp"
#include "business/PaperApiModule.hpp"
#include <memory>
#include <vector>
#include <optional>

namespace PaperCrawler {
namespace Services {

// ============================================================================
// 数据传输对象（DTO）
// ============================================================================

/**
 * @brief 论文DTO
 */
struct PaperDto {
    int id;
    std::string title;
    std::string authors;
    std::string abstract;
    int year;
    std::string publication;
    std::string url;
    std::string doi;
    std::string source;
    int citationCount;
    std::string createdAt;
    std::string updatedAt;

    /**
     * @brief 从数据库行创建DTO
     */
    static PaperDto fromRow(const std::map<std::string, std::string>& row);
};

/**
 * @brief 用户DTO
 */
struct UserDto {
    int id;
    std::string username;
    std::string email;
    std::string role;
    std::string createdAt;
};

/**
 * @brief 分页结果
 */
template<typename T>
struct PagedResult {
    std::vector<T> items;
    size_t total;
    size_t page;
    size_t pageSize;

    size_t getTotalPages() const {
        return (total + pageSize - 1) / pageSize;
    }
};

// ============================================================================
// 服务接口定义
// ============================================================================

/**
 * @brief 论文服务接口
 */
class IPaperService {
public:
    virtual ~IPaperService() = default;

    /**
     * @brief 根据ID获取论文
     */
    virtual std::optional<PaperDto> getPaperById(int id) = 0;

    /**
     * @brief 搜索论文
     */
    virtual PagedResult<PaperDto> searchPapers(
        const std::string& query,
        size_t page = 1,
        size_t pageSize = 20
    ) = 0;

    /**
     * @brief 获取热门论文
     */
    virtual std::vector<PaperDto> getPopularPapers(size_t limit = 10) = 0;

    /**
     * @brief 获取最新论文
     */
    virtual std::vector<PaperDto> getRecentPapers(size_t limit = 10) = 0;

    /**
     * @brief 创建论文
     */
    virtual PaperDto createPaper(const PaperDto& paper) = 0;

    /**
     * @brief 更新论文
     */
    virtual PaperDto updatePaper(int id, const PaperDto& paper) = 0;

    /**
     * @brief 删除论文
     */
    virtual bool deletePaper(int id) = 0;

    /**
     * @brief 检查论文是否存在
     */
    virtual bool paperExists(const std::string& title) = 0;
};

/**
 * @brief 用户服务接口
 */
class IUserService {
public:
    virtual ~IUserService() = default;

    /**
     * @brief 根据ID获取用户
     */
    virtual std::optional<UserDto> getUserById(int id) = 0;

    /**
     * @brief 根据用户名获取用户
     */
    virtual std::optional<UserDto> getUserByUsername(const std::string& username) = 0;

    /**
     * @brief 创建用户
     */
    virtual UserDto createUser(const UserDto& user) = 0;

    /**
     * @brief 验证用户凭据
     */
    virtual bool verifyCredentials(
        const std::string& username,
        const std::string& password
    ) = 0;
};

// ============================================================================
// 服务实现
// ============================================================================

/**
 * @brief 论文服务实现
 */
class PaperService : public IPaperService {
public:
    explicit PaperService(std::shared_ptr<IDatabase> database)
        : database_(database) {}

    std::optional<PaperDto> getPaperById(int id) override {
        try {
            std::ostringstream sql;
            sql << "SELECT * FROM papers WHERE id = " << id;

            auto results = database_->query(sql.str());
            if (results.empty()) {
                return std::nullopt;
            }

            return PaperDto::fromRow(results[0]);
        } catch (const std::exception& e) {
            getGlobalErrorHandler().handle(
                Errors::DatabaseError("Failed to get paper by id: " + std::string(e.what())),
                {{"paper_id", std::to_string(id)}}
            );
            throw;
        }
    }

    PagedResult<PaperDto> searchPapers(
        const std::string& query,
        size_t page,
        size_t pageSize
    ) override {
        try {
            // 使用全文搜索
            std::ostringstream sql;
            size_t offset = (page - 1) * pageSize;

            sql << "SELECT *, "
                 << "MATCH(title, abstract) AGAINST('" << database_->escape(query)
                 << "' IN NATURAL LANGUAGE MODE) AS relevance "
                 << "FROM papers "
                 << "WHERE MATCH(title, abstract) AGAINST('" << database_->escape(query)
                 << "' IN NATURAL LANGUAGE MODE) "
                 << "ORDER BY relevance DESC "
                 << "LIMIT " << pageSize << " OFFSET " << offset;

            auto results = database_->query(sql.str());

            // 获取总数
            std::ostringstream countSql;
            countSql << "SELECT COUNT(*) as total FROM papers "
                      << "WHERE MATCH(title, abstract) AGAINST('"
                      << database_->escape(query) << "' IN NATURAL LANGUAGE MODE)";

            auto countResults = database_->query(countSql.str());
            size_t total = std::stoull(countResults[0]["total"]);

            // 转换为DTO
            std::vector<PaperDto> papers;
            for (const auto& row : results) {
                papers.push_back(PaperDto::fromRow(row));
            }

            return PagedResult<PaperDto>{papers, total, page, pageSize};
        } catch (const std::exception& e) {
            getGlobalErrorHandler().handle(
                Errors::DatabaseError("Failed to search papers: " + std::string(e.what())),
                {{"query", query}}
            );
            throw;
        }
    }

    std::vector<PaperDto> getPopularPapers(size_t limit) override {
        std::ostringstream sql;
        sql << "SELECT * FROM papers "
             << "ORDER BY citation_count DESC "
             << "LIMIT " << limit;

        auto results = database_->query(sql.str());

        std::vector<PaperDto> papers;
        for (const auto& row : results) {
            papers.push_back(PaperDto::fromRow(row));
        }

        return papers;
    }

    std::vector<PaperDto> getRecentPapers(size_t limit) override {
        std::ostringstream sql;
        sql << "SELECT * FROM papers "
             << "ORDER BY created_at DESC "
             << "LIMIT " << limit;

        auto results = database_->query(sql.str());

        std::vector<PaperDto> papers;
        for (const auto& row : results) {
            papers.push_back(PaperDto::fromRow(row));
        }

        return papers;
    }

    PaperDto createPaper(const PaperDto& paper) override {
        try {
            // 检查重复
            if (paperExists(paper.title)) {
                throw Errors::DuplicatePaper(paper.title);
            }

            // 插入论文
            std::ostringstream sql;
            sql << "INSERT INTO papers (title, authors, abstract, year, "
                 << "publication, url, doi, source, citation_count, "
                 << "created_at, updated_at) VALUES ("
                 << "'" << database_->escape(paper.title) << "', "
                 << "'" << database_->escape(paper.authors) << "', "
                 << "'" << database_->escape(paper.abstract) << "', "
                 << paper.year << ", "
                 << "'" << database_->escape(paper.publication) << "', "
                 << "'" << database_->escape(paper.url) << "', "
                 << "'" << database_->escape(paper.doi) << "', "
                 << "'" << database_->escape(paper.source) << "', "
                 << paper.citationCount << ", "
                 << "NOW(), NOW())";

            if (!database_->execute(sql.str())) {
                throw Errors::DatabaseError("Failed to insert paper");
            }

            // 返回创建的论文
            int id = database_->getLastInsertId();
            auto created = getPaperById(id);
            if (created) {
                return *created;
            }

            throw Errors::InternalError("Failed to retrieve created paper");
        } catch (const AppException& e) {
            throw;  // 重新抛出已知异常
        } catch (const std::exception& e) {
            getGlobalErrorHandler().handle(
                Errors::InternalError("Failed to create paper: " + std::string(e.what()))
            );
            throw;
        }
    }

    PaperDto updatePaper(int id, const PaperDto& paper) override {
        // 实现更新逻辑
        // ...
        return paper;
    }

    bool deletePaper(int id) override {
        std::ostringstream sql;
        sql << "DELETE FROM papers WHERE id = " << id;
        return database_->execute(sql.str());
    }

    bool paperExists(const std::string& title) override {
        std::ostringstream sql;
        sql << "SELECT COUNT(*) as cnt FROM papers WHERE title_hash = SHA2('"
             << database_->escape(title) << "', 256)";

        auto results = database_->query(sql.str());
        return std::stoull(results[0]["cnt"]) > 0;
    }

private:
    std::shared_ptr<IDatabase> database_;
};

// ============================================================================
// 服务工厂
// ============================================================================

/**
 * @brief 服务工厂（依赖注入）
 */
class ServiceFactory {
public:
    static ServiceFactory& getInstance() {
        static ServiceFactory instance;
        return instance;
    }

    void initialize(std::shared_ptr<IDatabase> database) {
        database_ = database;
    }

    std::shared_ptr<IPaperService> getPaperService() {
        if (!paperService_) {
            paperService_ = std::make_shared<PaperService>(database_);
        }
        return paperService_;
    }

    std::shared_ptr<IUserService> getUserService() {
        if (!userService_) {
            // userService_ = std::make_shared<UserService>(database_);
        }
        return userService_;
    }

private:
    ServiceFactory() = default;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<IPaperService> paperService_;
    std::shared_ptr<IUserService> userService_;
};

} // namespace Services
} // namespace PaperCrawler

// ============================================================================
// DTO实现
// ============================================================================

namespace PaperCrawler {
namespace Services {

PaperDto PaperDto::fromRow(const std::map<std::string, std::string>& row) {
    PaperDto dto;
    dto.id = std::stoi(row.at("id"));
    dto.title = row.at("title");
    dto.authors = row.at("authors");
    dto.abstract = row.at("abstract");
    dto.year = std::stoi(row.at("year"));
    dto.publication = row.at("publication");
    dto.url = row.at("url");
    dto.doi = row.count("doi") ? row.at("doi") : "";
    dto.source = row.at("source");
    dto.citationCount = std::stoi(row.at("citation_count"));
    dto.createdAt = row.at("created_at");
    dto.updatedAt = row.at("updated_at");
    return dto;
}

} // namespace Services
} // namespace PaperCrawler
