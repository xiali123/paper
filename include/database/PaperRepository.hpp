#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Paper.hpp"
#include <vector>
#include <string>
#include <memory>

namespace PaperCrawler {

/**
 * @brief Repository for Paper data access
 *
 * Implements Repository pattern for paper database operations
 */
class PaperRepository {
public:
    explicit PaperRepository(DatabaseManager& db);
    ~PaperRepository() = default;

    /**
     * @brief Insert a paper into database
     * @param paper Paper object
     * @return Inserted paper ID
     */
    int insert(const Paper& paper);

    /**
     * @brief Insert multiple papers
     * @param papers Vector of papers
     */
    void insertBatch(const std::vector<Paper>& papers);

    /**
     * @brief Update paper information
     * @param paper Paper object with updated data
     */
    void update(const Paper& paper);

    /**
     * @brief Find paper by ID
     * @param id Paper ID
     * @return Paper object or empty paper if not found
     */
    Paper findById(int id);

    /**
     * @brief Find papers by type (keyword)
     * @param type Search keyword
     * @return Vector of papers
     */
    std::vector<Paper> findByType(const std::string& type);

    /**
     * @brief Find papers that need journal info update
     * @param type Search type
     * @return Vector of papers with qkid=0
     */
    std::vector<Paper> findPapersWithoutJournalInfo(const std::string& type);

    /**
     * @brief Update journal information for paper
     * @param paperId Paper ID
     * @param qkid Journal ID
     * @param journalFull Journal full name
     * @param level Journal level
     */
    void updateJournalInfo(int paperId, int qkid, const std::string& journalFull, const std::string& level);

private:
    DatabaseManager& db_;

    /**
     * @brief Convert database row to Paper object
     * @param row Database row
     * @return Paper object
     */
    Paper rowToPaper(const DbRow& row);
};

} // namespace PaperCrawler
