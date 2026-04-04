#include <iostream>
#include <mysql.h>
#include <string>

using namespace std;

int main() {
    MYSQL* conn = mysql_init(NULL);

    // Connect to MySQL
    cout << "Connecting to MySQL..." << endl;
    if (!mysql_real_connect(conn, "127.0.0.1", "root", "123456",
                               "papercrawler_db", 3306, NULL, CLIENT_MULTI_STATEMENTS)) {
        cerr << "Connection failed: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return 1;
    }
    cout << "Connected successfully!" << endl;

    // Check if users table exists
    cout << "\nChecking for 'users' table..." << endl;
    if (mysql_query(conn, "SHOW TABLES LIKE 'users'")) {
        cerr << "Query failed: " << mysql_error(conn) << endl;
    } else {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result && mysql_num_rows(result) > 0) {
            cout << "OK - 'users' table exists!" << endl;
        } else {
            cout << "'users' table does NOT exist. Creating..." << endl;

            // Create users table
            const char* createSQL =
                "CREATE TABLE users ("
                "id INT PRIMARY KEY AUTO_INCREMENT,"
                "email VARCHAR(255) UNIQUE NOT NULL,"
                "password_hash VARCHAR(255) NOT NULL,"
                "name VARCHAR(100),"
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
                "INDEX idx_email (email)"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

            if (mysql_query(conn, createSQL)) {
                cerr << "Failed to create users table: " << mysql_error(conn) << endl;
            } else {
                cout << "OK - 'users' table created successfully!" << endl;
            }
        }
        mysql_free_result(result);
    }

    // Check if user_sessions table exists
    cout << "\nChecking for 'user_sessions' table..." << endl;
    if (mysql_query(conn, "SHOW TABLES LIKE 'user_sessions'")) {
        cerr << "Query failed: " << mysql_error(conn) << endl;
    } else {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result && mysql_num_rows(result) > 0) {
            cout << "OK - 'user_sessions' table exists!" << endl;
        } else {
            cout << "'user_sessions' table does NOT exist. Creating..." << endl;

            // Create user_sessions table
            const char* createSQL =
                "CREATE TABLE user_sessions ("
                "id INT PRIMARY KEY AUTO_INCREMENT,"
                "user_id INT NOT NULL,"
                "token VARCHAR(512) NOT NULL,"
                "device_info VARCHAR(255),"
                "ip_address VARCHAR(45),"
                "expires_at TIMESTAMP NOT NULL,"
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
                "INDEX idx_user_id (user_id),"
                "INDEX idx_token (token(255))"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

            if (mysql_query(conn, createSQL)) {
                cerr << "Failed to create user_sessions table: " << mysql_error(conn) << endl;
            } else {
                cout << "OK - 'user_sessions' table created successfully!" << endl;
            }
        }
        mysql_free_result(result);
    }

    // List all tables
    cout << "\nAll user tables in database:" << endl;
    if (mysql_query(conn, "SHOW TABLES LIKE 'user%'")) {
        cerr << "Query failed: " << mysql_error(conn) << endl;
    } else {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                cout << "  - " << row[0] << endl;
            }
            mysql_free_result(result);
        }
    }

    cout << "\n=======================================================" << endl;
    cout << "Database setup completed!" << endl;
    cout << "Now test registration:" << endl;
    cout << "  curl -X POST http://localhost:8080/api/auth/register \\" << endl;
    cout << "    -H 'Content-Type: application/json' \\" << endl;
    cout << "    -d '{\"email\":\"test@example.com\",\"password\":\"test123456\",\"name\":\"Test\"}'" << endl;
    cout << "=======================================================" << endl;

    mysql_close(conn);
    return 0;
}
