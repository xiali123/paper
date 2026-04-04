#include <iostream>
#include <fstream>
#include <mysql.h>

int main() {
    MYSQL* conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "127.0.0.1", "root", "123456", "papercrawler_db", 3306, NULL, 0)) {
        std::cerr << "Connection failed: " << mysql_error(conn) << std::endl;
        return 1;
    }
    
    std::cout << "\n=== Users in Database ===" << std::endl;
    
    if (mysql_query(conn, "SELECT id, email, name, LEFT(password_hash, 20) as pwd_hash FROM users")) {
        std::cerr << "Query failed: " << mysql_error(conn) << std::endl;
    } else {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result) {
            std::cout << "Total users: " << mysql_num_rows(result) << std::endl << std::endl;
            
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                std::cout << "ID: " << (row[0] ? row[0] : "NULL")
                          << ", Email: " << (row[1] ? row[1] : "NULL")
                          << ", Name: " << (row[2] ? row[2] : "NULL")
                          << ", PwdHash: " << (row[3] ? row[3] : "NULL") << std::endl;
            }
            mysql_free_result(result);
        }
    }
    
    mysql_close(conn);
    return 0;
}
