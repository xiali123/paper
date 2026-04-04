#include <iostream>
#include <fstream>
#include <string>
#include <mysql.h>

using namespace std;

bool executeSQL(MYSQL* conn, const string& sqlFile) {
    // Read SQL file
    ifstream file(sqlFile);
    if (!file.is_open()) {
        cerr << "Error: Cannot open SQL file: " << sqlFile << endl;
        return false;
    }

    string sql((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // Split by semicolon and execute each statement
    size_t pos = 0;
    while ((pos = sql.find(';')) != string::npos) {
        string statement = sql.substr(0, pos);

        // Skip empty statements
        if (statement.find_first_not_of(" \t\r\n") != string::npos) {
            if (mysql_query(conn, statement.c_str()) != 0) {
                cerr << "SQL Error: " << mysql_error(conn) << endl;
                cerr << "Statement: " << statement.substr(0, 100) << "..." << endl;
            }
        }

        sql.erase(0, pos + 1);
    }

    return true;
}

int main() {
    cout << "==========================================================" << endl;
    cout << "PaperCrawler Authentication System - Database Migration" << endl;
    cout << "==========================================================" << endl;
    cout << endl;

    // Initialize MySQL
    MYSQL* conn = mysql_init(NULL);
    if (!conn) {
        cerr << "Error: Failed to initialize MySQL" << endl;
        return 1;
    }

    // Connect to MySQL
    cout << "[1/4] Connecting to MySQL..." << endl;
    const char* host = "127.0.0.1";
    const char* user = "root";
    const char* pass = "123456";
    const char* db = NULL; // Connect without database first

    if (!mysql_real_connect(conn, host, user, pass, db, 3306, NULL, 0)) {
        cerr << "Error: Failed to connect to MySQL" << endl;
        cerr << "MySQL Error: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return 1;
    }
    cout << "OK - Connected successfully" << endl;
    cout << endl;

    // Create database
    cout << "[2/4] Creating database papercrawler_db..." << endl;
    string createDB = "CREATE DATABASE IF NOT EXISTS papercrawler_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
    if (mysql_query(conn, createDB.c_str()) != 0) {
        cerr << "Error: Failed to create database" << endl;
        cerr << "MySQL Error: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return 1;
    }
    cout << "OK - Database ready" << endl;
    cout << endl;

    // Select database
    cout << "[3/4] Selecting database..." << endl;
    if (mysql_select_db(conn, "papercrawler_db") != 0) {
        cerr << "Error: Failed to select database" << endl;
        mysql_close(conn);
        return 1;
    }
    cout << "OK - Database selected" << endl;
    cout << endl;

    // Execute migration
    cout << "[4/4] Running migration 002_add_authentication.sql..." << endl;
    string sqlFile = "E:\\PaperCrawler\\backend\\migrations\\002_add_authentication.sql";
    if (!executeSQL(conn, sqlFile)) {
        cerr << "Error: Migration failed" << endl;
        mysql_close(conn);
        return 1;
    }
    cout << "OK - Migration completed" << endl;
    cout << endl;

    // Verify tables
    cout << "==========================================================" << endl;
    cout "Verifying tables..." << endl;
    if (mysql_query(conn, "SHOW TABLES LIKE 'user%'") != 0) {
        cerr << "Error: Failed to query tables" << endl;
    } else {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result) {
            int count = mysql_num_rows(result);
            if (count > 0) {
                cout << "OK - Created " << count << " user tables:" << endl;
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(result))) {
                    cout << "  - " << row[0] << endl;
                }
            } else {
                cout << "WARNING: No user tables found" << endl;
            }
            mysql_free_result(result);
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

    mysql_close(conn);
    return 0;
}
