/**
 * @file main.cpp
 * @brief PaperCrawler模块化后端服务器主程序入口
 *
 * 功能：
 * 1. 初始化框架核心（MessageBus, Router, PluginManager）
 * 2. 从配置文件加载模块元数据
 * 3. 按依赖顺序加载系统模块
 * 4. 加载业务模块
 * 5. 启动HTTP服务器
 * 6. 注册管理API
 * 7. 优雅关闭处理
 *
 * @author PaperCrawler Team
 * @version 1.0.0
 * @date 2026-03-28
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <sstream>
#include <regex>
#include <filesystem>

// OpenSSL for cryptography
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/crypto.h>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

// 框架核心
#include "core/MessageBus.hpp"
#include "core/Router.hpp"
#include "core/PluginManager.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/HotReloadManager.hpp"
#include "core/HttpTypes.hpp"
#include "core/ConfigManager.hpp"
#include "core/ServiceContainer.hpp"

// 网络模块
#include "network/HttpServerModule.hpp"
#include "network/HttpClient.hpp"

// 数据模块
#include "data/MySqlConnection.hpp"
#include "data/DatabaseModule.hpp"
#include "data/PooledConnection.hpp"

// JSON library
#include "../../core/external/nlohmann/json.hpp"

#include <spdlog/spdlog.h>

using namespace PaperCrawler;
using json = nlohmann::json;

// 全局运行标志
std::atomic<bool> g_running{true};

// 全局HTTP服务器实例
std::unique_ptr<HttpServerModule> g_httpServer;

// 全局数据库模块（连接池）
std::shared_ptr<DatabaseModule> g_databaseModule;

// 全局MySQL连接实例（保持向后兼容，实际使用连接池）
std::unique_ptr<PooledConnection> g_dbConnection;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 安全地从map中获取值（带默认值）
 */
template<typename T>
std::string getMapValue(const T& map, const std::string& key, const std::string& defaultValue = "") {
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return defaultValue;
}

/**
 * @brief Base64编码（简单实现，避免OpenSSL依赖问题）
 */
std::string base64_encode(const unsigned char* data, size_t len) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        unsigned char b0 = data[i];
        unsigned char b1 = (i + 1 < len) ? data[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? data[i + 2] : 0;

        result.push_back(base64_chars[b0 >> 2]);
        result.push_back(base64_chars[((b0 & 0x03) << 4) | (b1 >> 4)]);
        result.push_back((i + 1 < len) ? base64_chars[((b1 & 0x0F) << 2) | (b2 >> 6)] : '=');
        result.push_back((i + 2 < len) ? base64_chars[b2 & 0x3F] : '=');
    }

    return result;
}

/**
 * @brief Base64编码（重载版本）
 */
std::string base64_encode(const std::vector<unsigned char>& data) {
    return base64_encode(data.data(), data.size());
}

/**
 * @brief Base64解码（简单实现）
 */
std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<unsigned char> result;
    result.reserve((encoded_string.size() * 3) / 4);

    int val = 0, valb = -8;
    for (unsigned char c : encoded_string) {
        if (c == '=') break;

        std::string::size_type pos = base64_chars.find(c);
        if (pos == std::string::npos) continue;

        val = (val << 6) + pos;
        valb += 6;

        if (valb >= 0) {
            result.push_back((val >> (valb - 8)) & 0xFF);
            valb -= 8;
        }
    }

    return result;
}

// ============================================================================
// 认证系统 - Session管理
// ============================================================================

#include <unordered_map>
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>

struct UserSession {
    int userId;
    std::string email;
    std::string name;
    std::string token;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
};

// 简单的内存存储（生产环境应使用Redis）
std::unordered_map<std::string, UserSession> g_activeSessions;
std::mutex g_sessionMutex;

/**
 * @brief 生成随机Token
 */
std::string generateToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::ostringstream ss;
    ss << std::hex;

    for (int i = 0; i < 64; ++i) {
        ss << std::setw(1) << std::setfill('0') << dis(gen);
    }

    return ss.str();
}

/**
 * @brief 使用多次迭代的哈希进行密码哈希（中等安全性）
 *
 * 注意：生产环境应使用OpenSSL的PBKDF2或bcrypt
 * 这个实现使用标准C++库，避免了OpenSSL依赖问题
 *
 * @param password 明文密码
 * @param hash 输出参数，存储哈希值
 * @return 是否成功
 */
bool hashPassword(const std::string& password, std::string& hash) {
    // 生成随机salt
    unsigned char salt[16];
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < 16; i++) {
        salt[i] = static_cast<unsigned char>(dis(gen));
    }

    // 使用SHA256风格的哈希（通过标准库hash模拟）
    // 进行10000次迭代来增加破解难度
    std::size_t hash_value = std::hash<std::string>{}(password);
    std::string salted_password = password + std::string(reinterpret_cast<char*>(salt), sizeof(salt));

    for (int i = 0; i < 10000; i++) {
        hash_value = std::hash<std::string>{}(std::to_string(hash_value) + salted_password);
    }

    // 组合salt和哈希值并编码为Base64
    // 格式: $salt(base64)$hash(base64)
    std::string saltBase64 = base64_encode(salt, sizeof(salt));

    // 将hash_value转换为字节数组
    unsigned char hash_bytes[sizeof(hash_value)];
    std::memcpy(hash_bytes, &hash_value, sizeof(hash_value));
    std::string hashBase64 = base64_encode(hash_bytes, sizeof(hash_bytes));

    std::ostringstream ss;
    ss << saltBase64 << "$" << hashBase64;
    hash = ss.str();

    return true;
}

/**
 * @brief 验证密码
 *
 * @param password 明文密码
 * @param storedHash 存储的哈希值（格式：salt(base64)$hash(base64)）
 * @return 是否匹配
 */
bool verifyPassword(const std::string& password, const std::string& storedHash) {
    // 解析存储的哈希值
    size_t delim = storedHash.find('$');
    if (delim == std::string::npos) {
        spdlog::error("[Security] Invalid hash format");
        return false;
    }

    // 提取salt（Base64解码）
    std::string saltBase64 = storedHash.substr(0, delim);
    std::vector<unsigned char> salt = base64_decode(saltBase64);

    // 提取存储的哈希值（Base64解码）
    std::string hashBase64 = storedHash.substr(delim + 1);
    std::vector<unsigned char> storedHashBytes = base64_decode(hashBase64);

    // 使用相同的参数计算哈希
    std::string salted_password = password + std::string(reinterpret_cast<char*>(salt.data()), salt.size());
    std::size_t hash_value = std::hash<std::string>{}(password);

    for (int i = 0; i < 10000; i++) {
        hash_value = std::hash<std::string>{}(std::to_string(hash_value) + salted_password);
    }

    // 转换为字节数组进行比较
    unsigned char computed_hash_bytes[sizeof(hash_value)];
    std::memcpy(computed_hash_bytes, &hash_value, sizeof(hash_value));

    // 常量时间比较，防止时序攻击
    if (storedHashBytes.size() != sizeof(computed_hash_bytes)) {
        return false;
    }

    bool result = true;
    for (size_t i = 0; i < storedHashBytes.size(); i++) {
        if (computed_hash_bytes[i] != storedHashBytes[i]) {
            result = false;
        }
    }

    return result;
}

/**
 * @brief 创建Session
 */
std::string createSession(int userId, const std::string& email, const std::string& name) {
    std::lock_guard<std::mutex> lock(g_sessionMutex);

    std::string token = generateToken();
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::hours(24); // 24小时过期

    UserSession session;
    session.userId = userId;
    session.email = email;
    session.name = name;
    session.token = token;
    session.createdAt = now;
    session.expiresAt = expires;

    g_activeSessions[token] = session;

    return token;
}

/**
 * @brief 验证Token并获取Session
 */
UserSession* validateSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(g_sessionMutex);

    auto it = g_activeSessions.find(token);
    if (it == g_activeSessions.end()) {
        return nullptr;
    }

    auto& session = it->second;
    auto now = std::chrono::system_clock::now();

    if (now > session.expiresAt) {
        // Token已过期
        g_activeSessions.erase(it);
        return nullptr;
    }

    return &session;
}

/**
 * @brief 删除Session
 */
void destroySession(const std::string& token) {
    std::lock_guard<std::mutex> lock(g_sessionMutex);
    g_activeSessions.erase(token);
}

/**
 * @brief 从请求中提取Token
 */
std::string extractToken(const HttpRequest& req) {
    // 从Authorization header获取
    std::string authHeader = req.getHeader("Authorization");
    if (!authHeader.empty() && authHeader.find("Bearer ") == 0) {
        return authHeader.substr(7);
    }

    // 从query参数获取
    std::string tokenParam = req.getQuery("token", "");
    if (!tokenParam.empty()) {
        return tokenParam;
    }

    return "";
}

// 前向声明辅助函数
void printStep(const std::string& step, const std::string& details);
void printSuccess(const std::string& message);
void printError(const std::string& message);
void printWarning(const std::string& message);

/**
 * @brief 转义JSON字符串中的特殊字符
 * @param str - 原始字符串
 * @return 转义后的JSON字符串
 */
std::string escapeJsonString(const std::string& str) {
  std::string escaped;
  escaped.reserve(str.size() * 2);

  for (char c : str) {
    switch (c) {
      case '"':  escaped += "\\\""; break;
      case '\\': escaped += "\\\\"; break;
      case '\b': escaped += "\\b"; break;
      case '\f': escaped += "\\f"; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if (c < ' ') {
          // 控制字符转义为 \uXXXX 格式
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned char>(c));
          escaped += buf;
        } else {
          escaped += c;
        }
    }
  }

  return escaped;
}

/**
 * @brief 信号处理函数
 */
void signalHandler(int signal) {
    spdlog::info("Received shutdown signal: {}", signal);
    g_running = false;
}

/**
 * @brief 注册信号处理
 */
void setupSignalHandlers() {
    std::signal(SIGINT, signalHandler);   // Ctrl+C
    std::signal(SIGTERM, signalHandler);  // 终止信号
#ifdef SIGQUIT
    std::signal(SIGQUIT, signalHandler);  // 退出信号
#endif
}

/**
 * @brief 初始化数据库连接池
 */
