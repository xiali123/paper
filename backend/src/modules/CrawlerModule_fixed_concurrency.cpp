// ✅ 并发修复版本 - 带事务保护和分布式锁
// 文件位置：backend/src/modules/CrawlerModule_fixed_concurrency.cpp

#include "modules/CrawlerModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/RedisDistributedLock.hpp"  // 新增：分布式锁
#include <spdlog/spdlog.h>
#include <sstream>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// 修复1：事务保护版本
// ============================================================================

void CrawlerModule::savePapersWithTransaction(
    const std::vector<CrawledPaper>& papers,
    const CrawlerTask& task) {

    auto logger = spdlog::get("Crawler");
    auto db = getDatabaseConnection();

    int papersAdded = 0;
    int papersUpdated = 0;

    // 对每篇论文使用事务保护
    for (const auto& paper : papers) {
        try {
            // 开始事务
            if (!db->beginTransaction()) {
                log(task.id, LogLevel::ERROR, "Failed to begin transaction");
                continue;
            }

            // 检查论文是否已存在（在事务中）
            std::ostringstream checkSql;
            checkSql << "SELECT id FROM papers WHERE title = '"
                     << db->escape(paper.title) << "' LIMIT 1";

            auto existing = db->query(checkSql.str());

            if (existing.empty()) {
                // 插入新论文
                std::ostringstream insertSql;
                insertSql << "INSERT INTO papers (title, authors, abstract, year, "
                         << "publication, url, doi, source, citation_count, "
                         << "created_at, updated_at) VALUES ("
                         << "'" << db->escape(paper.title) << "', "
                         << "'" << db->escape(paper.authors) << "', "
                         << "'" << db->escape(paper.abstract) << "', "
                         << paper.year << ", "
                         << "'" << db->escape(paper.publication) << "', "
                         << "'" << db->escape(paper.url) << "', "
                         << "'" << db->escape(paper.doi) << "', "
                         << "'" << db->escape(paper.source) << "', "
                         << paper.citationCount << ", "
                         << "NOW(), NOW())";

                if (db->execute(insertSql.str())) {
                    papersAdded++;
                    db->commitTransaction();  // 提交事务
                } else {
                    db->rollbackTransaction();  // 回滚事务
                    log(task.id, LogLevel::ERROR, "Failed to insert paper");
                }
            } else {
                // 更新现有论文
                std::ostringstream updateSql;
                updateSql << "UPDATE papers SET "
                         << "authors = '" << db->escape(paper.authors) << "', "
                         << "abstract = '" << db->escape(paper.abstract) << "', "
                         << "citation_count = " << paper.citationCount << ", "
                         << "updated_at = NOW() "
                         << "WHERE id = " << existing[0]["id"];

                if (db->execute(updateSql.str())) {
                    papersUpdated++;
                    db->commitTransaction();  // 提交事务
                } else {
                    db->rollbackTransaction();  // 回滚事务
                    log(task.id, LogLevel::ERROR, "Failed to update paper");
                }
            }

        } catch (const std::exception& e) {
            db->rollbackTransaction();  // 异常时回滚
            log(task.id, LogLevel::ERROR, "Failed to save paper: " + std::string(e.what()));
        }
    }

    if (logger) {
        logger->info("Task {} completed: {} papers added, {} papers updated",
                     task.id, papersAdded, papersUpdated);
    }
}

// ============================================================================
// 修复2：分布式锁版本
// ============================================================================

void CrawlerModule::savePapersWithDistributedLock(
    const std::vector<CrawledPaper>& papers,
    const CrawlerTask& task) {

    auto logger = spdlog::get("Crawler");
    auto db = getDatabaseConnection();
    auto lockManager = getDistributedLockManager();  // 获取分布式锁管理器

    int papersAdded = 0;
    int papersUpdated = 0;

    for (const auto& paper : papers) {
        // 为每篇论文创建分布式锁键
        std::string lockKey = "crawler:paper:" + std::to_string(std::hash<std::string>{}(paper.title));

        // 尝试获取锁（超时30秒）
        auto lock = lockManager->tryLock(lockKey, std::chrono::seconds(30));

        if (!lock) {
            log(task.id, LogLevel::WARN, "Failed to acquire lock for paper: " + paper.title);
            continue;  // 跳过，等待其他节点处理
        }

        try {
            // 在分布式锁保护下执行数据库操作
            if (!db->beginTransaction()) {
                log(task.id, LogLevel::ERROR, "Failed to begin transaction");
                continue;
            }

            std::ostringstream checkSql;
            checkSql << "SELECT id FROM papers WHERE title = '"
                     << db->escape(paper.title) << "' LIMIT 1";

            auto existing = db->query(checkSql.str());

            if (existing.empty()) {
                std::ostringstream insertSql;
                insertSql << "INSERT INTO papers ...";  // 同上

                if (db->execute(insertSql.str())) {
                    papersAdded++;
                    db->commitTransaction();
                } else {
                    db->rollbackTransaction();
                }
            } else {
                std::ostringstream updateSql;
                updateSql << "UPDATE papers SET ...";  // 同上

                if (db->execute(updateSql.str())) {
                    papersUpdated++;
                    db->commitTransaction();
                } else {
                    db->rollbackTransaction();
                }
            }

        } catch (const std::exception& e) {
            db->rollbackTransaction();
            log(task.id, LogLevel::ERROR, "Failed to save paper: " + std::string(e.what()));
        }

        // 自动释放锁（lock析构时释放）
    }

    if (logger) {
        logger->info("Task {} completed with distributed lock: {} added, {} updated",
                     task.id, papersAdded, papersUpdated);
    }
}

// ============================================================================
// 修复3：使用INSERT IGNORE + 唯一约束（最优方案）
// ============================================================================

void CrawlerModule::savePapersWithUpsert(
    const std::vector<CrawledPaper>& papers,
    const CrawlerTask& task) {

    auto logger = spdlog::get("Crawler");
    auto db = getDatabaseConnection();

    int papersAdded = 0;
    int papersUpdated = 0;

    for (const auto& paper : papers) {
        try {
            // 使用INSERT ... ON DUPLICATE KEY UPDATE（MySQL）
            std::ostringstream upsertSql;
            upsertSql << "INSERT INTO papers (title, authors, abstract, year, "
                     << "publication, url, doi, source, citation_count, "
                     << "created_at, updated_at) VALUES ("
                     << "'" << db->escape(paper.title) << "', "
                     << "'" << db->escape(paper.authors) << "', "
                     << "'" << db->escape(paper.abstract) << "', "
                     << paper.year << ", "
                     << "'" << db->escape(paper.publication) << "', "
                     << "'" << db->escape(paper.url) << "', "
                     << "'" << db->escape(paper.doi) << "', "
                     << "'" << db->escape(paper.source) << "', "
                     << paper.citationCount << ", "
                     << "NOW(), NOW()) "
                     << "ON DUPLICATE KEY UPDATE "
                     << "authors = VALUES(authors), "
                     << "abstract = VALUES(abstract), "
                     << "citation_count = VALUES(citation_count), "
                     << "updated_at = NOW()";

            if (db->execute(upsertSql.str())) {
                if (db->getAffectedRows() == 1) {
                    papersAdded++;  // 新插入
                } else {
                    papersUpdated++;  // 更新
                }
            }

        } catch (const std::exception& e) {
            log(task.id, LogLevel::ERROR, "Failed to save paper: " + std::string(e.what()));
        }
    }

    if (logger) {
        logger->info("Task {} completed with upsert: {} added, {} updated",
                     task.id, papersAdded, papersUpdated);
    }
}

} // namespace PaperCrawler
