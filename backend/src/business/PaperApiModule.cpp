#include <iostream>
#include "business/PaperApiModule.hpp"
#include "features/operations/ResponseHandlerModule.hpp"
#include <sstream>
#include <map>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// Mock 数据（从 simple_api_server.cpp 提取）
// ============================================================================

/**
 * @brief 获取 Mock 论文列表数据
 */
std::string getMockPapers() {
    return "[\n"
           "  {\n"
           "    \"id\": 1,\n"
           "    \"title\": \"Attention Is All You Need\",\n"
           "    \"authors\": \"Ashish Vaswani et al.\",\n"
           "    \"year\": 2023,\n"
           "    \"citation_count\": 150\n"
           "  },\n"
           "  {\n"
           "    \"id\": 2,\n"
           "    \"title\": \"BERT: Pre-training of Deep Bidirectional Transformers\",\n"
           "    \"authors\": \"Jacob Devlin et al.\",\n"
           "    \"year\": 2019,\n"
           "    \"citation_count\": 89000\n"
           "  },\n"
           "  {\n"
           "    \"id\": 3,\n"
           "    \"title\": \"Deep Residual Learning for Image Recognition\",\n"
           "    \"authors\": \"Kaiming He et al.\",\n"
           "    \"year\": 2016,\n"
           "    \"citation_count\": 150000\n"
           "  },\n"
           "  {\n"
           "    \"id\": 4,\n"
           "    \"title\": \"GPT-4 Technical Report\",\n"
           "    \"authors\": \"OpenAI\",\n"
           "    \"year\": 2023,\n"
           "    \"citation_count\": 5000\n"
           "  }\n"
           "]";
}

/**
 * @brief 获取单个论文的 Mock 数据
 */
std::string getMockPaper(int id) {
    std::ostringstream json;
    json << "[\n";
    json << "  {\n";
    json << "    \"id\": " << id << ",\n";
    json << "    \"title\": \"Sample Paper\",\n";
    json << "    \"authors\": \"Test Author\",\n";
    json << "    \"year\": 2023,\n";
    json << "    \"abstract\": \"This is a test abstract...\"\n";
    json << "  }\n";
    json << "]";
    return json.str();
}

// ============================================================================
// PaperApiModule 实现
// ============================================================================

class PaperApiModule::Impl {
public:
    // Mock 论文存储
    std::map<int, Paper> mockPapers_;

    Impl() {
        // 初始化 Mock 数据
        loadMockData();
    }

    void loadMockData() {
        Paper paper1;
        paper1.id = 1;
        paper1.title = "Attention Is All You Need";
        paper1.authors = "Ashish Vaswani et al.";
        paper1.year = 2023;
        paper1.citationCount = 150;
        paper1.isRead = true;
        paper1.isFavorite = true;

        Paper paper2;
        paper2.id = 2;
        paper2.title = "BERT: Pre-training of Deep Bidirectional Transformers";
        paper2.authors = "Jacob Devlin et al.";
        paper2.year = 2019;
        paper2.citationCount = 89000;
        paper2.isRead = false;
        paper2.isFavorite = true;

        Paper paper3;
        paper3.id = 3;
        paper3.title = "Deep Residual Learning for Image Recognition";
        paper3.authors = "Kaiming He et al.";
        paper3.year = 2016;
        paper3.citationCount = 150000;
        paper3.isRead = true;
        paper3.isFavorite = false;

        Paper paper4;
        paper4.id = 4;
        paper4.title = "GPT-4 Technical Report";
        paper4.authors = "OpenAI";
        paper4.year = 2023;
        paper4.citationCount = 5000;
        paper4.isRead = false;
        paper4.isFavorite = false;

        mockPapers_[1] = paper1;
        mockPapers_[2] = paper2;
        mockPapers_[3] = paper3;
        mockPapers_[4] = paper4;
    }
};

// ============================================================================

PaperApiModule::PaperApiModule()
    : impl_(std::make_unique<Impl>()) {
}

PaperApiModule::~PaperApiModule() = default;

bool PaperApiModule::initialize() {
    registerRoutes();
    std::cout << "PaperApiModule initialized" << std::endl;
    return true;
}

bool PaperApiModule::start() {
    std::cout << "PaperApiModule started" << std::endl;
    return true;
}

bool PaperApiModule::stop() {
    std::cout << "PaperApiModule stopped" << std::endl;
    return true;
}

void PaperApiModule::cleanup() {
    // 清理资源
}

