#include <iostream>
#include "data/DatabaseModule.hpp"
#include "data/MySqlConnection.hpp"

using namespace PaperCrawler;

int main() {
    try {
        auto conn = std::make_shared<MySqlConnection>("127.0.0.1", 3306, "root", "123456", "papercrawler_db");

        if (!conn->isConnected()) {
            std::cerr << "Failed to connect to database" << std::endl;
            return 1;
        }

        std::cout << "Papers table structure in papercrawler_db:" << std::endl;
        auto results = conn->query("DESCRIBE papers");

        for (const auto& row : results) {
            std::string field = row.count("Field") ? row.at("Field") : "N/A";
            std::string type = row.count("Type") ? row.at("Type") : "N/A";
            std::string null = row.count("Null") ? row.at("Null") : "N/A";
            std::string key = row.count("Key") ? row.at("Key") : "N/A";

            std::cout << "  - " << field << " (" << type << ")";
            if (key == "PRI") std::cout << " [PRIMARY KEY]";
            if (key == "UNI") std::cout << " [UNIQUE]";
            std::cout << std::endl;
        }

        // Also check if table has data
        auto countResults = conn->query("SELECT COUNT(*) as count FROM papers");
        if (!countResults.empty()) {
            std::string count = countResults[0].count("count") ? countResults[0].at("count") : "0";
            std::cout << "\nTotal papers: " << count << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
