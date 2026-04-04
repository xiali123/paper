#include <iostream>
#include <vector>
#include <string>
#include "data/MySqlConnection.hpp"

using namespace PaperCrawler;

int main() {
    std::cout << "=== 快速MySQL连接测试 ===" << std::endl;

    // 硬编码连接信息
    std::string host = "localhost";
    int port = 3306;
    std::string user = "root";
    std::string password = "123456";  // 用户提供的密码
    std::string database = "papercrawler";

    std::cout << "连接配置:" << std::endl;
    std::cout << "  Host: " << host << ":" << port << std::endl;
    std::cout << "  User: " << user << std::endl;
    std::cout << "  Database: " << database << std::endl;
    std::cout << "  Password: *******" << std::endl;

    try {
        // 创建MySQL连接
        std::cout << "\n正在连接MySQL..." << std::endl;
        MySqlConnection conn(host, port, user, password, database);

        if (!conn.isConnected()) {
            std::cerr << "❌ 连接失败！" << std::endl;
            std::cerr << "请检查：" << std::endl;
            std::cerr << "  1. MySQL服务是否启动" << std::endl;
            std::cerr << "  2. 用户名和密码是否正确" << std::endl;
            std::cerr << "  3. 数据库 'papercrawler' 是否存在" << std::endl;
            return 1;
        }

        std::cout << "✅ 连接成功！" << std::endl;

        // 测试查询1：查看所有表
        std::cout << "\n=== 查询所有表 ===" << std::endl;
        auto tables = conn.query("SHOW TABLES");

        if (tables.empty()) {
            std::cout << "⚠️  数据库中没有表" << std::endl;
            std::cout << "请运行: mysql -u root -p123456 papercrawler < database/complete-schema-mysql.sql" << std::endl;
        } else {
            std::cout << "找到 " << tables.size() << " 个表：" << std::endl;
            for (const auto& table : tables) {
                std::string tableName = table.begin()->second;
                std::cout << "  ✓ " << tableName << std::endl;
            }
        }

        // 测试查询2：查看users表结构和数据
        std::cout << "\n=== 查询 users 表 ===" << std::endl;
        try {
            auto users = conn.query("SELECT id, username, email, role, active FROM users LIMIT 5");

            if (users.empty()) {
                std::cout << "⚠️  users表为空" << std::endl;
            } else {
                std::cout << "找到 " << users.size() << " 个用户：" << std::endl;
                for (const auto& user : users) {
                    std::cout << "  ID: " << user.at("id")
                              << ", Username: " << user.at("username")
                              << ", Email: " << user.at("email")
                              << ", Role: " << user.at("role")
                              << ", Active: " << user.at("active") << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "⚠️  users表查询失败: " << e.what() << std::endl;
        }

        // 测试查询3：查看papers表
        std::cout << "\n=== 查询 papers 表 ===" << std::endl;
        try {
            auto papers = conn.query("SELECT id, title, authors, year, citation_count FROM papers LIMIT 5");

            if (papers.empty()) {
                std::cout << "⚠️  papers表为空" << std::endl;
            } else {
                std::cout << "找到 " << papers.size() << " 篇论文：" << std::endl;
                for (const auto& paper : papers) {
                    std::cout << "  ID: " << paper.at("id")
                              << ", Title: " << paper.at("title")
                              << ", Authors: " << paper.at("authors")
                              << ", Year: " << paper.at("year")
                              << ", Citations: " << paper.at("citation_count") << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "⚠️  papers表查询失败: " << e.what() << std::endl;
        }

        // 测试查询4：数据库统计
        std::cout << "\n=== 数据库统计 ===" << std::endl;
        try {
            auto stats = conn.query(
                "SELECT table_name, table_rows "
                "FROM information_schema.tables "
                "WHERE table_schema = '" + database + "' "
                "ORDER BY table_rows DESC"
            );

            if (!stats.empty()) {
                std::cout << "表名和行数（按行数降序）：" << std::endl;
                for (const auto& stat : stats) {
                    std::string table_name = stat.at("table_name");
                    std::string row_count = stat.at("table_rows");
                    std::cout << "  " << table_name << ": " << row_count << " 行" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "⚠️  统计查询失败: " << e.what() << std::endl;
        }

        // 测试查询5：测试插入一条数据
        std::cout << "\n=== 测试插入功能 ===" << std::endl;
        try {
            // 先检查是否已有测试用户
            auto existing = conn.query("SELECT id FROM users WHERE username = 'test_user'");
            if (existing.empty()) {
                std::cout << "插入测试用户..." << std::endl;
                bool inserted = conn.execute(
                    "INSERT INTO users (username, email, password_hash, full_name, role, active, created_at) "
                    "VALUES ('test_user', 'test@example.com', 'hashed_password', 'Test User', 'user', 1, NOW())"
                );

                if (inserted) {
                    std::cout << "✅ 测试用户插入成功" << std::endl;
                    uint64_t insertId = conn.getLastInsertId();
                    std::cout << "  新用户ID: " << insertId << std::endl;
                    std::cout << "  受影响行数: " << conn.getAffectedRows() << std::endl;

                    // 查询刚插入的用户
                    auto newUsers = conn.query("SELECT * FROM users WHERE id = " + std::to_string(insertId));
                    if (!newUsers.empty()) {
                        std::cout << "  验证查询成功，找到新用户: " << newUsers[0]["username"] << std::endl;
                    }
                } else {
                    std::cout << "❌ 插入失败" << std::endl;
                }
            } else {
                std::cout << "ℹ️  测试用户已存在（ID: " << existing[0]["id"] << "）" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "⚠️  插入测试失败: " << e.what() << std::endl;
        }

        std::cout << "\n✅ 所有测试完成！数据库连接正常。" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ 错误: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n按任意键退出..." << std::endl;
    std::cin.get();
    return 0;
}
