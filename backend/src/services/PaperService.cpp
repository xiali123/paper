#include "services/PaperService.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

PaperService::PaperService(std::shared_ptr<PaperRepository> repo)
    : repo_(repo) {}

std::optional<Paper> PaperService::getPaper(int id) {
    if (!repo_) return std::nullopt;
    return repo_->getPaperById(id);
}

std::vector<Paper> PaperService::listPapers(int page, int limit, const std::string& sortBy, bool ascending) {
    if (!repo_) return {};
    return repo_->listPapers(page, limit, sortBy, ascending);
}

std::optional<Paper> PaperService::createPaper(const Paper& paper) {
    if (!repo_) return std::nullopt;
    if (paper.title.empty()) {
        spdlog::warn("[PaperService] createPaper: title is empty");
        return std::nullopt;
    }
    return repo_->createPaper(paper);
}

bool PaperService::updatePaper(int id, const Paper& paper) {
    if (!repo_) return false;
    return repo_->updatePaper(id, paper);
}

bool PaperService::deletePaper(int id) {
    if (!repo_) return false;
    return repo_->deletePaper(id);
}

bool PaperService::markAsRead(int id, bool read) {
    if (!repo_) return false;
    return repo_->markAsRead(id, read);
}

bool PaperService::markAsFavorite(int id, bool favorite) {
    if (!repo_) return false;
    return repo_->markAsFavorite(id, favorite);
}

PaperStats PaperService::getStats() {
    if (!repo_) return {};
    return repo_->getPaperStats();
}

std::vector<Paper> PaperService::getRecentPapers(int limit) {
    if (!repo_) return {};
    auto domainPapers = repo_->getRecent(limit);
    std::vector<Paper> papers;
    papers.reserve(domainPapers.size());
    for (auto& p : domainPapers) {
        papers.push_back(std::move(p));
    }
    return papers;
}

std::vector<Paper> PaperService::getTopCitedPapers(int limit) {
    if (!repo_) return {};
    auto domainPapers = repo_->getTopCited(limit);
    std::vector<Paper> papers;
    papers.reserve(domainPapers.size());
    for (auto& p : domainPapers) {
        papers.push_back(std::move(p));
    }
    return papers;
}

size_t PaperService::importPapers(const std::vector<Paper>& papers) {
    if (!repo_) return 0;
    size_t imported = 0;
    for (const auto& paper : papers) {
        if (repo_->createPaper(paper)) {
            ++imported;
        }
    }
    return imported;
}

bool PaperService::paperExists(int id) {
    if (!repo_) return false;
    return repo_->getPaperById(id).has_value();
}

} // namespace PaperCrawler