std::vector<Paper> PaperApiModule::listPapers(int page, int limit, const std::string& sortBy, bool ascending) {
    std::vector<Paper> papers;

    // 从 Mock 数据加载
    for (const auto& pair : impl_->mockPapers_) {
        papers.push_back(pair.second);
    }

    // 排序
    // TODO: 根据 sortBy 参数排序

    // 分页
    size_t start = (page - 1) * limit;
    size_t end = std::min(start + limit, papers.size());

    if (start >= papers.size()) {
        return {}; // 空结果
    }

    return std::vector<Paper>(papers.begin() + start, papers.begin() + end);
}

std::optional<Paper> PaperApiModule::getPaper(int id) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Paper> PaperApiModule::createPaper(const Paper& paper) {
    // 生成新ID（简化）
    int newId = impl_->mockPapers_.size() + 1;
    Paper newPaper = paper;
    newPaper.id = newId;
    impl_->mockPapers_[newId] = newPaper;

    return newPaper;
}

bool PaperApiModule::updatePaper(int id, const Paper& paper) {
    auto it = impl_->mockPapers_.find(id);
    if (it == impl_->mockPapers_.end()) {
        return false;
    }

    Paper updatedPaper = paper;
    updatedPaper.id = id;
    it->second = updatedPaper;
    return true;
}

bool PaperApiModule::deletePaper(int id) {
    return impl_->mockPapers_.erase(id) > 0;
}

std::vector<Paper> PaperApiModule::searchPapers(const PaperSearchCriteria& criteria, int page, int limit) {
    std::vector<Paper> results;

    for (const auto& pair : impl_->mockPapers_) {
        const Paper& paper = pair.second;

        // 简单过滤逻辑
        bool match = true;

        if (!criteria.query.empty()) {
            // 搜索标题或作者
            if (paper.title.find(criteria.query) == std::string::npos &&
                paper.authors.find(criteria.query) == std::string::npos) {
                match = false;
            }
        }

        if (criteria.yearFrom > 0 && paper.year < criteria.yearFrom) {
            match = false;
        }

        if (criteria.yearTo > 0 && paper.year > criteria.yearTo) {
            match = false;
        }

        if (criteria.isRead && !paper.isRead) {
            match = false;
        }

        if (criteria.isFavorite && !paper.isFavorite) {
            match = false;
        }

        if (match) {
            results.push_back(paper);
        }
    }

    // 分页
    size_t start = (page - 1) * limit;
    size_t end = std::min(start + limit, results.size());

    if (start >= results.size()) {
        return {};
    }

    return std::vector<Paper>(results.begin() + start, results.begin() + end);
}

PaperStats PaperApiModule::getStats() {
    PaperStats stats;

    stats.totalPapers = impl_->mockPapers_.size();
    stats.readPapers = 0;
    stats.unreadPapers = 0;
    stats.favoritePapers = 0;

    for (const auto& pair : impl_->mockPapers_) {
        const Paper& paper = pair.second;

        if (paper.isRead) stats.readPapers++;
        else stats.unreadPapers++;

        if (paper.isFavorite) stats.favoritePapers++;

        stats.papersByYear[paper.year]++;
        stats.papersByJournal[paper.journal]++;
        stats.papersByAuthor[paper.authors]++;
    }

    return stats;
}

size_t PaperApiModule::importPapers(const std::vector<Paper>& papers) {
    size_t imported = 0;
    int nextId = impl_->mockPapers_.size() + 1;

    for (const auto& paper : papers) {
        Paper newPaper = paper;
        newPaper.id = nextId++;
        impl_->mockPapers_[newPaper.id] = newPaper;
        imported++;
    }

    return imported;
}

std::string PaperApiModule::exportPapers(const std::vector<int>& ids, const std::string& format) {
    // TODO: 实现导出逻辑
    if (format == "json") {
        // 构建 JSON
    } else if (format == "bibtex") {
        // 构建 BibTeX
    }

    return "{}";
}

bool PaperApiModule::markAsRead(int id, bool read) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        it->second.isRead = read;
        return true;
    }
    return false;
}

bool PaperApiModule::markAsFavorite(int id, bool favorite) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        it->second.isFavorite = favorite;
        return true;
    }
    return false;
}

bool PaperApiModule::addTag(int id, const std::string& tag) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        it->second.tags.push_back(tag);
        return true;
    }
    return false;
}

bool PaperApiModule::removeTag(int id, const std::string& tag) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        auto& tags = it->second.tags;
        auto tagIt = std::find(tags.begin(), tags.end(), tag);
        if (tagIt != tags.end()) {
            tags.erase(tagIt);
            return true;
        }
    }
    return false;
}

bool PaperApiModule::uploadPDF(int id, const std::string& filePath) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        it->second.pdfPath = filePath;
        return true;
    }
    return false;
}

