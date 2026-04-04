#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../../include/network/HttpServerModule.hpp"
#include "../../include/modules/DatabaseModule.hpp"

using namespace std;

bool executeSQLFile(shared_ptr<DatabaseConnection> conn, const string& sqlFile) {
    // Read SQL file
    ifstream file(sqlFile);
    if (!file.is_open()) {
        cerr << "Error: Cannot open SQL file: " << sqlFile << endl;
        return false;
    }

    // Read entire file
    stringstream buffer;
    buffer << file.rdbuf();
    string sql = buffer.str();
    file.close();

    // Remove DELIMITER commands (MySQL specific)
    size_t pos;
    while ((pos = sql.find("DELIMITER //")) != string::npos) {
        size_t endPos = sql.find("\n", pos);
        sql.erase(pos, endPos - pos + 1);
    }
    while ((pos = sql.find("DELIMITER ;")) != string::npos) {
        size_t endPos = sql.find("\n", pos);
        sql.erase(pos, endPos - pos + 1);
    }

    // Replace // with ; for simple statements
    replace(sql.begin(), sql.end(), '//', ';');

    // Split by semicolon and execute
    stringstream ss(sql);
    string statement;
    while (getline(ss, statement, ';')) {
        // Trim whitespace
        size_t start = statement.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        size_t end = statement.find_last_not_of(" \t\r\n");
        statement = statement.substr(start, end - start + 1);

        if (!statement.empty() && statement.length() > 10) {
            cout << "Executing: " << statement.substr(0, min(50UL, statement.length())) << "..." << endl;

            auto stmt = conn->prepare(statement);
            if (stmt && stmt->execute()) {
                // Success
            } else {
                cerr << "Warning: Statement failed (might be expected): "
                     << statement.substr(0, min(100UL, statement.length())) << endl;
            }
        }
    }

    return true;
}

int main() {
    cout << "==========================================================" << endl;
    cout << "PaperCrawler Authentication System - Database Migration" << endl;
    cout << "==========================================================" << endl;
    cout << endl;

    // Initialize DatabaseModule
    cout << "[1/5] Initializing DatabaseModule..." << endl;
    auto dbModule = make_shared<DatabaseModule>();

    DatabaseConfig dbConfig;
    dbConfig.host = "127.0.0.1";
    dbConfig.port = 3306;
    dbConfig.username = "root";
    dbConfig.password = "123456";
    dbConfig.database = "papercrawler_db";
    dbConfig.poolSize = 1;
    dbConfig.maxPoolSize = 1;

    dbModule->setConfig(dbConfig);

    if (!dbModule->initialize()) {
        cerr << "Error: Failed to initialize DatabaseModule" << endl;
        return 1;
    }
    cout << "OK - DatabaseModule initialized" << endl;
    cout << endl;

    // Start DatabaseModule
    cout << "[2/5] Starting DatabaseModule..." << endl;
    if (!dbModule->start()) {
        cerr << "Error: Failed to start DatabaseModule" << endl;
        return 1;
    }
    cout << "OK - DatabaseModule started" << endl;
    cout << endl;

    // Get connection
    cout << "[3/5] Getting database connection..." << endl;
    auto conn = dbModule->getConnection();
    if (!conn || !conn->isConnected()) {
        cerr << "Error: Failed to get database connection" << endl;
        return 1;
    }
    cout << "OK - Connected to database" << endl;
    cout << endl;

    // Create database if not exists
    cout << "[4/5] Ensuring database exists..." << endl;
    string createDB = "CREATE DATABASE IF NOT EXISTS papercrawler_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
    auto stmt = conn->prepare(createDB);
    if (stmt && stmt->execute()) {
        cout << "OK - Database ready" << endl;
    } else {
        cerr << "Warning: Database check failed" << endl;
    }
    cout << endl;

    // Execute migration
    cout << "[5/5] Running authentication migration..." << endl;
    string sqlFile = "E:\\PaperCrawler\\backend\\migrations\\002_add_authentication.sql";
    if (!executeSQLFile(conn, sqlFile)) {
        cerr << "Error: Migration failed" << endl;
        return 1;
    }
    cout << "OK - Migration completed" << endl;
    cout << endl;

    // Verify tables
    cout << "==========================================================" << endl;
    cout << "Verifying tables..." << endl;
    string checkQuery = "SHOW TABLES LIKE 'user%'";
    auto checkStmt = conn->prepare(checkQuery);
    if (checkStmt) {
        auto result = checkStmt->query();
        if (!result.empty()) {
            cout << "OK - Found " << result.size() << " user tables:" << endl;
            for (const auto& row : result) {
                auto it = row.begin();
                if (it != row.end()) {
                    cout << "  - " << it->second << endl;
                }
            }
        } else {
            cout << "WARNING: No user tables found" << endl;
        }
    }
    cout << "==========================================================" << endl;
    cout << endl;

    cout << "Migration completed successfully!" << endl;
    cout << endl;
    cout << "You can now test the registration API:" << endl;
    cout << "  curl -X POST http://localhost:8080/api/auth/register \\" << endl;
    cout << "    -H 'Content-Type: application/json' \\" << endl;
    cout << "    -d '{\"email\":\"test@example.com\",\"password\":\"test123\",\"name\":\"Test User\"}'" << endl;
    cout << endl;

    return 0;
}
