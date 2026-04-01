#include <iostream>
#include <vector>
#include <string>
#include "data/MySqlConnection.hpp"

using namespace PaperCrawler;

int main() {
    std::cout << "=== MySQL连接测试 ===" << std::endl;
    std::cout << "请输入MySQL连接信息：" << std::endl;

    std::string host = "localhost";
    int port = 3306;
    std::string user = "root";
    std::string password;
    std::string database = "papercrawler";

    std::cout << "Host [" << host << "]: ";
    std::getline(std::cin, host);
    if (host.empty()) host = "localhost";

    std::cout << "Port [" << port << "]: ";
    std::string portStr;
    std::getline(std::cin, portStr);
    if (!portStr.empty()) port = std::stoi(portStr);

    std::cout << "User [" << user << "]: ";
    std::getline(std::cin, user);
    if (user.empty()) user = "root";

    std::cout << "Password: ";
    std::getline(std::cin, password);

    std::cout << "Database [" << database << "]: ";
    std::getline(std::cin, database);
    if (database.empty()) database = "papercrawler";

    std::cout << "\n正在连接到 " << user << "@" << host << ":" << port << "/" << database << std::endl;

    try {
        // 创建MySQL连接
        MySqlConnection conn(host, port, user, password, database);

        if (!conn.isConnected()) {
            std::cerr << "❌ 连接失败！" << std::endl;
            return 1;
        }

        std::cout << "✅ 连接成功！" << std::endl;

        // 测试查询1：查看所有表
        std::cout << "\n=== 查询所有表 ===" << std::endl;
        auto tables = conn.query("SHOW TABLES");

        if (tables.empty()) {
            std::cout << "⚠️  数据库中没有表" << std::endl;
        } else {
            std::cout << "找到 " << tables.size() << " 个表：" << std::endl;
            for (const auto& table : tables) {
                std::string tableName = table.begin()->second;
                std::cout << "  - " << tableName << std::endl;
            }
        }

        // 测试查询2：查看users表
        std::cout << "\n=== 查询 users 表 ===" << std::endl;
        try {
            auto users = conn.query("SELECT id, username, email, role, active FROM users LIMIT 10");
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
            std::cout << "⚠️  users表不存在或查询失败: " << e.what() << std::endl;
        }

        // 测试查询3：查看papers表
        std::cout << "\n=== 查询 papers 表 ===" << std::endl;
        try {
            auto papers = conn.query("SELECT id, title, authors, year, citation_count FROM papers LIMIT 10");
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
            std::cout << "⚠️  papers表不存在或查询失败: " << e.what() << std::endl;
        }

        // 测试查询4：数据库统计
        std::cout << "\n=== 数据库统计 ===" << std::endl;
        try {
            auto stats = conn.query("SELECT table_name, table_rows "
                                   "FROM information_schema.tables "
                                   "WHERE table_schema = '" + database + "' "
                                   "ORDER BY table_name");

            if (!stats.empty()) {
                std::cout << "表名和行数：" << std::endl;
                for (const auto& stat : stats) {
                    std::cout << "  " << stat.at("table_name") << ": "
                              << stat.at("table_rows") << " 行" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "⚠️  统计查询失败: " << e.what() << std::endl;
        }

        std::cout << "\n✅ 测试完成！数据库连接正常。" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ 错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