std::string PaperApiModule::getPDFPath(int id) {
    auto it = impl_->mockPapers_.find(id);
    if (it != impl_->mockPapers_.end()) {
        return it->second.pdfPath;
    }
    return "";
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByAuthor(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) {
        grouped[paper.authors].push_back(paper);
    }
    return grouped;
}

std::map<int, std::vector<Paper>> PaperApiModule::groupByYear(const std::vector<Paper>& papers) {
    std::map<int, std::vector<Paper>> grouped;
    for (const auto& paper : papers) {
        grouped[paper.year].push_back(paper);
    }
    return grouped;
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByTag(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) {
        for (const auto& tag : paper.tags) {
            grouped[tag].push_back(paper);
        }
    }
    return grouped;
}

// ============================================================================
// 路由注册和处理
// ============================================================================

void PaperApiModule::registerRoutes() {
    // TODO: 注册路由到 Router
    // 示例：
    // auto& router = Router::getInstance();
    // router.get("/api/papers", [this](auto& req) { return this->handleListPapers(req); });
}

std::string PaperApiModule::handleListPapers(const std::map<std::string, std::string>& params) {
    int page = 1;
    int limit = 20;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) {
        page = std::stoi(pageIt->second);
    }

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) {
        limit = std::stoi(limitIt->second);
    }

    auto papers = listPapers(page, limit);

    // 构建JSON
    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& paper : papers) {
        if (!first) json << ",";
        first = false;
        json << paper.toJSON();
    }
    json << "]";

    return ResponseHandlerModule::buildPapersJsonResponse(json.str(), impl_->mockPapers_.size(), page, limit);
}

std::string PaperApiModule::handleGetPaper(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return ResponseHandlerModule::buildJsonResponse({{"error", "Missing paper ID"}}, 400);
    }

    int id = std::stoi(idIt->second);
    auto paper = getPaper(id);

    if (!paper.has_value()) {
        return ResponseHandlerModule::buildJsonResponse({{"error", "Paper not found"}}, 404);
    }

    return ResponseHandlerModule::buildPapersJsonResponse("[" + paper->toJSON() + "]", 1, 1, 1);
}

std::string PaperApiModule::handleCreatePaper(const std::string& body) {
    // TODO: 解析 JSON body
    Paper paper;
    auto newPaper = createPaper(paper);

    if (newPaper.has_value()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Paper created"},
            {"id", std::to_string(newPaper->id)}
        }, 201);
    }

    return ResponseHandlerModule::buildJsonResponse({{"error", "Failed to create paper"}}, 500);
}

std::string PaperApiModule::handleUpdatePaper(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return ResponseHandlerModule::buildJsonResponse({{"error", "Missing paper ID"}}, 400);
    }

    int id = std::stoi(idIt->second);

    // TODO: 解析 JSON body
    Paper paper;

    if (updatePaper(id, paper)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Paper updated"}
        });
    }

    return ResponseHandlerModule::buildJsonResponse({{"error", "Failed to update paper"}}, 500);
}

std::string PaperApiModule::handleDeletePaper(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return ResponseHandlerModule::buildJsonResponse({{"error", "Missing paper ID"}}, 400);
    }

    int id = std::stoi(idIt->second);

    if (deletePaper(id)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Paper deleted"}
        });
    }

    return ResponseHandlerModule::buildJsonResponse({{"error", "Failed to delete paper"}}, 500);
}

std::string PaperApiModule::handleSearch(const std::map<std::string, std::string>& params) {
    PaperSearchCriteria criteria;

    auto queryIt = params.find("query");
    if (queryIt != params.end()) {
        criteria.query = queryIt->second;
    }

    auto yearIt = params.find("yearFrom");
    if (yearIt != params.end()) {
        criteria.yearFrom = std::stoi(yearIt->second);
    }

    int page = 1;
    int limit = 20;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) {
        page = std::stoi(pageIt->second);
    }

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) {
        limit = std::stoi(limitIt->second);
    }

    auto papers = searchPapers(criteria, page, limit);

    // 构建JSON
    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& paper : papers) {
        if (!first) json << ",";
        first = false;
        json << paper.toJSON();
    }
    json << "]";

    return ResponseHandlerModule::buildPapersJsonResponse(json.str(), papers.size(), page, limit);
}

std::string PaperApiModule::handleStats() {
    auto stats = getStats();

    std::map<std::string, std::string> statsMap;
    statsMap["totalPapers"] = std::to_string(stats.totalPapers);
    statsMap["readPapers"] = std::to_string(stats.readPapers);
    statsMap["unreadPapers"] = std::to_string(stats.unreadPapers);
    statsMap["favoritePapers"] = std::to_string(stats.favoritePapers);

    return ResponseHandlerModule::buildStatsJsonResponse(statsMap);
}

} // namespace PaperCrawler
