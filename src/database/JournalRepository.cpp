#include "database/JournalRepository.hpp"
#include <sstream>

namespace PaperCrawler {

JournalRepository::JournalRepository(DatabaseManager& db) : db_(db) {}

int JournalRepository::insert(const Journal& journal) {
    std::vector<std::string> columns = {
        "name", "fullname", "level", "flevel", "info", "url"
    };

    std::vector<std::vector<std::string>> values;
    std::vector<std::string> row = {
        "'" + db_.escape(journal.getName()) + "'",
        "'" + db_.escape(journal.getFullName()) + "'",
        "'" + db_.escape(journal.getLevel()) + "'",
        "'" + db_.escape(journal.getFLevel()) + "'",
        "'" + db_.escape(journal.getInfo()) + "'",
        "'" + db_.escape(journal.getUrl()) + "'"
    };

    values.push_back(row);
    db_.insert("qikantb", columns, values);
    return 0;
}

void JournalRepository::insertBatch(const std::vector<Journal>& journals) {
    if (journals.empty()) return;

    std::vector<std::string> columns = {
        "name", "fullname", "level", "flevel", "info", "url"
    };

    std::vector<std::vector<std::string>> values;
    for (const auto& journal : journals) {
        std::vector<std::string> row = {
            "'" + db_.escape(journal.getName()) + "'",
            "'" + db_.escape(journal.getFullName()) + "'",
            "'" + db_.escape(journal.getLevel()) + "'",
            "'" + db_.escape(journal.getFLevel()) + "'",
            "'" + db_.escape(journal.getInfo()) + "'",
            "'" + db_.escape(journal.getUrl()) + "'"
        };
        values.push_back(row);
    }

    db_.insert("qikantb", columns, values);
    LOG_INFO("Inserted {} journals", journals.size());
}

void JournalRepository::update(const Journal& journal) {
    std::map<std::string, std::string> updates = {
        {"fullname", journal.getFullName()},
        {"level", journal.getLevel()},
        {"flevel", journal.getFLevel()},
        {"info", journal.getInfo()}
    };

    std::map<std::string, std::string> conditions = {
        {"name", journal.getName()}
    };

    db_.update("qikantb", updates, conditions);
}

Journal JournalRepository::findByName(const std::string& name) {
    std::string sql = "SELECT * FROM qikantb WHERE name = '" + db_.escape(name) + "'";
    DbResult result = db_.query(sql);

    if (result.size() >= 1) {
        return rowToJournal(result.getRows()[0]);
    }
    return Journal();
}

Journal JournalRepository::findById(int id) {
    std::string sql = "SELECT * FROM qikantb WHERE id = " + std::to_string(id);
    DbResult result = db_.query(sql);

    if (result.size() == 1) {
        return rowToJournal(result.getRows()[0]);
    }
    return Journal();
}

std::vector<Journal> JournalRepository::findAll() {
    std::string sql = "SELECT * FROM qikantb";
    DbResult result = db_.query(sql);

    std::vector<Journal> journals;
    for (const auto& row : result.getRows()) {
        journals.push_back(rowToJournal(row));
    }
    return journals;
}

std::vector<Journal> JournalRepository::findJournalsWithoutInfo() {
    std::string sql = "SELECT * FROM qikantb WHERE fullname = '' OR info = ''";
    DbResult result = db_.query(sql);

    std::vector<Journal> journals;
    for (const auto& row : result.getRows()) {
        journals.push_back(rowToJournal(row));
    }
    return journals;
}

std::map<std::string, Journal> JournalRepository::buildJournalMap() {
    std::string sql = "SELECT * FROM qikantb";
    DbResult result = db_.query(sql);

    std::map<std::string, Journal> journalMap;
    for (const auto& row : result.getRows()) {
        Journal journal = rowToJournal(row);
        journalMap[journal.getName()] = journal;
    }
    return journalMap;
}

Journal JournalRepository::rowToJournal(const DbRow& row) {
    Journal journal;
    if (row.size() >= 7) {
        journal.setId(std::stoi(row[0]));
        journal.setName(row[1]);
        journal.setFullName(row[2]);
        journal.setLevel(row[3]);
        journal.setFLevel(row[4]);
        journal.setInfo(row[5]);
        journal.setUrl(row[6]);
    }
    return journal;
}

} // namespace PaperCrawler
