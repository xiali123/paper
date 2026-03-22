#include "database/PaperRepository.hpp"
#include "core/Logger.hpp"
#include <sstream>

namespace PaperCrawler {

PaperRepository::PaperRepository(DatabaseManager& db) : db_(db) {}

int PaperRepository::insert(const Paper& paper) {
    std::vector<std::string> columns = {
        "kid", "type", "title", "qikanfull", "qikanjc",
        "year", "author", "qikanurl", "doiurl", "info", "qkid", "level"
    };

    std::vector<std::vector<std::string>> values;
    std::vector<std::string> row = {
        std::to_string(paper.getKid()),
        "'" + db_.escape(paper.getType()) + "'",
        "'" + db_.escape(paper.getTitle()) + "'",
        "'" + db_.escape(paper.getJournalFull()) + "'",
        "'" + db_.escape(paper.getJournalShort()) + "'",
        "'" + db_.escape(paper.getYear()) + "'",
        "'" + db_.escape(paper.getAuthor()) + "'",
        "'" + db_.escape(paper.getJournalUrl()) + "'",
        "'" + db_.escape(paper.getDoiUrl()) + "'",
        "'" + db_.escape(paper.getInfo()) + "'",
        std::to_string(paper.getQkid()),
        "'" + db_.escape(paper.getLevel()) + "'"
    };

    values.push_back(row);
    db_.insert("cspaper", columns, values);

    // Return last insert ID (simplified - should use mysql_insert_id)
    return 0;
}

void PaperRepository::insertBatch(const std::vector<Paper>& papers) {
    if (papers.empty()) return;

    std::vector<std::string> columns = {
        "kid", "type", "title", "qikanfull", "qikanjc",
        "year", "author", "qikanurl", "doiurl", "info", "qkid", "level"
    };

    std::vector<std::vector<std::string>> values;
    for (const auto& paper : papers) {
        std::vector<std::string> row = {
            std::to_string(paper.getKid()),
            "'" + db_.escape(paper.getType()) + "'",
            "'" + db_.escape(paper.getTitle()) + "'",
            "'" + db_.escape(paper.getJournalFull()) + "'",
            "'" + db_.escape(paper.getJournalShort()) + "'",
            "'" + db_.escape(paper.getYear()) + "'",
            " ",  // author - empty for now
            "'" + db_.escape(paper.getJournalUrl()) + "'",
            "'" + db_.escape(paper.getDoiUrl()) + "'",
            "",   // info
            "0",  // qkid
            "'" + db_.escape(paper.getLevel()) + "'"
        };
        values.push_back(row);
    }

    db_.insert("cspaper", columns, values);
    LOG_INFO("Inserted {} papers", papers.size());
}

void PaperRepository::update(const Paper& paper) {
    std::map<std::string, std::string> updates = {
        {"title", paper.getTitle()},
        {"qikanfull", paper.getJournalFull()},
        {"qikanjc", paper.getJournalShort()},
        {"year", paper.getYear()},
        {"qikanurl", paper.getJournalUrl()},
        {"doiurl", paper.getDoiUrl()},
        {"qkid", std::to_string(paper.getQkid())},
        {"level", paper.getLevel()}
    };

    std::map<std::string, std::string> conditions = {
        {"id", std::to_string(paper.getId())}
    };

    db_.update("cspaper", updates, conditions);
}

Paper PaperRepository::findById(int id) {
    std::string sql = "SELECT * FROM cspaper WHERE id = " + std::to_string(id);
    DbResult result = db_.query(sql);

    if (result.size() == 1) {
        return rowToPaper(result.getRows()[0]);
    }
    return Paper();
}

std::vector<Paper> PaperRepository::findByType(const std::string& type) {
    std::string sql = "SELECT * FROM cspaper WHERE type = '" + db_.escape(type) + "'";
    DbResult result = db_.query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.getRows()) {
        papers.push_back(rowToPaper(row));
    }
    return papers;
}

std::vector<Paper> PaperRepository::findPapersWithoutJournalInfo(const std::string& type) {
    std::string sql = "SELECT id, qikanjc FROM cspaper WHERE qkid = 0 AND type = '" +
                      db_.escape(type) + "'";
    DbResult result = db_.query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.getRows()) {
        Paper paper;
        paper.setId(std::stoi(row[0]));
        paper.setJournalShort(row[1]);
        papers.push_back(paper);
    }
    return papers;
}

void PaperRepository::updateJournalInfo(int paperId, int qkid,
                                       const std::string& journalFull,
                                       const std::string& level) {
    std::string sql = "UPDATE cspaper SET qkid = " + std::to_string(qkid) +
                      ", qikanfull = '" + db_.escape(journalFull) + "'" +
                      ", level = '" + db_.escape(level) + "'" +
                      " WHERE id = " + std::to_string(paperId);
    db_.execute(sql);
}

Paper PaperRepository::rowToPaper(const DbRow& row) {
    Paper paper;
    if (row.size() >= 12) {
        paper.setId(std::stoi(row[0]));
        paper.setKid(std::stoi(row[1]));
        paper.setType(row[2]);
        paper.setTitle(row[3]);
        paper.setJournalFull(row[4]);
        paper.setJournalShort(row[5]);
        paper.setYear(row[6]);
        paper.setAuthor(row[7]);
        paper.setJournalUrl(row[8]);
        paper.setDoiUrl(row[9]);
        paper.setInfo(row[10]);
        paper.setQkid(std::stoi(row[11]));
        if (row.size() >= 13) {
            paper.setLevel(row[12]);
        }
    }
    return paper;
}

} // namespace PaperCrawler