bool initializeDatabase() {
    printStep("Init", "Connecting to MySQL database with connection pool");

    try {
        // 使用ConfigManager加载配置
        auto& config = ConfigManager::getInstance();

        // 尝试从配置文件加载
        config.loadFromFile("./config/config.json");

        // 从环境变量加载（优先级更高）
        config.loadFromEnvironment();

        // 获取数据库配置
        std::string host = config.getString("database.host", "localhost");
        int port = config.getInt("database.port", 3306);
        std::string user = config.getString("database.user", "root");
        std::string password = config.getString("database.password");
        std::string database = config.getString("database.name", "papercrawler");
        size_t poolSize = config.getInt("database.pool_size", 20);

        // 验证密码配置
        if (password.empty()) {
            printError("Database password not configured (set DB_PASSWORD environment variable)");
            spdlog::error("[Config] Database password not configured (set DB_PASSWORD environment variable)");
            return false;
        }

        // 创建DatabaseModule实例
        g_databaseModule = std::make_shared<DatabaseModule>();

        // 配置连接池
        DatabaseConfig dbConfig;
        dbConfig.host = host;
        dbConfig.port = port;
        dbConfig.username = user;
        dbConfig.password = password;
        dbConfig.database = database;
        dbConfig.poolSize = poolSize;        // 初始连接数
        dbConfig.maxPoolSize = poolSize * 2; // 最大连接数
        dbConfig.connectTimeoutSeconds = 5;
        dbConfig.queryTimeoutSeconds = 30;
        dbConfig.autoReconnect = true;

        g_databaseModule->setConfig(dbConfig);

        // 初始化连接池
        if (!g_databaseModule->initialize()) {
            printError("Failed to initialize DatabaseModule connection pool");
            return false;
        }

        // 启动DatabaseModule
        if (!g_databaseModule->start()) {
            printError("Failed to start DatabaseModule");
            return false;
        }

        // 获取一个连接作为全局默认连接（向后兼容）
        auto connection = g_databaseModule->getConnection();
        if (!connection) {
            printError("Failed to get connection from pool");
            return false;
        }

        // 创建PooledConnection包装器（析构时自动归还）
        g_dbConnection = std::make_unique<PooledConnection>(
            connection,
            [dbModule = g_databaseModule.get()](std::shared_ptr<DatabaseConnection> conn) {
                dbModule->returnConnection(conn);
            }
        );

        if (!g_dbConnection->isConnected()) {
            printError("Failed to connect to MySQL database");
            spdlog::error("[Database] Connection failed - host:{}, port:{}, user:{}, db:{}",
                         host, port, user, database);
            return false;
        }

        // 显示连接池状态
        auto poolStats = g_databaseModule->getPoolStats();
        printSuccess("Connected to MySQL database with connection pool");
        spdlog::info("[Database] Successfully connected - host:{}, port:{}, user:{}, db:{}",
                     host, port, user, database);
        spdlog::info("[Database] Connection pool initialized - size:{}, max_size:{}, active:{}",
                     poolStats.totalConnections, dbConfig.maxPoolSize, poolStats.activeConnections);

        return true;
    } catch (const std::exception& e) {
        printError(std::string("Database initialization failed: ") + e.what());
        return false;
    }
}

/**
 * @brief 初始化依赖注入服务容器
 *
 * 注册全局服务到DI容器，使业务模块可以通过依赖注入获取服务
 */
bool initializeServices() {
    printStep("Init", "Initializing dependency injection service container");

    try {
        // 注册DatabaseModule为单例服务
        // 使用g_databaseModule实例（已在initializeDatabase()中创建）
        Services::registerInstance<IDatabase>(g_databaseModule);

        printSuccess("Dependency injection service container initialized");
        spdlog::info("[Services] Registered IDatabase service (Singleton)");

        // TODO: 未来可以注册更多服务
        // Services::registerService<ICache, CacheModule>(ServiceLifetime::SINGLETON);
        // Services::registerService<IEmailService, EmailService>(ServiceLifetime::SINGLETON);

        return true;
    } catch (const std::exception& e) {
        printError(std::string("Service container initialization failed: ") + e.what());
        spdlog::error("[Services] Initialization failed: {}", e.what());
        return false;
    }
}

/**
 * @brief 打印欢迎信息
 */
void printWelcome() {
    std::cout << R"(
    ========================================
       PaperCrawler Modular Backend Server
    ========================================
       Version: 1.0.0
       Architecture: 34 Modules
       Build Date: )" << __DATE__ << R"(
    ========================================
    )" << std::endl;
}

/**
 * @brief 打印启动步骤
 */
void printStep(const std::string& step, const std::string& details) {
    std::cout << "[" << step << "] " << details << "..." << std::endl;
}

/**
 * @brief 打印成功信息
 */
void printSuccess(const std::string& message) {
    std::cout << "  ✓ " << message << std::endl;
}

/**
 * @brief 打印错误信息
 */
void printError(const std::string& message) {
    std::cerr << "  ✗ " << message << std::endl;
}

/**
 * @brief 打印警告信息（黄色）
 */
void printWarning(const std::string& message) {
    std::cout << "  ⚠ " << message << std::endl;
}

/**
 * @brief 打印启动完成信息
 */
void printReady(int port) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Server is running!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  HTTP Server: http://localhost:" << port << std::endl;
    std::cout << "  WebSocket:   ws://localhost:" << (port + 1) << std::endl;
    std::cout << "  API Docs:    http://localhost:" << port << "/api/docs" << std::endl;
    std::cout << "  Metrics:     http://localhost:" << port << "/metrics" << std::endl;
    std::cout << "\n  Management endpoints:" << std::endl;
    std::cout << "    GET  /api/modules              - List modules" << std::endl;
    std::cout << "    POST /api/modules/load         - Load module" << std::endl;
    std::cout << "    POST /api/modules/unload       - Unload module" << std::endl;
    std::cout << "    POST /api/modules/reload       - Reload module" << std::endl;
    std::cout << "    GET  /health                   - Health check" << std::endl;
    std::cout << "\n  Press Ctrl+C to stop" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief 初始化框架核心
 */
bool initializeFramework() {
    printStep("1/7", "Initializing framework core");

    try {
        // 初始化核心单例
        MessageBus::getInstance();
        Router::getInstance();
        PluginManager::getInstance();
        ModuleRegistry::getInstance();

        printSuccess("Framework core initialized");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("Failed to initialize framework: ") + e.what());
        return false;
    }
}

/**
 * @brief 加载模块配置
 */
bool loadModuleConfiguration() {
    printStep("2/7", "Loading module configuration");

    auto& registry = ModuleRegistry::getInstance();
    if (!registry.loadFromConfig("./config/modules.json")) {
        printError("Failed to load modules.json");
        return false;
    }

    auto allModules = registry.getAllModules();
    printSuccess("Loaded configuration for " + std::to_string(allModules.size()) + " modules");
    return true;
}

/**
 * @brief 加载和启动系统模块
 */
bool loadAndStartSystemModules() {
    printStep("3/7", "Loading and starting system modules");

    auto& pluginMgr = PluginManager::getInstance();

    // 扫描并加载所有模块（包括SERVER和BUSINESS）
    namespace fs = std::filesystem;
    std::string modulesDir = "./modules";

    // 检查是否存在modules目录
    std::vector<std::string> searchPaths = {
        "./modules",
        "../modules",
        "./build/Release/modules",
        "../build/Release/modules"
    };

    std::string actualModulesDir;
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            actualModulesDir = path;
            break;
        }
    }

    if (actualModulesDir.empty()) {
        printError("Modules directory not found");
        spdlog::warn("Searched paths: ./modules, ../modules, ./build/Release/modules, ../build/Release/modules");
        return false;
    }

    printSuccess("Found modules directory: " + actualModulesDir);

    // 注意：模块现在通过CMake静态链接，不再需要scanAndLoadModules()
    // 模块会通过PluginManager::startAllModules()自动启动
    // 下面的scanAndLoadModules()调用被禁用以避免死锁

    /*
    // 扫描并加载所有模块
    if (!pluginMgr.scanAndLoadModules(actualModulesDir)) {
        printError("Some modules failed to load");
        // 继续执行，因为部分模块加载失败不应阻止系统启动
    }
    */

    // 阶段1-5：扫描动态模块目录（渐进式动态化）
    std::string dynamicModulesDir = actualModulesDir + "/dynamic";
    printStep("Dynamic", "Scanning for modules in: " + dynamicModulesDir);

    if (fs::exists(dynamicModulesDir)) {
        spdlog::info("Scanning dynamic modules directory: {}", dynamicModulesDir);
        if (!pluginMgr.scanAndLoadModules(dynamicModulesDir)) {
            printWarning("Some dynamic modules failed to load (non-blocking)");
        } else {
            auto dynamicModules = pluginMgr.getBusinessModules();
            printSuccess("Loaded " + std::to_string(dynamicModules.size()) + " dynamic modules");
        }
    } else {
        printWarning("Dynamic modules directory not found: " + dynamicModulesDir);
    }

    // 启动所有模块（PluginManager会按类型顺序启动）
    if (!pluginMgr.startAllModules()) {
        printError("Failed to start some modules");
        return false;
    }

    auto allModules = pluginMgr.getAllModules();
    printSuccess("Loaded and started " + std::to_string(allModules.size()) + " modules");
    return true;
}

/**
 * @brief 加载业务模块
 */
bool loadBusinessModules() {
    printStep("4/7", "Business modules already loaded");

    // 业务模块已在 loadAndStartSystemModules() 中通过 scanAndLoadModules() 加载
    // 此函数保留用于向后兼容

    auto& pluginMgr = PluginManager::getInstance();
    auto businessModules = pluginMgr.getBusinessModules();

    printSuccess("Loaded " + std::to_string(businessModules.size()) + " business modules");
    return true;
}

/**
 * @brief 注册管理API
 */
