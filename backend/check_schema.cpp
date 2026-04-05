#include <iostream>
#include <vector>
#include <string>
#include "data/DatabaseModule.hpp"
#include "data/MySqlConnection.hpp"

using namespace PaperCrawler;

int main() {
    std::cout << "Database Schema Checker" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << std::endl;

    auto connection = std::make_shared<MySqlConnection>(
        "127.0.0.1", 3306, "root", "123456", "papercrawler"
    );

    if (!connection->isConnected()) {
        std::cerr << "[ERROR] Failed to connect to database" << std::endl;
        return 1;
    }

    std::cout << "[SUCCESS] Connected to papercrawler database" << std::endl;
    std::cout << std::endl;

    std::vector<std::string> tables = {
        "users",
        "user_sessions",
        "papers"
    };

    for (const auto& table : tables) {
        try {
            std::cout << "========================================" << std::endl;
            std::cout << "Table: " << table << std::endl;
            std::cout << "========================================" << std::endl;

            std::string sql = "DESCRIBE `" + table + "`";
            auto results = connection->query(sql);

            if (!results.empty()) {
                std::cout << "Columns (" << results.size() << "):" << std::endl;
                for (const auto& row : results) {
                    std::string field = row.count("Field") ? row.at("Field") : "N/A";
                    std::string type = row.count("Type") ? row.at("Type") : "N/A";
                    std::string null = row.count("Null") ? row.at("Null") : "N/A";
                    std::string key = row.count("Key") ? row.at("Key") : "N/A";
                    std::string extra = row.count("Extra") ? row.at("Extra") : "N/A";

                    std::cout << "  - " << field << " (" << type << ")";
                    if (key == "PRI") std::cout << " [PRIMARY KEY]";
                    if (key == "UNI") std::cout << " [UNIQUE]";
                    if (extra.find("auto_increment") != std::string::npos) std::cout << " [AUTO_INCREMENT]";
                    std::cout << std::endl;
                }

                // Get row count
                std::string countSql = "SELECT COUNT(*) as count FROM `" + table + "`";
                auto countResults = connection->query(countSql);
                if (!countResults.empty()) {
                    std::string count = countResults[0].count("count") ? countResults[0]["count"] : "0";
                    std::cout << "Total rows: " << count << std::endl;
                }
            } else {
                std::cout << "[WARN] Table '" << table << "' doesn't exist" << std::endl;
            }

            std::cout << std::endl;

        } catch (const std::exception& e) {
            std::cout << "[ERROR] Failed to describe table " << table << ": " << e.what() << std::endl;
            std::cout << std::endl;
        }
    }

    return 0;
}
