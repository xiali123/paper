#pragma once

#include "domain/IPaperRepository.hpp"
#include "data/IDatabase.hpp"
#include "domain/models/Paper.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace PaperCrawler {

class PaperRepository : public Domain::IPaperRepository {
public:
    explicit PaperRepository(std::shared_ptr<IDatabase> database);
    ~PaperRepository() override = default;

    bool save(const Domain::Paper& paper) override;
    size_t batchSave(const std::vector<Domain::Paper>& papers) override;
    std::optional<Domain::Paper> findById(const std::string& id) override;
    std::vector<Domain::Paper> findByIds(const std::vector<std::string>& ids) override;
    std::vector<Domain::Paper> findByAuthor(const std::string& author, int limit = 100) override;
    std::vector<Domain::Paper> search(const Domain::PaperQuery& query) override;
    std::vector<Domain::Paper> fullTextSearch(const std::string& searchText, int limit = 100) override;
    bool update(const Domain::Paper& paper) override;
    bool deleteById(const std::string& id) override;
    size_t batchDelete(const std::vector<std::string>& ids) override;
    size_t count() const override;
    bool exists(const std::string& id) const override;
    std::vector<Domain::Paper> getRecent(int limit = 10) override;
    std::vector<Domain::Paper> getTopCited(int limit = 10) override;
    RepositoryStats getStats() const override;
    void beginTransaction() override;
    void commitTransaction() override;
    void rollbackTransaction() override;

    // Additional methods for PaperApiModule compatibility
    std::optional<Paper> getPaperById(int id);
    std::vector<Paper> listPapers(int page, int limit, const std::string& sortBy, bool ascending);
    std::optional<Paper> createPaper(const Paper& paper);
    bool updatePaper(int id, const Paper& paper);
    bool deletePaper(int id);
    bool markAsRead(int id, bool read);
    bool markAsFavorite(int id, bool favorite);
    PaperStats getPaperStats();

private:
    std::shared_ptr<IDatabase> database_;
    Paper paperFromDbRow(const std::map<std::string, std::string>& row);
};

} // namespace PaperCrawler
