#pragma once

#include "database/DatabaseManager.hpp"
#include "models/Journal.hpp"
#include <vector>
#include <string>
#include <memory>

namespace PaperCrawler {

/**
 * @brief Repository for Journal data access
 */
class JournalRepository {
public:
    explicit JournalRepository(DatabaseManager& db);
    ~JournalRepository() = default;

    /**
     * @brief Insert a journal into database
     * @param journal Journal object
     * @return Inserted journal ID
     */
    int insert(const Journal& journal);

    /**
     * @brief Insert multiple journals
     * @param journals Vector of journals
     */
    void insertBatch(const std::vector<Journal>& journals);

    /**
     * @brief Update journal information
     * @param journal Journal object
     */
    void update(const Journal& journal);

    /**
     * @brief Find journal by name
     * @param name Journal short name
     * @return Journal object or empty if not found
     */
    Journal findByName(const std::string& name);

    /**
     * @brief Find journal by ID
     * @param id Journal ID
     * @return Journal object
     */
    Journal findById(int id);

    /**
     * @brief Get all journals
     * @return Vector of all journals
     */
    std::vector<Journal> findAll();

    /**
     * @brief Find journals with missing information
     * @return Vector of journals needing update
     */
    std::vector<Journal> findJournalsWithoutInfo();

    /**
     * @brief Build journal lookup map (name -> Journal)
     * @return Map of journal name to Journal object
     */
    std::map<std::string, Journal> buildJournalMap();

private:
    DatabaseManager& db_;

    /**
     * @brief Convert database row to Journal object
     * @param row Database row
     * @return Journal object
     */
    Journal rowToJournal(const DbRow& row);
};

} // namespace PaperCrawler
