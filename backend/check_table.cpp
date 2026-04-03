#include <iostream>
#include <windows.h>
#include <string>

// Simple MySQL API declarations
typedef void* MYSQL;
typedef void* MYSQL_RES;
typedef char** MYSQL_ROW;

extern "C" {
    __declspec(dllimport) MYSQL* mysql_init(MYSQL* mysql);
    __declspec(dllimport) int mysql_real_connect(MYSQL* mysql, const char* host, const char* user, const char* passwd, const char* db, unsigned int port, const char* unix_socket, unsigned long clientflag);
    __declspec(dllimport) void mysql_close(MYSQL* sock);
    __declspec(dllimport) int mysql_query(MYSQL* mysql, const char* q);
    __declspec(dllimport) MYSQL_RES* mysql_store_result(MYSQL* mysql);
    __declspec(dllimport) MYSQL_ROW mysql_fetch_row(MYSQL_RES* result);
    __declspec(dllimport) unsigned int mysql_num_rows(MYSQL_RES* result);
    __declspec(dllimport) void mysql_free_result(MYSQL_RES* result);
    __declspec(dllimport) const char* mysql_error(MYSQL* mysql);
}

int main() {
    // Try different MySQL paths
    const char* dllPaths[] = {
        "C:\\Program Files\\MySQL\\MySQL Server 8.0\\lib\\libmysql.dll",
        "C:\\xampp\\mysql\\bin\\libmysql.dll",
        "C:\\wamp64\\bin\\mysql\\mysql8.0.31\\lib\\libmysql.dll",
        NULL
    };

    HMODULE hDll = NULL;
    for (int i = 0; dllPaths[i] != NULL; i++) {
        hDll = LoadLibraryA(dllPaths[i]);
        if (hDll) {
            std::cout << "Loaded MySQL DLL from: " << dllPaths[i] << std::endl;
            break;
        }
    }

    if (!hDll) {
        std::cerr << "ERROR: Cannot load libmysql.dll" << std::endl;
        std::cerr << "Please ensure MySQL is installed" << std::endl;
        return 1;
    }

    // Load function pointers
    auto p_mysql_init = (decltype(&mysql_init))GetProcAddress(hDll, "mysql_init");
    auto p_mysql_real_connect = (decltype(&mysql_real_connect))GetProcAddress(hDll, "mysql_real_connect");
    auto p_mysql_close = (decltype(&mysql_close))GetProcAddress(hDll, "mysql_close");
    auto p_mysql_query = (decltype(&mysql_query))GetProcAddress(hDll, "mysql_query");
    auto p_mysql_store_result = (decltype(&mysql_store_result))GetProcAddress(hDll, "mysql_store_result");
    auto p_mysql_fetch_row = (decltype(&mysql_fetch_row))GetProcAddress(hDll, "mysql_fetch_row");
    auto p_mysql_num_rows = (decltype(&mysql_num_rows))GetProcAddress(hDll, "mysql_num_rows");
    auto p_mysql_free_result = (decltype(&mysql_free_result))GetProcAddress(hDll, "mysql_free_result");
    auto p_mysql_error = (decltype(&mysql_error))GetProcAddress(hDll, "mysql_error");

    if (!p_mysql_init || !p_mysql_real_connect || !p_mysql_query) {
        std::cerr << "ERROR: Cannot load MySQL functions" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    MYSQL* conn = p_mysql_init(NULL);

    std::cout << "\nConnecting to MySQL (127.0.0.1:3306, user=root, db=papercrawler_db)..." << std::endl;
    if (!p_mysql_real_connect(conn, "127.0.0.1", "root", "123456", "papercrawler_db", 3306, NULL, 0)) {
        std::cerr << "ERROR: Connection failed: " << p_mysql_error(conn) << std::endl;
        p_mysql_close(conn);
        FreeLibrary(hDll);
        return 1;
    }

    std::cout << "OK - Connected successfully!\n" << std::endl;

    // Check users table
    std::cout << "Checking for 'users' table..." << std::endl;
    if (p_mysql_query(conn, "SHOW TABLES LIKE 'users'")) {
        std::cerr << "Query failed: " << p_mysql_error(conn) << std::endl;
    } else {
        MYSQL_RES* result = p_mysql_store_result(conn);
        if (result && p_mysql_num_rows(result) > 0) {
            std::cout << "OK - 'users' table EXISTS!" << std::endl;

            // Count users
            p_mysql_query(conn, "SELECT COUNT(*) as count FROM users");
            MYSQL_RES* countResult = p_mysql_store_result(conn);
            if (countResult) {
                MYSQL_ROW row = p_mysql_fetch_row(countResult);
                if (row && row[0]) {
                    std::cout << "Total users: " << row[0] << std::endl;
                }
                p_mysql_free_result(countResult);
            }

            // List users
            p_mysql_query(conn, "SELECT id, email, name FROM users LIMIT 5");
            MYSQL_RES* usersResult = p_mysql_store_result(conn);
            if (usersResult && p_mysql_num_rows(usersResult) > 0) {
                std::cout << "\nExisting users:" << std::endl;
                MYSQL_ROW row;
                while ((row = p_mysql_fetch_row(usersResult))) {
                    std::cout << "  ID: " << (row[0] ? row[0] : "NULL")
                              << ", Email: " << (row[1] ? row[1] : "NULL")
                              << ", Name: " << (row[2] ? row[2] : "NULL") << std::endl;
                }
                p_mysql_free_result(usersResult);
            }
        } else {
            std::cout << "NOT FOUND - 'users' table does NOT exist!" << std::endl;
            std::cout << "\nThis is why registration is failing!" << std::endl;
            std::cout << "\nPlease run this SQL in MySQL:" << std::endl;
            std::cout << "====================================" << std::endl;
            std::cout << "CREATE DATABASE IF NOT EXISTS papercrawler_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" << std::endl;
            std::cout << "USE papercrawler_db;" << std::endl;
            std::cout << "CREATE TABLE users (" << std::endl;
            std::cout << "    id INT PRIMARY KEY AUTO_INCREMENT," << std::endl;
            std::cout << "    email VARCHAR(255) UNIQUE NOT NULL," << std::endl;
            std::cout << "    password_hash VARCHAR(255) NOT NULL," << std::endl;
            std::cout << "    name VARCHAR(100)," << std::endl;
            std::cout << "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP," << std::endl;
            std::cout << "    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP," << std::endl;
            std::cout << "    INDEX idx_email (email)" << std::endl;
            std::cout << ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;" << std::endl;
            std::cout << "====================================" << std::endl;
        }
        if (result) p_mysql_free_result(result);
    }

    p_mysql_close(conn);
    FreeLibrary(hDll);

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}
