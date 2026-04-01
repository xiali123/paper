#include <iostream>
#include "data/DatabaseModule.hpp"

using namespace PaperCrawler;

int main() {
    std::cout << "=== PaperCrawler 数据库连接测试 ===" << std::endl;

    // 从环境变量读取配置
    std::string host = "localhost";
    int port = 3306;
    std::string user = "root";
    std::string password;  // 从环境变量读取
    std::string database = "papercrawler";
    size_t poolSize = 5;

    // 获取环境变量
    const char* env_password = std::getenv("DB_PASSWORD");
    if (env_password) {
        password = env_password;
        std::cout << "从环境变量读取到DB_PASSWORD" << std::endl;
    } else {
        std::cout << "请输入MySQL密码: ";
        std::getline(std::cin, password);
    }

    const char* env_host = std::getenv("DB_HOST");
    if (env_host) host = env_host;

    const char* env_user = std::getenv("DB_USER");
    if (env_user) user = env_user;

    const char* env_db = std::getenv("DB_NAME");
    if (env_db) database = env_db;

    std::cout << "\n连接配置:" << std::endl;
    std::cout << "  Host: " << host << ":" << port << std::endl;
    std::cout << "  User: " << user << std::endl;
    std::cout << "  Database: " << database << std::endl;

    // 创建DatabaseModule
    DatabaseModule dbModule;

    // 配置连接池
    DatabaseConfig config;
    config.host = host;
    config.port = port;
    config.username = user;
    config.password = password;
    config.database = database;
    config.poolSize = poolSize;
    config.maxPoolSize = poolSize * 2;
    config.connectTimeoutSeconds = 10;
    config.queryTimeoutSeconds = 30;
    config.autoReconnect = true;

    dbModule.setConfig(config);

    // 初始化连接池
    std::cout << "\n正在初始化连接池..." << std::endl;
    if (!dbModule.initialize()) {
        std::cerr << "❌ 连接池初始化失败！" << std::endl;
        return 1;
    }

    // 启动模块
    std::cout << "正在启动模块..." << std::endl;
    if (!dbModule.start()) {
        std::cerr << "❌ 模块启动失败！" << std::endl;
        return 1;
    }

    // 测试连接
    std::cout << "\n正在测试数据库连接..." << std::endl;
    if (!dbModule.testConnection()) {
        std::cerr << "❌ 数据库连接测试失败！" << std::endl;
        dbModule.stop();
        dbModule.cleanup();
        return 1;
    }

    std::cout << "\n✅ 所有测试通过！" << std::endl;

    // 清理
    dbModule.stop();
    dbModule.cleanup();

    return 0;
}
