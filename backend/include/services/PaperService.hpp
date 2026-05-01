#pragma once

#include "domain/models/Paper.hpp"
#include "repositories/PaperRepository.hpp"
#include "data/IDatabase.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace PaperCrawler {

class PaperService {
public:
    explicit PaperService(std::shared_ptr<PaperRepository> repo);

    std::optional<Paper> getPaper(int id);
    std::vector<Paper> listPapers(int page, int limit, const std::string& sortBy, bool ascending);
    std::optional<Paper> createPaper(const Paper& paper);
    bool updatePaper(int id, const Paper& paper);
    bool deletePaper(int id);
    bool markAsRead(int id, bool read);
    bool markAsFavorite(int id, bool favorite);
    PaperStats getStats();
    std::vector<Paper> getRecentPapers(int limit);
    std::vector<Paper> getTopCitedPapers(int limit);
    size_t importPapers(const std::vector<Paper>& papers);
    bool paperExists(int id);

private:
    std::shared_ptr<PaperRepository> repo_;
};

} // namespace PaperCrawler