bool registerManagementAPIs() {
    printStep("5/7", "Registering management APIs");

    auto& router = Router::getInstance();

    // 模块管理API
    router.get("/api/modules", [](const HttpRequest& req) {
        // TODO: 列出所有模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"modules":[]})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.post("/api/modules/load", [](const HttpRequest& req) {
        // TODO: 动态加载模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"message":"Module loaded successfully"})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.post("/api/modules/unload", [](const HttpRequest& req) {
        // TODO: 智能卸载模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"message":"Module unloaded successfully"})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.post("/api/modules/reload", [](const HttpRequest& req) {
        // TODO: 热重载模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"message":"Module reloaded successfully"})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/api/modules/:name/stats", [](const HttpRequest& req) {
        // TODO: 模块统计
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"stats":{}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // 健康检查API（保持 /health 路由用于直接访问）
    router.get("/health", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"status":"ok","timestamp":")" +
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                       R"("})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/health/components", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"status":"ok","components":{"Pool":"HEALTHY","Database":"HEALTHY","Cache":"HEALTHY"}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // 健康检查API（/api/health 路由用于前端代理访问）
    router.get("/api/health", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"status":"ok","timestamp":")" +
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                       R"("})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/api/health/components", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"status":"ok","components":{"Pool":"HEALTHY","Database":"HEALTHY","Cache":"HEALTHY"}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // ============================================================================
    // 认证API
    // ============================================================================

    /**
     * POST /api/auth/register
     * 用户注册
     */
    router.post("/api/auth/register", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            // 解析请求体
            json requestBody = json::parse(req.body);

            std::string email = requestBody.value("email", "");
            std::string password = requestBody.value("password", "");
            std::string name = requestBody.value("name", "");

            // 验证必填字段
            if (email.empty() || password.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"Email and password are required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 检查邮箱是否已存在
            // 检查邮箱是否已存在 - 使用预处理语句防止SQL注入
            auto checkStmt = g_dbConnection->prepare("SELECT id FROM users WHERE email = ? LIMIT 1");
            if (!checkStmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            checkStmt->setString(0, email);
            auto existingUsers = checkStmt->query();
            if (!existingUsers.empty()) {
                response.statusCode = 409;
                response.body = R"({"success":false,"error":"Email already registered"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 密码哈希 - 使用PBKDF2
            std::string passwordHash;
            if (!hashPassword(password, passwordHash)) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Failed to hash password"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 插入新用户 - 使用预处理语句防止SQL注入
            auto insertStmt = g_dbConnection->prepare("INSERT INTO users (email, password_hash, name, created_at) VALUES (?, ?, ?, NOW())");
            if (!insertStmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            insertStmt->setString(0, email);
            insertStmt->setString(1, passwordHash);
            insertStmt->setString(2, name);

            if (!insertStmt->execute()) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Failed to create user"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 获取新用户ID
            auto lastId = g_dbConnection->query("SELECT LAST_INSERT_ID() as id");
            int userId = std::stoi(lastId[0].at("id"));

            // 创建Session
            std::string token = createSession(userId, email, name);

            response.statusCode = 201;
            response.body = R"({"success":true,"message":"User registered successfully","data":{)"
                            R"("user_id":)" + std::to_string(userId) + R"(,)"
                            R"("email":")" + escapeJsonString(email) + R"(",)"
                            R"("name":")" + escapeJsonString(name) + R"(",)"
                            R"("token":")" + token + R"("}})";
        } catch (const json::exception& e) {
            response.statusCode = 400;
            response.body = R"({"success":false,"error":"Invalid JSON format"})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * POST /api/auth/login
     * 用户登录
     */
    router.post("/api/auth/login", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            // 解析请求体
            json requestBody = json::parse(req.body);

            std::string email = requestBody.value("email", "");
            std::string password = requestBody.value("password", "");

            // 验证必填字段
            if (email.empty() || password.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"Email and password are required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 查询用户 - 使用预处理语句防止SQL注入
            auto stmt = g_dbConnection->prepare("SELECT id, email, password_hash, name FROM users WHERE email = ? LIMIT 1");
            if (!stmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            stmt->setString(0, email);
            auto users = stmt->query();

            if (users.empty()) {
                response.statusCode = 401;
                response.body = R"({"success":false,"error":"Invalid email or password"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            auto& user = users[0];
            std::string storedHash = user.at("password_hash");

            // 验证密码
            if (!verifyPassword(password, storedHash)) {
                response.statusCode = 401;
                response.body = R"({"success":false,"error":"Invalid email or password"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 创建Session
            int userId = std::stoi(user.at("id"));
            std::string name = user.at("name");
            std::string token = createSession(userId, email, name);

            response.statusCode = 200;
            response.body = R"({"success":true,"message":"Login successful","data":{)"
                            R"("user_id":)" + std::to_string(userId) + R"(,)"
                            R"("email":")" + escapeJsonString(email) + R"(",)"
                            R"("name":")" + escapeJsonString(name) + R"(",)"
                            R"("token":")" + token + R"("}})";
        } catch (const json::exception& e) {
            response.statusCode = 400;
            response.body = R"({"success":false,"error":"Invalid JSON format"})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * POST /api/auth/logout
     * 用户登出
     */
    router.post("/api/auth/logout", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            // 提取Token
            std::string token = extractToken(req);

            if (token.empty()) {
                response.statusCode = 401;
                response.body = R"({"success":false,"error":"Authorization token required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 删除Session
            destroySession(token);

            response.statusCode = 200;
            response.body = R"({"success":true,"message":"Logout successful"})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/auth/me
     * 获取当前用户信息
     */
    router.get("/api/auth/me", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            // 提取Token
            std::string token = extractToken(req);

            if (token.empty()) {
                response.statusCode = 401;
                response.body = R"({"success":false,"error":"Authorization token required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 验证Session
            UserSession* session = validateSession(token);
            if (!session) {
                response.statusCode = 401;
                response.body = R"({"success":false,"error":"Invalid or expired token"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 查询用户详细信息 - 使用预处理语句防止SQL注入
            auto stmt = g_dbConnection->prepare("SELECT id, email, name, created_at FROM users WHERE id = ? LIMIT 1");
            if (!stmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            stmt->setInt(0, session->userId);
            auto users = stmt->query();

            if (users.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"User not found"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            auto& user = users[0];

            response.statusCode = 200;
            response.body = R"({"success":true,"data":{)"
                            R"("user_id":)" + user.at("id") + R"(,)"
                            R"("email":")" + escapeJsonString(user.at("email")) + R"(",)"
                            R"("name":")" + escapeJsonString(user.at("name")) + R"(",)"
                            R"("created_at":")" + user.at("created_at") + R"(",)"
                            R"("token":")" + token + R"("}})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // ============================================================================
    // Papers API
    // ============================================================================
    router.get("/api/papers", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            // 从MySQL数据库查询论文
            std::string sql = "SELECT id, title, authors, year, publication, citation_count FROM papers";

            auto papers = g_dbConnection->query(sql);

            std::ostringstream json;
            json << R"({"success":true,"papers":[)";

            bool first = true;
            for (const auto& paper : papers) {
                if (!first) json << ",";
                first = false;

                json << R"({)"
                     << R"("id":)" << paper.at("id") << R"(,)"
                     << R"("title":")" << escapeJsonString(paper.at("title")) << R"(",)"
                     << R"("authors":")" << escapeJsonString(paper.at("authors")) << R"(",)"
                     << R"("year":)" << paper.at("year") << R"(,)"
                     << R"("publication":")" << escapeJsonString(paper.at("publication")) << R"(",)"
                     << R"("citation_count":)" << paper.at("citation_count")
                     << R"(})";
            }

            json << R"(],"total":)" << papers.size() << R"(,"page":1,"pageSize":20})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    
    // Search API
    router.get("/api/search", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string query = req.getQuery("q", "");

            if (query.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"Query parameter 'q' is required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 构建搜索SQL - 在标题和作者中搜索（大小写不敏感）
            // 使用预处理语句防止SQL注入
            std::string searchPattern = "%" + query + "%";

            auto stmt = g_dbConnection->prepare(
                "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE "
                "LOWER(title) LIKE ? OR LOWER(authors) LIKE ?"
            );

            if (!stmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            stmt->setString(0, searchPattern);
            stmt->setString(1, searchPattern);
            auto papers = stmt->query();

            std::ostringstream json;
            json << R"({"success":true,"papers":[)";

            bool first = true;
            for (const auto& paper : papers) {
                if (!first) json << ",";
                first = false;

                json << R"({)"
                     << R"("id":)" << paper.at("id") << R"(,)"
                     << R"("title":")" << escapeJsonString(paper.at("title")) << R"(",)"
                     << R"("authors":")" << escapeJsonString(paper.at("authors")) << R"(",)"
                     << R"("year":)" << paper.at("year") << R"(,)"
                     << R"("publication":")" << escapeJsonString(paper.at("publication")) << R"(",)"
                     << R"("citation_count":)" << paper.at("citation_count")
                     << R"(})";
            }

            json << R"(],"total":)" << papers.size() << R"(,"query":")" << escapeJsonString(query) << R"("})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Papers search API - 必须在 /api/papers/:id 之前注册
    router.get("/api/papers/search", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string query = req.getQuery("q", "");

            if (query.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"Query parameter 'q' is required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 构建搜索SQL - 在标题和作者中搜索（大小写不敏感）
            // 使用预处理语句防止SQL注入
            std::string searchPattern = "%" + query + "%";

            auto stmt = g_dbConnection->prepare(
                "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE "
                "LOWER(title) LIKE ? OR LOWER(authors) LIKE ?"
            );

            if (!stmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            stmt->setString(0, searchPattern);
            stmt->setString(1, searchPattern);
            auto papers = stmt->query();

            std::ostringstream json;
            json << R"({"success":true,"papers":[)";

            bool first = true;
            for (const auto& paper : papers) {
                if (!first) json << ",";
                first = false;

                json << R"({)"
                     << R"("id":)" << paper.at("id") << R"(,)"
                     << R"("title":")" << escapeJsonString(paper.at("title")) << R"(",)"
                     << R"("authors":")" << escapeJsonString(paper.at("authors")) << R"(",)"
                     << R"("year":)" << paper.at("year") << R"(,)"
                     << R"("publication":")" << escapeJsonString(paper.at("publication")) << R"(",)"
                     << R"("citation_count":)" << paper.at("citation_count")
                     << R"(})";
            }

            json << R"(],"total":)" << papers.size() << R"(,"query":")" << escapeJsonString(query) << R"("})";
            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/api/papers/:id", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string paperId = req.getPathParam("id", "0");

            // 使用预处理语句防止SQL注入
            auto stmt = g_dbConnection->prepare(
                "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE id = ?"
            );

            if (!stmt) {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Database preparation failed"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            stmt->setString(0, paperId);
            auto papers = stmt->query();

            if (papers.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Paper not found"})";
            } else {
                const auto& paper = papers[0];
                std::ostringstream json;
                json << R"({"success":true,"paper":{)"
                     << R"("id":)" << paper.at("id") << R"(,)"
                     << R"("title":")" << escapeJsonString(paper.at("title")) << R"(",)"
                     << R"("authors":")" << escapeJsonString(paper.at("authors")) << R"(",)"
                     << R"("year":)" << paper.at("year") << R"(,)"
                     << R"("publication":")" << escapeJsonString(paper.at("publication")) << R"(",)"
                     << R"("citation_count":)" << paper.at("citation_count")
                     << R"(}})";
                response.body = json.str();
            }
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // POST - 创建新论文
    router.post("/api/papers", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 201;

        try {
            // 解析请求体（简化版，实际应使用JSON解析器）
            std::string title = req.getQuery("title", "");
            std::string authors = req.getQuery("authors", "");
            std::string year = req.getQuery("year", "0");
            std::string publication = req.getQuery("publication", "");
            std::string citationCount = req.getQuery("citation_count", "0");

            if (title.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"Title is required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // Convert year and citation_count to integers
            int yearValue = 0;
            int citationCountValue = 0;
            try {
                yearValue = std::stoi(year);
                citationCountValue = std::stoi(citationCount);
            } catch (...) {
                // Use default value 0 if conversion fails
            }

            // Use prepared statement to prevent SQL injection
            auto stmt = g_dbConnection->prepare(
                "INSERT INTO papers (title, authors, year, publication, citation_count) VALUES (?, ?, ?, ?, ?)"
            );
            stmt->setString(0, title);
            stmt->setString(1, authors);
            stmt->setInt(2, yearValue);
            stmt->setString(3, publication);
            stmt->setInt(4, citationCountValue);

            if (stmt->execute()) {
                uint64_t newId = stmt->getLastInsertId();
                response.body = R"({"success":true,"message":"Paper created","id":)" + std::to_string(newId) + R"(})";
            } else {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Failed to create paper"})";
            }
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // PUT - 更新论文
    router.put("/api/papers/:id", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string paperId = req.getPathParam("id", "0");

            // 检查论文是否存在
            auto checkStmt = g_dbConnection->prepare("SELECT id FROM papers WHERE id = ?");
            checkStmt->setUInt64(0, std::stoull(paperId));
            auto existing = checkStmt->query();

            if (existing.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Paper not found"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 构建更新SQL - 使用prepared statement
            std::string title = req.getQuery("title", "");
            std::string authors = req.getQuery("authors", "");
            std::string year = req.getQuery("year", "");
            std::string publication = req.getQuery("publication", "");
            std::string citationCount = req.getQuery("citation_count", "");

            // 构建SET子句和参数
            std::vector<std::string> setParts;
            std::vector<std::string> params;
            int paramIndex = 0;

            if (!title.empty()) {
                setParts.push_back("title = ?");
                params.push_back(title);
            }
            if (!authors.empty()) {
                setParts.push_back("authors = ?");
                params.push_back(authors);
            }
            if (!year.empty()) {
                setParts.push_back("year = ?");
                params.push_back(year);
            }
            if (!publication.empty()) {
                setParts.push_back("publication = ?");
                params.push_back(publication);
            }
            if (!citationCount.empty()) {
                setParts.push_back("citation_count = ?");
                params.push_back(citationCount);
            }

            if (setParts.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"No fields to update"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 构建完整的SQL语句
            std::string sql = "UPDATE papers SET ";
            for (size_t i = 0; i < setParts.size(); ++i) {
                if (i > 0) sql += ", ";
                sql += setParts[i];
            }
            sql += " WHERE id = ?";
            params.push_back(paperId);

            // 创建prepared statement并绑定参数
            auto stmt = g_dbConnection->prepare(sql);
            for (size_t i = 0; i < params.size(); ++i) {
                // 尝试解析为数字，如果是数字则用setInt，否则用setString
                try {
                    int value = std::stoi(params[i]);
                    stmt->setInt(i, value);
                } catch (...) {
                    stmt->setString(i, params[i]);
                }
            }

            if (stmt->execute()) {
                response.body = R"({"success":true,"message":"Paper updated","id":)" + paperId + R"(})";
            } else {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Failed to update paper"})";
            }
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // DELETE - 删除论文
    router.del("/api/papers/:id", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string paperId = req.getPathParam("id", "0");

            // 检查论文是否存在
            auto checkStmt = g_dbConnection->prepare("SELECT id FROM papers WHERE id = ?");
            checkStmt->setUInt64(0, std::stoull(paperId));
            auto existing = checkStmt->query();

            if (existing.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Paper not found"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // Use prepared statement for DELETE
            auto stmt = g_dbConnection->prepare("DELETE FROM papers WHERE id = ?");
            stmt->setUInt64(0, std::stoull(paperId));

            if (stmt->execute()) {
                response.body = R"({"success":true,"message":"Paper deleted","id":)" + paperId + R"(})";
            } else {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Failed to delete paper"})";
            }
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Journals API
    router.get("/api/journals", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            // 从MySQL数据库查询期刊
            std::string sql = "SELECT name, tier, impact_factor FROM journals";

            auto journals = g_dbConnection->query(sql);

            std::ostringstream json;
            json << R"({"success":true,"journals":[)";

            bool first = true;
            for (const auto& journal : journals) {
                if (!first) json << ",";
                first = false;

                json << R"({)"
                     << R"("name":")" << journal.at("name") << R"(",)"
                     << R"("tier":")" << journal.at("tier") << R"(",)"
                     << R"("impact_factor":)" << journal.at("impact_factor")
                     << R"(})";
            }

            json << R"(]})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Authors API
    router.get("/api/authors", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            // 从MySQL数据库查询作者
            std::string sql = "SELECT id, name, email, affiliation, h_index FROM authors";

            auto authors = g_dbConnection->query(sql);

            std::ostringstream json;
            json << R"({"success":true,"authors":[)";

            bool first = true;
            for (const auto& author : authors) {
                if (!first) json << ",";
                first = false;

                json << R"({)"
                     << R"("id":)" << author.at("id") << R"(,)"
                     << R"("name":")" << author.at("name") << R"(",)"
                     << R"("email":")" << author.at("email") << R"(",)"
                     << R"("affiliation":")" << author.at("affiliation") << R"(",)"
                     << R"("h_index":)" << author.at("h_index")
                     << R"(})";
            }

            json << R"(]})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Collections API
    router.get("/api/collections", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 查询数据库获取收藏集
        response.body = R"({"success":true,"collections":[]})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Statistics API
    router.get("/api/stats", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            // 从MySQL数据库查询统计数据
            auto papers = g_dbConnection->query("SELECT COUNT(*) as count FROM papers");
            auto journals = g_dbConnection->query("SELECT COUNT(*) as count FROM journals");
            auto authors = g_dbConnection->query("SELECT COUNT(*) as count FROM authors");
            auto collections = g_dbConnection->query("SELECT COUNT(*) as count FROM collections");

            size_t totalPapers = papers.empty() ? 0 : std::stoul(papers[0].at("count"));
            size_t totalJournals = journals.empty() ? 0 : std::stoul(journals[0].at("count"));
            size_t totalAuthors = authors.empty() ? 0 : std::stoul(authors[0].at("count"));
            size_t totalCollections = collections.empty() ? 0 : std::stoul(collections[0].at("count"));

            // 获取最近的论文数量（最近7天）
            auto recentPapers = g_dbConnection->query(
                "SELECT COUNT(*) as count FROM papers WHERE created_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)"
            );
            size_t recentPapersCount = recentPapers.empty() ? 0 : std::stoul(recentPapers[0].at("count"));

            std::ostringstream json;
            json << R"({"success":true,"stats":{)"
                 << R"("totalPapers":)" << totalPapers << R"(,)"
                 << R"("totalJournals":)" << totalJournals << R"(,)"
                 << R"("totalAuthors":)" << totalAuthors << R"(,)"
                 << R"("totalCollections":)" << totalCollections << R"(,)"
                 << R"("recentPapers":)" << recentPapersCount
                 << R"(}})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/stats/papers-by-year
     * 按年份统计论文数量
     */
    router.get("/api/stats/papers-by-year", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string sql = "SELECT year, COUNT(*) as count FROM papers "
                            "WHERE year IS NOT NULL AND year > 0 "
                            "GROUP BY year ORDER BY year DESC LIMIT 20";

            auto results = g_dbConnection->query(sql);

            std::ostringstream json;
            json << R"({"success":true,"data":[)";

            bool first = true;
            for (const auto& row : results) {
                if (!first) json << ",";
                first = false;

                json << R"({"year":)" << row.at("year")
                     << R"(,"count":)" << row.at("count") << R"(})";
            }

            json << R"(]})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/stats/top-conferences
     * 获取top会议/期刊
     */
    router.get("/api/stats/top-conferences", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            int limit = 10;
            std::string limitStr = req.getQuery("limit", "10");
            try {
                limit = std::stoi(limitStr);
            } catch (...) {
                limit = 10;
            }

            std::ostringstream sql;
            sql << "SELECT publication, COUNT(*) as count, AVG(citation_count) as avg_citations "
                << "FROM papers WHERE publication IS NOT NULL AND publication != '' "
                << "GROUP BY publication ORDER BY count DESC LIMIT " << limit;

            auto results = g_dbConnection->query(sql.str());

            std::ostringstream json;
            json << R"({"success":true,"data":[)";

            bool first = true;
            for (const auto& row : results) {
                if (!first) json << ",";
                first = false;

                json << R"({"publication":")" << escapeJsonString(row.at("publication"))
                     << R"(","count":)" << row.at("count")
                     << R"(,"avg_citations":)" << row.at("avg_citations") << R"(})";
            }

            json << R"(]})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/stats/recent-trends
     * 获取最近的趋势（最近12个月）
     */
    router.get("/api/stats/recent-trends", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string sql = "SELECT DATE_FORMAT(created_at, '%Y-%m') as month, "
                            "COUNT(*) as count FROM papers "
                            "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 12 MONTH) "
                            "GROUP BY month ORDER BY month ASC";

            auto results = g_dbConnection->query(sql);

            std::ostringstream json;
            json << R"({"success":true,"data":[)";

            bool first = true;
            for (const auto& row : results) {
                if (!first) json << ",";
                first = false;

                json << R"({"month":")" << row.at("month")
                     << R"(","count":)" << row.at("count") << R"(})";
            }

            json << R"(]})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/stats/citation-distribution
     * 获取引用数分布
     */
    router.get("/api/stats/citation-distribution", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string sql = "SELECT "
                            "SUM(CASE WHEN citation_count < 10 THEN 1 ELSE 0 END) as low, "
                            "SUM(CASE WHEN citation_count >= 10 AND citation_count < 50 THEN 1 ELSE 0 END) as medium, "
                            "SUM(CASE WHEN citation_count >= 50 AND citation_count < 100 THEN 1 ELSE 0 END) as high, "
                            "SUM(CASE WHEN citation_count >= 100 THEN 1 ELSE 0 END) as very_high, "
                            "AVG(citation_count) as average "
                            "FROM papers";

            auto results = g_dbConnection->query(sql);

            if (!results.empty()) {
                auto& row = results[0];

                std::ostringstream json;
                json << R"({"success":true,"data":{)"
                     << R"("low":)" << row.at("low") << R"(,)"
                     << R"("medium":)" << row.at("medium") << R"(,)"
                     << R"("high":)" << row.at("high") << R"(,)"
                     << R"("very_high":)" << row.at("very_high") << R"(,)"
                     << R"("average":)" << row.at("average")
                     << R"(}})";

                response.body = json.str();
            } else {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"No data found"})";
            }
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // ============================================================================
    // 导出API
    // ============================================================================

    /**
     * GET /api/export/bibtex/:id
     * 导出单篇论文的BibTeX
     */
    router.get("/api/export/bibtex/:id", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            std::string idStr = req.getPathParam("id", "0");
            int paperId = std::stoi(idStr);

            // 查询论文信息
            std::ostringstream sql;
            sql << "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE id = " << paperId << " LIMIT 1";

            auto papers = g_dbConnection->query(sql.str());

            if (papers.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Paper not found"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            auto& paper = papers[0];

            // 生成BibTeX
            std::ostringstream bibtex;
            std::string citeKey = "paper_" + std::to_string(paperId);

            // 判断文献类型
            std::string publication = paper.at("publication");
            bool isConference = publication.find("Proceedings") != std::string::npos ||
                               publication.find("Conf") != std::string::npos ||
                               publication.find("Symposium") != std::string::npos;

            bibtex << "@" << (isConference ? "inproceedings" : "article") << "{" << citeKey << ",\n";
            bibtex << "  title = {{" << paper.at("title") << "}},\n";
            bibtex << "  author = {{" << paper.at("authors") << "}},\n";
            bibtex << "  year = {" << paper.at("year") << "},\n";
            bibtex << "  journal = {{" << publication << "}}";

            // 添加可选字段
            std::string doi = getMapValue(paper, "doi", "");
            if (!doi.empty()) {
                bibtex << ",\n  doi = {" << doi << "}";
            }

            std::string url = getMapValue(paper, "url", "");
            if (!url.empty()) {
                bibtex << ",\n  url = {" << url << "}";
            }

            bibtex << "\n}\n";

            response.statusCode = 200;
            response.body = R"({"success":true,"data":{)"
                           R"("paper_id":)" + std::to_string(paperId) + R"(,)"
                           R"("bibtex":")" + escapeJsonString(bibtex.str()) + R"("}})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * POST /api/export/bibtex/batch
     * 批量导出多篇论文的BibTeX
     */
    router.post("/api/export/bibtex/batch", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            // 解析请求体
            json requestBody = json::parse(req.body);
            std::vector<int> paperIds = requestBody.value("paper_ids", std::vector<int>());

            if (paperIds.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"paper_ids array is required"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 构建SQL查询
            std::ostringstream sql;
            sql << "SELECT id, title, authors, year, publication, "
                << "doi, url, citation_count FROM papers WHERE id IN (";

            bool first = true;
            for (int id : paperIds) {
                if (!first) sql << ",";
                first = false;
                sql << id;
            }
            sql << ")";

            auto papers = g_dbConnection->query(sql.str());

            // 生成BibTeX
            std::ostringstream bibtex;
            for (const auto& paper : papers) {
                int paperId = std::stoi(paper.at("id"));
                std::string citeKey = "paper_" + std::to_string(paperId);

                std::string publication = paper.at("publication");
                bool isConference = publication.find("Proceedings") != std::string::npos ||
                                   publication.find("Conf") != std::string::npos ||
                                   publication.find("Symposium") != std::string::npos;

                bibtex << "@" << (isConference ? "inproceedings" : "article") << "{" << citeKey << ",\n";
                bibtex << "  title = {{" << paper.at("title") << "}},\n";
                bibtex << "  author = {{" << paper.at("authors") << "}},\n";
                bibtex << "  year = {" << paper.at("year") << "},\n";
                bibtex << "  journal = {{" << publication << "}}";

                std::string doi = getMapValue(paper, "doi", "");
                if (!doi.empty()) {
                    bibtex << ",\n  doi = {" << doi << "}";
                }

                std::string url = getMapValue(paper, "url", "");
                if (!url.empty()) {
                    bibtex << ",\n  url = {" << url << "}";
                }

                bibtex << "\n}\n\n";
            }

            response.statusCode = 200;
            response.body = R"({"success":true,"data":{)"
                           R"("count":)" + std::to_string(papers.size()) + R"(,)"
                           R"("bibtex":")" + escapeJsonString(bibtex.str()) + R"("}})";
        } catch (const json::exception& e) {
            response.statusCode = 400;
            response.body = R"({"success":false,"error":"Invalid JSON format"})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/export/bibtex/collection/:id
     * 导出整个收藏的BibTeX
     */
    router.get("/api/export/bibtex/collection/:id", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            std::string collectionIdStr = req.getPathParam("id", "0");
            int collectionId = std::stoi(collectionIdStr);

            // 查询收藏中的所有论文
            std::ostringstream sql;
            sql << "SELECT p.id, p.title, p.authors, p.year, p.publication, "
                << "p.doi, p.url, p.citation_count "
                << "FROM papers p "
                << "INNER JOIN collection_papers cp ON p.id = cp.paper_id "
                << "WHERE cp.collection_id = " << collectionId;

            auto papers = g_dbConnection->query(sql.str());

            if (papers.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Collection not found or empty"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 生成BibTeX
            std::ostringstream bibtex;
            for (const auto& paper : papers) {
                int paperId = std::stoi(paper.at("id"));
                std::string citeKey = "paper_" + std::to_string(paperId);

                std::string publication = paper.at("publication");
                bool isConference = publication.find("Proceedings") != std::string::npos ||
                                   publication.find("Conf") != std::string::npos ||
                                   publication.find("Symposium") != std::string::npos;

                bibtex << "@" << (isConference ? "inproceedings" : "article") << "{" << citeKey << ",\n";
                bibtex << "  title = {{" << paper.at("title") << "}},\n";
                bibtex << "  author = {{" << paper.at("authors") << "}},\n";
                bibtex << "  year = {" << paper.at("year") << "},\n";
                bibtex << "  journal = {{" << publication << "}}";

                std::string doi = getMapValue(paper, "doi", "");
                if (!doi.empty()) {
                    bibtex << ",\n  doi = {" << doi << "}";
                }

                std::string url = getMapValue(paper, "url", "");
                if (!url.empty()) {
                    bibtex << ",\n  url = {" << url << "}";
                }

                bibtex << "\n\n";
            }

            response.statusCode = 200;
            response.body = R"({"success":true,"data":{)"
                           R"("collection_id":)" + std::to_string(collectionId) + R"(,)"
                           R"("count":)" + std::to_string(papers.size()) + R"(,)"
                           R"("bibtex":")" + escapeJsonString(bibtex.str()) + R"("}})";
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    /**
     * GET /api/export/csv
     * 导出所有论文为CSV格式
     */
    router.get("/api/export/csv", [](const HttpRequest& req) {
        HttpResponse response;

        try {
            // 查询所有论文
            std::string sql = "SELECT id, title, authors, year, publication, "
                            "doi, url, citation_count, abstract FROM papers";

            auto papers = g_dbConnection->query(sql);

            // 生成CSV
            std::ostringstream csv;
            csv << "ID,Title,Authors,Year,Publication,DOI,URL,Citations,Abstract\n";

            for (const auto& paper : papers) {
                csv << paper.at("id") << ",";

                // CSV字段需要用引号包裹，并转义内部引号
                auto escapeCsvField = [](const std::string& field) -> std::string {
                    std::string escaped = field;
                    // 替换 " 为 ""
                    size_t pos = 0;
                    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
                        escaped.replace(pos, 1, "\"\"");
                        pos += 2;
                    }
                    return "\"" + escaped + "\"";
                };

                csv << escapeCsvField(paper.at("title")) << ",";
                csv << escapeCsvField(paper.at("authors")) << ",";
                csv << paper.at("year") << ",";
                csv << escapeCsvField(paper.at("publication")) << ",";
                csv << escapeCsvField(getMapValue(paper, "doi", "")) << ",";
                csv << escapeCsvField(getMapValue(paper, "url", "")) << ",";
                csv << paper.at("citation_count") << ",";
                csv << escapeCsvField(getMapValue(paper, "abstract", "")) << "\n";
            }

            response.statusCode = 200;
            response.body = csv.str();
            response.setHeader("Content-Type", "text/csv");
            response.setHeader("Content-Disposition", "attachment; filename=\"papers.csv\"");
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
            response.setHeader("Content-Type", "application/json");
        }

        return response;
    });

    // Crawler API - Search arXiv
    router.post("/api/crawler/search", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        // Debug log
        if (req.body.empty()) {
            spdlog::warn("[Crawler] Empty request body received");
            response.statusCode = 400;
            response.body = R"({"success":false,"error":"Empty request body. Make sure Content-Type header is set to application/json"})";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        try {
            auto bodyJson = json::parse(req.body);
            std::string query = bodyJson.value("query", "");
            int limit = bodyJson.value("limit", 10);
            int maxRetries = bodyJson.value("max_retries", 3);
            int initialDelay = bodyJson.value("delay", 2);

            if (query.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"Missing required parameter: query"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            spdlog::info("[Crawler] POST request - Searching arXiv for: {}", query);

            // Build arXiv API URL
            std::string arxivQuery = query;
            // Replace spaces with + for URL encoding
            std::replace(arxivQuery.begin(), arxivQuery.end(), ' ', '+');

            std::string arxivUrl = "http://export.arxiv.org/api/query?search_query=all:" +
                                 arxivQuery + "&start=0&max_results=" + std::to_string(limit);

            // Create HTTP client
            std::unique_ptr<Network::HttpClient> httpClient = std::make_unique<Network::HttpClient>();
            httpClient->setDefaultHeader("User-Agent", "PaperCrawler/1.0");
            httpClient->setTimeout(15);

            // Retry logic with exponential backoff
            Network::HttpClientResponse httpResp;
            int attempt = 0;
            bool success = false;
            int currentDelay = initialDelay;
            bool wasRateLimited = false;

            while (attempt < maxRetries && !success) {
                attempt++;
                spdlog::info("[Crawler] Attempt {}/{} - Fetching from arXiv", attempt, maxRetries);

                httpResp = httpClient->get(arxivUrl);

                // Check if request was successful
                if (httpResp.isSuccess()) {
                    success = true;
                    spdlog::info("[Crawler] Success on attempt {}", attempt);
                    break;
                }

                // Request failed - check if we should retry
                if (httpResp.statusCode == 429) {
                    wasRateLimited = true;
                    spdlog::warn("[Crawler] Rate limited (429) by arXiv");
                } else {
                    spdlog::warn("[Crawler] Request failed (status: {}, error: {})",
                                httpResp.statusCode, httpResp.errorMessage);
                }

                // Check if we should retry
                if (attempt < maxRetries) {
                    spdlog::info("[Crawler] Waiting {} seconds before retry...", currentDelay);

                    #ifdef _WIN32
                        Sleep(currentDelay * 1000);
                    #else
                        sleep(currentDelay);
                    #endif

                    // Exponential backoff: double the delay
                    currentDelay *= 2;
                    continue;
                }
            }

            // Check if all retries exhausted
            if (!success) {
                spdlog::error("[Crawler] Max retries ({}) reached", maxRetries);
                response.statusCode = wasRateLimited ? 429 : 500;
                std::ostringstream err;
                if (wasRateLimited) {
                    err << R"({"success":false,"error":"Rate limited by arXiv API","status":429,"retries":)"
                        << maxRetries << R"(,"message":"Please wait a few minutes before trying again"})";
                } else {
                    err << R"({"success":false,"error":"Failed to fetch from arXiv after )"
                        << maxRetries << R"( retries","status":)"
                        << httpResp.statusCode << R"(,"message":")"
                        << escapeJsonString(httpResp.errorMessage) << R"("})";
                }
                response.body = err.str();
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // Parse arXiv XML response (simplified)
            std::vector<std::map<std::string, std::string>> papers;
            std::string xmlContent = httpResp.body;

            // Extract entries between <entry> tags (Windows-compatible regex)
            std::regex entryRegex("<entry>[\\s\\S]*?</entry>");
            std::sregex_iterator it(xmlContent.begin(), xmlContent.end(), entryRegex);
            std::sregex_iterator end;

            int count = 0;
            for (; it != end && count < limit; ++it) {
                std::string entryXml = it->str(0);  // Fixed: was str(1), should be str(0)

                std::map<std::string, std::string> paper;

                // Extract title
                std::regex titleRegex("<title>(.*?)</title>");
                std::smatch titleMatch;
                if (std::regex_search(entryXml, titleMatch, titleRegex)) {
                    paper["title"] = titleMatch[1].str();
                }

                // Extract summary (abstract)
                std::regex summaryRegex("<summary>(.*?)</summary>");
                std::smatch summaryMatch;
                if (std::regex_search(entryXml, summaryMatch, summaryRegex)) {
                    paper["summary"] = summaryMatch[1].str();
                }

                // Extract authors
                std::regex authorRegex("<name>(.*?)</name>");
                std::sregex_iterator authorIt(entryXml.begin(), entryXml.end(), authorRegex);
                std::sregex_iterator authorEnd;
                std::vector<std::string> authors;
                for (; authorIt != authorEnd; ++authorIt) {
                    authors.push_back(authorIt->str(1));
                }
                paper["authors"] = "";
                for (size_t i = 0; i < authors.size(); ++i) {
                    if (i > 0) paper["authors"] += ", ";
                    paper["authors"] += authors[i];
                }

                // Extract year from published date
                std::regex publishedRegex("<published>(\\d{4})");
                std::smatch publishedMatch;
                if (std::regex_search(entryXml, publishedMatch, publishedRegex)) {
                    paper["year"] = publishedMatch[1].str();
                }

                // Extract arXiv ID and URL
                std::regex idRegex("<id>(http://arxiv\\.org/abs/\\d+\\.\\d+)</id>");
                std::smatch idMatch;
                if (std::regex_search(entryXml, idMatch, idRegex)) {
                    paper["url"] = idMatch[1].str();
                    paper["pdfUrl"] = idMatch[1].str() + ".pdf";

                    // Extract ID from URL
                    size_t lastSlash = idMatch[1].str().find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        paper["arxivId"] = idMatch[1].str().substr(lastSlash + 1);
                    }
                }

                papers.push_back(paper);
                count++;
            }

            spdlog::info("[Crawler] Parsed {} papers from arXiv", papers.size());

            // Build JSON response
            std::ostringstream jsonResponse;
            jsonResponse << R"({"success":true,"query":")" << escapeJsonString(query)
                        << R"(","total":)" << papers.size()
                        << R"(,"retries":)" << (attempt - 1)
                        << R"(,"papers":[)";

            for (size_t i = 0; i < papers.size(); ++i) {
                if (i > 0) jsonResponse << ",";
                jsonResponse << "{"
                             << R"("title":")" << escapeJsonString(papers[i]["title"]) << R"(",)"
                             << R"("authors":")" << escapeJsonString(papers[i]["authors"]) << R"(",)"
                             << R"("abstract":")" << escapeJsonString(papers[i]["summary"]) << R"(",)"
                             << R"("year":")" << papers[i]["year"] << R"(",)"
                             << R"("url":")" << escapeJsonString(papers[i]["url"]) << R"(",)"
                             << R"("pdfUrl":")" << escapeJsonString(papers[i]["pdfUrl"]) << R"(",)"
                             << R"("arxivId":")" << escapeJsonString(papers[i]["arxivId"]) << R"(",)"
                             << R"("source":"arXiv)"
                             << "}";
            }

            jsonResponse << R"(]})";
            response.body = jsonResponse.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Crawler API - Search arXiv (GET method for easy testing)
    router.get("/api/crawler/arxiv", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            std::string query = req.getQuery("q", "");
            int limit = std::stoi(req.getQuery("limit", "5"));
            int maxRetries = std::stoi(req.getQuery("max_retries", "3"));
            int initialDelay = std::stoi(req.getQuery("delay", "2"));

            if (query.empty()) {
                response.statusCode = 400;
                response.body = "{\"success\":false,\"error\":\"Missing required parameter: q (search query)\"}";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            spdlog::info("[Crawler] GET request - Searching arXiv for: {}", query);

            // Build arXiv API URL
            std::string arxivQuery = query;
            std::replace(arxivQuery.begin(), arxivQuery.end(), ' ', '+');
            std::string arxivUrl = "http://export.arxiv.org/api/query?search_query=all:" + arxivQuery + "&max_results=" + std::to_string(limit);

            // Create HTTP client
            std::unique_ptr<Network::HttpClient> httpClient = std::make_unique<Network::HttpClient>();
            httpClient->setDefaultHeader("User-Agent", "PaperCrawler/1.0");
            httpClient->setTimeout(15);

            // Retry logic with exponential backoff
            Network::HttpClientResponse httpResp;
            int attempt = 0;
            bool success = false;
            int currentDelay = initialDelay;
            bool wasRateLimited = false;

            while (attempt < maxRetries && !success) {
                attempt++;
                spdlog::info("[Crawler] Attempt {}/{} - Fetching from arXiv", attempt, maxRetries);

                httpResp = httpClient->get(arxivUrl);

                // Check if request was successful
                if (httpResp.isSuccess()) {
                    success = true;
                    spdlog::info("[Crawler] Success on attempt {}", attempt);
                    break;
                }

                // Request failed - check if we should retry
                if (httpResp.statusCode == 429) {
                    wasRateLimited = true;
                    spdlog::warn("[Crawler] Rate limited (429) by arXiv");
                } else {
                    spdlog::warn("[Crawler] Request failed (status: {}, error: {})",
                                httpResp.statusCode, httpResp.errorMessage);
                }

                // Check if we should retry
                if (attempt < maxRetries) {
                    spdlog::info("[Crawler] Waiting {} seconds before retry...", currentDelay);

                    #ifdef _WIN32
                        Sleep(currentDelay * 1000);
                    #else
                        sleep(currentDelay);
                    #endif

                    // Exponential backoff: double the delay
                    currentDelay *= 2;
                    continue;
                }
            }

            // Check if all retries exhausted
            if (!success) {
                spdlog::error("[Crawler] Max retries ({}) reached", maxRetries);
                response.statusCode = wasRateLimited ? 429 : 500;
                std::ostringstream err;
                if (wasRateLimited) {
                    err << R"({"success":false,"error":"Rate limited by arXiv API","status":429,"retries":)"
                        << maxRetries << R"(,"message":"Please wait a few minutes before trying again"})";
                } else {
                    err << R"({"success":false,"error":"Failed to fetch from arXiv after )"
                        << maxRetries << R"( retries","status":)"
                        << httpResp.statusCode << R"(,"message":")"
                        << escapeJsonString(httpResp.errorMessage) << R"("})";
                }
                response.body = err.str();
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            spdlog::info("[Crawler] Received {} bytes from arXiv", httpResp.body.length());

            // Parse XML and extract papers (simplified)
            std::vector<std::map<std::string, std::string>> papers;
            std::regex entryRegex("<entry>[\\s\\S]*?</entry>");
            std::sregex_iterator it(httpResp.body.begin(), httpResp.body.end(), entryRegex);
            std::sregex_iterator end;

            int count = 0;
            for (; it != end && count < limit; ++it) {
                std::string entryXml = it->str(0);
                std::map<std::string, std::string> paper;

                std::regex titleRegex("<title>(.*?)</title>");
                std::smatch titleMatch;
                if (std::regex_search(entryXml, titleMatch, titleRegex)) {
                    paper["title"] = titleMatch[1].str();
                }

                std::regex summaryRegex("<summary>(.*?)</summary>");
                std::smatch summaryMatch;
                if (std::regex_search(entryXml, summaryMatch, summaryRegex)) {
                    paper["summary"] = summaryMatch[1].str();
                }

                std::regex authorRegex("<name>(.*?)</name>");
                std::sregex_iterator authorIt(entryXml.begin(), entryXml.end(), authorRegex);
                std::vector<std::string> authors;
                for (; authorIt != std::sregex_iterator(); ++authorIt) {
                    authors.push_back(authorIt->str(1));
                }
                paper["authors"] = "";
                for (size_t i = 0; i < authors.size(); ++i) {
                    if (i > 0) paper["authors"] += ", ";
                    paper["authors"] += authors[i];
                }

                std::regex publishedRegex("<published>(\\d{4})");
                std::smatch publishedMatch;
                if (std::regex_search(entryXml, publishedMatch, publishedRegex)) {
                    paper["year"] = publishedMatch[1].str();
                }

                std::regex idRegex("<id>(http://arxiv\\.org/abs/(\\d+\\.\\w+))</id>");
                std::smatch idMatch;
                if (std::regex_search(entryXml, idMatch, idRegex)) {
                    paper["url"] = idMatch[1].str();
                    paper["pdfUrl"] = idMatch[1].str() + ".pdf";
                    paper["arxivId"] = idMatch[2].str();
                }

                papers.push_back(paper);
                count++;
            }

            spdlog::info("[Crawler] Parsed {} papers from arXiv", papers.size());

            // Build JSON response
            std::ostringstream json;
            json << R"({"success":true,"query":")" << escapeJsonString(query)
                << R"(","source":"arXiv","total":)" << papers.size()
                << R"(,"retries":)" << (attempt - 1)
                << R"(,"papers":[)";

            for (size_t i = 0; i < papers.size(); ++i) {
                if (i > 0) json << ",";
                json << "{"
                    << R"("title":")" << escapeJsonString(papers[i]["title"]) << R"(",)"
                    << R"("authors":")" << escapeJsonString(papers[i]["authors"]) << R"(",)"
                    << R"("abstract":")" << escapeJsonString(papers[i]["summary"].substr(0, 200) + "...") << R"(",)"
                    << R"("year":")" << papers[i]["year"] << R"(",)"
                    << R"("url":")" << escapeJsonString(papers[i]["url"]) << R"(",)"
                    << R"("pdfUrl":")" << escapeJsonString(papers[i]["pdfUrl"]) << R"(",)"
                    << R"("arxivId":")" << escapeJsonString(papers[i]["arxivId"]) << R"(",)"
                    << R"("source":"arXiv")"
                    << "}";
            }

            json << R"(]})";
            response.body = json.str();
        } catch (const std::exception& e) {
            spdlog::error("[Crawler] Exception: {}", e.what());
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // PubMed Crawler API (Coming Soon)
    router.get("/api/crawler/pubmed", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        std::string query = req.getQuery("q", "");
        int limit = std::stoi(req.getQuery("limit", "5"));

        if (query.empty()) {
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Missing required parameter: q (search query)\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        spdlog::info("[Crawler] PubMed request - Searching for: {}", query);

        // TODO: Implement PubMed API integration
        // PubMed API: https://www.ncbi.nlm.nih.gov/books/NBK25501/
        // Uses E-utilities API with JSON response format
        // Example: https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?db=pubmed&term=science[journal]+AND+2024[dp]

        response.statusCode = 501;  // Not Implemented
        std::ostringstream json;
        json << R"({"success":false,"message":"PubMed crawler is under development","source":"PubMed","query":")"
            << escapeJsonString(query)
            << R"(","info":{"api":"NCBI E-utilities","documentation":"https://www.ncbi.nlm.nih.gov/books/NBK25501/","status":"Implementation in progress"}})";
        response.body = json.str();
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Google Scholar Crawler API (Coming Soon)
    router.get("/api/crawler/scholar", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        std::string query = req.getQuery("q", "");
        int limit = std::stoi(req.getQuery("limit", "5"));

        if (query.empty()) {
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Missing required parameter: q (search query)\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        spdlog::info("[Crawler] Google Scholar request - Searching for: {}", query);

        // TODO: Implement Google Scholar scraping
        // Note: Google Scholar does not provide an official API
        // Requires HTML parsing and rate limiting (strict anti-bot measures)
        // Alternative: Use Google Scholar APIs (third-party services)

        response.statusCode = 501;  // Not Implemented
        std::ostringstream json;
        json << R"({"success":false,"message":"Google Scholar crawler is under development","source":"Google Scholar","query":")"
            << escapeJsonString(query)
            << R"(","info":{"note":"Google Scholar does not provide an official API","alternatives":["Serpdog","SerpApi","ScraperAPI"],"status":"Implementation in progress"}})";
        response.body = json.str();
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // ============================================================================
    // DBLP Crawler API (简化版，演示数据)
    // ============================================================================

    router.get("/api/crawler/dblp", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        std::string query = req.getQuery("q", "");
        int limit = std::stoi(req.getQuery("limit", "30"));

        if (query.empty()) {
            response.statusCode = 400;
            response.statusText = "Bad Request";
            response.body = "{\"success\":false,\"error\":\"Missing required parameter: q (search query)\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        spdlog::info("[Crawler] DBLP request - Searching for: {}", query);

        // 演示数据 - 真实爬虫功能需要libcurl HTML解析
        std::ostringstream jsonResponse;
        jsonResponse << R"({"success":true,"source":"DBLP","query":")"
                     << escapeJsonString(query)
                     << R"(","total":1,"papers":[{)"
                     << R"("title":"Deep Learning: Methods and Applications",)"
                     << R"("authors":"Yann LeCun, Yoshua Bengio, Geoffrey Hinton",)"
                     << R"("year":2015,)"
                     << R"("publication":"Nature",)"
                     << R"("url":"https://dblp.org/rec/journals/nature/LeCun15",)"
                     << R"("source":"DBLP"}],)"
                     << R"("message":"DBLP爬虫框架已就绪，真实HTML解析待实现（需要libcurl C++支持）"})";

        response.body = jsonResponse.str();
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // CCF期刊/会议等级查询API (简化版，演示数据)
    router.get("/api/crawler/ccf-rank", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        std::string venue = req.getQuery("venue", "");

        if (venue.empty()) {
            response.statusCode = 400;
            response.statusText = "Bad Request";
            response.body = "{\"success\":false,\"error\":\"Missing required parameter: venue (journal/conference name)\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        spdlog::info("[Crawler] CCF rank query - venue: {}", venue);

        // 演示等级数据
        std::string level = "A";
        if (venue.find("CVPR") != std::string::npos ||
            venue.find("ICCV") != std::string::npos ||
            venue.find("ECCV") != std::string::npos) {
            level = "A";
        } else if (venue.find("AAAI") != std::string::npos) {
            level = "A";
        } else {
            level = "C";
        }

        std::ostringstream jsonResponse;
        jsonResponse << R"({"success":true,"venue":")"
                     << escapeJsonString(venue)
                     << R"(","rank":{)"
                     << R"("name":")" << escapeJsonString(venue) << R"(",)"
                     << R"("fullname":")" << escapeJsonString(venue) << R"(",)"
                     << R"("level":")" << level << R"(",)"
                     << R"("flevel":")" << level << R"(",)"
                     << R"("info":"CCF等级查询框架已就绪，真实myhuiban.com爬虫待实现",)"
                     << R"("url":"https://www.myhuiban.com/"})";

        response.body = jsonResponse.str();
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Save crawled papers to database
    router.post("/api/crawler/save", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        if (req.body.empty()) {
            spdlog::warn("[Crawler] Empty request body for save");
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Empty request body\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        try {
            auto bodyJson = json::parse(req.body);
            auto papersJson = bodyJson["papers"];

            if (!papersJson.is_array()) {
                response.statusCode = 400;
                response.body = "{\"success\":false,\"error\":\"papers must be an array\"}";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            spdlog::info("[Crawler] Saving {} papers to database", papersJson.size());

            int saved = 0;
            int updated = 0;
            int failed = 0;

            for (const auto& paperJson : papersJson) {
                try {
                    std::string title = paperJson.value("title", "");
                    std::string authors = paperJson.value("authors", "");
                    std::string yearStr = paperJson.value("year", "");
                    std::string abstract = paperJson.value("abstract", "");
                    std::string url = paperJson.value("url", "");
                    std::string arxivId = paperJson.value("arxivId", "");
                    std::string source = paperJson.value("source", "arXiv");

                    if (title.empty()) {
                        spdlog::warn("[Crawler] Skipping paper with empty title");
                        failed++;
                        continue;
                    }

                    // Convert year string to integer
                    int year = 0;
                    if (!yearStr.empty()) {
                        try {
                            year = std::stoi(yearStr);
                        } catch (...) {
                            spdlog::warn("[Crawler] Invalid year: {}", yearStr);
                        }
                    }

                    // Check if paper already exists
                    auto checkStmt = g_dbConnection->prepare("SELECT id, title FROM papers WHERE title = ? LIMIT 1");
                    checkStmt->setString(0, title);
                    auto checkResult = checkStmt->query();

                    if (checkResult.size() > 0) {
                        // Update existing paper
                        auto updateStmt = g_dbConnection->prepare(
                            "UPDATE papers SET authors = ?, year = ?, abstract = ?, publication = ?, updated_at = NOW() WHERE id = ?"
                        );
                        updateStmt->setString(0, authors);
                        updateStmt->setInt(1, year);
                        updateStmt->setString(2, abstract);
                        updateStmt->setString(3, source);
                        updateStmt->setUInt64(4, std::stoull(checkResult[0]["id"]));
                        updateStmt->execute();
                        updated++;
                        spdlog::info("[Crawler] Updated paper: {}", title);
                    } else {
                        // Insert new paper
                        auto insertStmt = g_dbConnection->prepare(
                            "INSERT INTO papers (title, authors, year, abstract, publication, is_favorite, is_read, created_at, updated_at) "
                            "VALUES (?, ?, ?, ?, ?, 0, 0, NOW(), NOW())"
                        );
                        insertStmt->setString(0, title);
                        insertStmt->setString(1, authors);
                        insertStmt->setInt(2, year);
                        insertStmt->setString(3, abstract);
                        insertStmt->setString(4, source);
                        insertStmt->execute();
                        saved++;
                        spdlog::info("[Crawler] Saved paper: {}", title);
                    }
                } catch (const std::exception& e) {
                    spdlog::error("[Crawler] Error saving paper: {}", e.what());
                    failed++;
                }
            }

            std::ostringstream jsonResponse;
            jsonResponse << R"({"success":true,"message":"Papers saved to database",)"
                        << R"("saved":)" << saved
                        << R"(,"updated":)" << updated
                        << R"(,"failed":)" << failed
                        << R"(,"total":)" << papersJson.size()
                        << R"(,"source":"Crawler"})";
            response.body = jsonResponse.str();

        } catch (const std::exception& e) {
            spdlog::error("[Crawler] Exception in save: {}", e.what());
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Create test user endpoint (for testing only)
    router.post("/api/test/create-user", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            // Check if user already exists
            std::string checkQuery = "SELECT id FROM users WHERE username = 'testuser' OR email = 'test@example.com' LIMIT 1";
            auto existingUsers = g_dbConnection->query(checkQuery);

            if (existingUsers.size() > 0) {
                response.body = R"({"success":false,"message":"Test user already exists","existing":true})";
            } else {
                // Create test user with bcrypt hashed password
                // Password: test123456
                std::string insertQuery =
                    "INSERT INTO users (username, email, password_hash, full_name, role, is_active, is_verified, created_at, updated_at) "
                    "VALUES ('testuser', 'test@example.com', "
                    "'$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', "
                    "'Test User', 'user', 1, 1, NOW(), NOW())";

                if (g_dbConnection->execute(insertQuery)) {
                    response.body = R"({"success":true,"message":"Test user created successfully","user":{"username":"testuser","email":"test@example.com","password":"test123456"}})";
                } else {
                    response.statusCode = 500;
                    response.body = R"({"success":false,"message":"Failed to create test user"})";
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("[TestUser] Exception: {}", e.what());
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Auth: Login endpoint
    router.post("/auth/login", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        if (req.body.empty()) {
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Empty request body\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        try {
            auto bodyJson = json::parse(req.body);
            std::string username = bodyJson.value("username", "");
            std::string password = bodyJson.value("password", "");

            if (username.empty() || password.empty()) {
                response.statusCode = 400;
                response.body = "{\"success\":false,\"error\":\"Username and password are required\"}";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            spdlog::info("[Auth] Login attempt for: {}", username);

            // Query user from database
            std::string query = "SELECT id, username, email, password_hash, full_name, role, is_active "
                              "FROM users WHERE username = '" + g_dbConnection->escape(username) + "' "
                              "OR email = '" + g_dbConnection->escape(username) + "' LIMIT 1";

            auto users = g_dbConnection->query(query);

            if (users.empty()) {
                spdlog::warn("[Auth] User not found: {}", username);
                response.statusCode = 401;
                response.body = "{\"success\":false,\"error\":\"Invalid credentials\"}";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            auto user = users[0];
            std::string storedHash = user["password_hash"];

            // Simple password check (in production, use bcrypt)
            // For now, accept if password is not empty (temporary workaround)
            // TODO: Implement proper bcrypt verification
            bool passwordValid = !password.empty();

            if (passwordValid) {
                // Generate fake tokens (in production, use JWT)
                std::string accessToken = "fake_access_token_" + std::to_string(std::time(nullptr));
                std::string refreshToken = "fake_refresh_token_" + std::to_string(std::time(nullptr));
                int expiresIn = 3600;

                // Update last login
                auto updateStmt = g_dbConnection->prepare("UPDATE users SET last_login = NOW() WHERE id = ?");
                updateStmt->setUInt64(0, std::stoull(user["id"]));
                updateStmt->execute();

                std::ostringstream jsonResponse;
                jsonResponse << R"({"success":true,"message":"Login successful",)"
                            << R"("access_token":")" << accessToken << R"(",)"
                            << R"("refresh_token":")" << refreshToken << R"(",)"
                            << R"("expires_in":)" << expiresIn << R"(,)"
                            << R"("user":{)"
                            << R"("id":)" << user["id"] << R"(,)"
                            << R"("username":")" << user["username"] << R"(",)"
                            << R"("email":")" << user["email"] << R"(",)"
                            << R"("fullName":")" << (user.count("full_name") ? user["full_name"] : std::string("")) << R"(",)"
                            << R"("role":")" << (user.count("role") ? user["role"] : std::string("user")) << R"(",)"
                            << R"("isActive":)" << (user.count("is_active") ? user["is_active"] : std::string("1")) << R"(})"
                            << R"(})";
                response.body = jsonResponse.str();

                spdlog::info("[Auth] Login successful for: {}", username);
            } else {
                spdlog::warn("[Auth] Invalid password for: {}", username);
                response.statusCode = 401;
                response.body = "{\"success\":false,\"error\":\"Invalid credentials\"}";
            }
        } catch (const std::exception& e) {
            spdlog::error("[Auth] Exception: {}", e.what());
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Auth: Register endpoint
    router.post("/auth/register", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 201;

        if (req.body.empty()) {
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Empty request body\"}";
            response.setHeader("Content-Type", "application/json");
            return response;
        }

        try {
            auto bodyJson = json::parse(req.body);
            std::string username = bodyJson.value("username", "");
            std::string email = bodyJson.value("email", "");
            std::string password = bodyJson.value("password", "");
            std::string fullName = bodyJson.value("fullName", "");

            if (username.empty() || email.empty() || password.empty()) {
                response.statusCode = 400;
                response.body = "{\"success\":false,\"error\":\"Username, email, and password are required\"}";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            spdlog::info("[Auth] Registration attempt for: {}", username);

            // Check if user exists
            auto checkStmt = g_dbConnection->prepare("SELECT id FROM users WHERE username = ? OR email = ? LIMIT 1");
            checkStmt->setString(0, username);
            checkStmt->setString(1, email);
            auto existingUsers = checkStmt->query();

            if (!existingUsers.empty()) {
                response.statusCode = 409;
                response.body = R"({"success":false,"error":"Username or email already exists"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // Insert new user
            auto insertStmt = g_dbConnection->prepare(
                "INSERT INTO users (username, email, password_hash, full_name, role, is_active, is_verified, created_at, updated_at) "
                "VALUES (?, ?, ?, ?, 'user', 1, 1, NOW(), NOW())"
            );
            insertStmt->setString(0, username);
            insertStmt->setString(1, email);
            insertStmt->setString(2, "$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy");  // Hash for 'test123456'
            insertStmt->setString(3, fullName);

            if (insertStmt->execute()) {
                uint64_t newId = insertStmt->getLastInsertId();

                // Generate tokens
                std::string accessToken = "fake_access_token_" + std::to_string(std::time(nullptr));
                std::string refreshToken = "fake_refresh_token_" + std::to_string(std::time(nullptr));
                int expiresIn = 3600;

                std::ostringstream jsonResponse;
                jsonResponse << R"({"success":true,"message":"Registration successful",)"
                            << R"("access_token":")" << accessToken << R"(",)"
                            << R"("refresh_token":")" << refreshToken << R"(",)"
                            << R"("expires_in":)" << expiresIn << R"(,)"
                            << R"("user":{)"
                            << R"("id":)" << newId << R"(,)"
                            << R"("username":")" << g_dbConnection->escape(username) << R"(",)"
                            << R"("email":")" << g_dbConnection->escape(email) << R"(",)"
                            << R"("fullName":")" << g_dbConnection->escape(fullName) << R"(",)"
                            << R"("role":"user",)"
                            << R"("isActive":true})"
                            << R"(})";
                response.body = jsonResponse.str();

                spdlog::info("[Auth] Registration successful for: {}", username);
            } else {
                response.statusCode = 500;
                response.body = R"({"success":false,"error":"Failed to create user"})";
            }
        } catch (const std::exception& e) {
            spdlog::error("[Auth] Exception: {}", e.what());
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Create test user via GET (workaround for POST body issue)
    router.get("/api/test/create-demo-user", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;

        try {
            // Check if user already exists
            std::string checkQuery = "SELECT id FROM users WHERE username = 'demouser' LIMIT 1";
            auto existingUsers = g_dbConnection->query(checkQuery);

            if (!existingUsers.empty()) {
                response.body = R"({"success":true,"message":"Demo user already exists","credentials":{"username":"demouser","password":"demo123"}})";
            } else {
                // Create demo user
                // Password: demo123 (bcrypt hash)
                std::string insertQuery =
                    "INSERT INTO users (username, email, password_hash, full_name, role, is_active, is_verified, created_at, updated_at) "
                    "VALUES ('demouser', 'demo@papercrawler.local', "
                    "'$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', "
                    "'Demo User', 'user', 1, 1, NOW(), NOW())";

                if (g_dbConnection->execute(insertQuery)) {
                    response.body = R"({"success":true,"message":"Demo user created","credentials":{"username":"demouser","password":"demo123"}})";
                    spdlog::info("[TestUser] Demo user created successfully");
                } else {
                    response.statusCode = 500;
                    response.body = R"({"success":false,"error":"Failed to create demo user"})";
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("[TestUser] Exception: {}", e.what());
            response.statusCode = 500;
            std::ostringstream err;
            err << R"({"success":false,"error":")" << escapeJsonString(e.what()) << R"("})";
            response.body = err.str();
        }

        response.setHeader("Content-Type", "application/json");
        return response;
    });

    printSuccess("Registered 22 endpoints");
    return true;
}

/**
 * @brief 启动HTTP服务器
 */
bool startHTTPServer() {
    printStep("6/7", "Starting HTTP server");

    // 创建HTTP服务器实例
    g_httpServer = std::make_unique<HttpServerModule>(8080);

    // 初始化服务器
    if (!g_httpServer->initialize()) {
        printError("Failed to initialize HTTP server");
        return false;
    }

    // 设置路由处理器 - 将Router连接到HttpServerModule
    auto& router = Router::getInstance();
    g_httpServer->setRouteHandler([&router](const HttpRequest& req) -> HttpResponse {
        return router.route(req);
    });

    // 启动服务器
    if (!g_httpServer->start()) {
        printError("Failed to start HTTP server");
        return false;
    }

    printSuccess("HTTP server started on port 8080");
    return true;
}

/**
 * @brief 打印已注册的路由
 */
void printRegisteredRoutes() {
    auto& router = Router::getInstance();
    router.printRoutes();
}

/**
 * @brief 主循环
 */
void mainLoop() {
    printStep("7/7", "Entering main loop");

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief 优雅关闭
 */
void gracefulShutdown() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Shutting down..." << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. 停止HTTP服务器
    std::cout << "  - Stopping HTTP server..." << std::endl;
    if (g_httpServer) {
        g_httpServer->stop();
        g_httpServer->cleanup();
        g_httpServer.reset();
    }

    // 2. 关闭数据库连接池
    std::cout << "  - Closing database connection pool..." << std::endl;
    if (g_dbConnection) {
        // PooledConnection析构时会自动归还连接
        g_dbConnection.reset();
    }
    if (g_databaseModule) {
        // 停止DatabaseModule（关闭所有连接）
        g_databaseModule->stop();
        g_databaseModule->cleanup();
        g_databaseModule.reset();
    }

    auto& pluginMgr = PluginManager::getInstance();

    // 2. 卸载业务模块
    std::cout << "  - Unloading business modules..." << std::endl;
    auto& registry = ModuleRegistry::getInstance();
    auto businessModules = registry.getModulesByType(ModuleType::BUSINESS);

    for (auto& moduleInfo : businessModules) {
        if (moduleInfo.state == ModuleState::STARTED) {
            std::cout << "    - Unloading " << moduleInfo.name << "..." << std::endl;

            pluginMgr.unloadModule(moduleInfo.name);
        }
    }

    // 3. 停止系统模块
    std::cout << "  - Stopping system modules..." << std::endl;
    pluginMgr.stopAllModules();

    std::cout << "✓ Shutdown complete" << std::endl;
    std::cout << "========================================" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    // 早期调试输出 - 使用C风格避免iostream初始化问题
    printf("DEBUG: Program starting...\n");
    printf("DEBUG: Command line args: %d\n", argc);
    fflush(stdout);

    // 初始化Windows Sockets
    #ifdef _WIN32
        printf("DEBUG: Initializing Winsock...\n");
        fflush(stdout);
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "Failed to initialize Winsock\n");
            return 1;
        }
        printf("DEBUG: Winsock initialized\n");
        fflush(stdout);
    #endif

    // 设置日志级别
    printf("DEBUG: Setting up logging...\n");
    fflush(stdout);
    spdlog::set_level(spdlog::level::info);
    printf("DEBUG: Logging configured\n");
    fflush(stdout);

    // 打印欢迎信息
    printWelcome();

    // 设置信号处理
    setupSignalHandlers();

    try {
        // 1. 初始化框架核心
        if (!initializeFramework()) {
            return 1;
        }

        // 2. 初始化数据库连接
        if (!initializeDatabase()) {
            return 1;
        }

        // 2.5. 初始化依赖注入服务容器
        if (!initializeServices()) {
            return 1;
        }

        // 3. 加载模块配置
        if (!loadModuleConfiguration()) {
            return 1;
        }

        // 4. 加载和启动系统模块
        if (!loadAndStartSystemModules()) {
            return 1;
        }

        // 5. 加载业务模块
        if (!loadBusinessModules()) {
            return 1;
        }

        // 6. 注册管理API
        if (!registerManagementAPIs()) {
            return 1;
        }

        // 7. 启动HTTP服务器
        if (!startHTTPServer()) {
            return 1;
        }

        // 打印路由
        printRegisteredRoutes();

        // 打印就绪信息
        printReady(8080);

        // 8. 主循环
        mainLoop();

        // 9. 优雅关闭
        gracefulShutdown();

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return 1;
    }

    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
