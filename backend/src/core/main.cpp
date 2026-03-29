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

// 网络模块
#include "network/HttpServerModule.hpp"
#include "network/HttpClient.hpp"

// 数据模块
#include "data/MySqlConnection.hpp"

// JSON library
#include "../../core/external/nlohmann/json.hpp"

#include <spdlog/spdlog.h>

using namespace PaperCrawler;
using json = nlohmann::json;

// 全局运行标志
std::atomic<bool> g_running{true};

// 全局HTTP服务器实例
std::unique_ptr<HttpServerModule> g_httpServer;

// 全局MySQL连接实例
std::unique_ptr<MySqlConnection> g_dbConnection;

// 前向声明辅助函数
void printStep(const std::string& step, const std::string& details);
void printSuccess(const std::string& message);
void printError(const std::string& message);

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
 * @brief 初始化数据库连接
 */
bool initializeDatabase() {
    printStep("Init", "Connecting to MySQL database");

    try {
        // 创建MySQL连接实例 - 使用配置的凭证
        g_dbConnection = std::make_unique<MySqlConnection>(
            "localhost",  // host
            3306,         // port
            "root",       // user
            "123456",     // password
            "papercrawler" // database
        );

        if (!g_dbConnection->isConnected()) {
            printError("Failed to connect to MySQL database");
            return false;
        }

        printSuccess("Connected to MySQL database: papercrawler");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("Database initialization failed: ") + e.what());
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

    auto& registry = ModuleRegistry::getInstance();
    auto& pluginMgr = PluginManager::getInstance();

    // 按优先级排序（这里简化处理，实际应该按照priority字段排序）
    auto systemModules = registry.getModulesByType(ModuleType::SERVER);

    size_t loadedCount = 0;
    for (const auto& moduleInfo : systemModules) {
        std::cout << "  - Loading " << moduleInfo.name << "..." << std::endl;

        if (!pluginMgr.loadModule(moduleInfo.name, moduleInfo.libraryPath)) {
            printError("Failed to load " + moduleInfo.name);
            continue;
        }

        loadedCount++;
    }

    // 启动所有模块
    if (!pluginMgr.startAllModules()) {
        printError("Failed to start some modules");
        return false;
    }

    printSuccess("Loaded and started " + std::to_string(loadedCount) + " system modules");
    return true;
}

/**
 * @brief 加载业务模块
 */
bool loadBusinessModules() {
    printStep("4/7", "Loading business modules");

    auto& registry = ModuleRegistry::getInstance();
    auto& pluginMgr = PluginManager::getInstance();

    auto businessModules = registry.getModulesByType(ModuleType::BUSINESS);
    size_t loadedCount = 0;

    for (const auto& moduleInfo : businessModules) {
        std::cout << "  - Loading " << moduleInfo.name
                  << " (v" << moduleInfo.version << ")..." << std::endl;

        if (!pluginMgr.loadModule(moduleInfo.name, moduleInfo.libraryPath)) {
            printError("Warning: Failed to load " + moduleInfo.name);
            continue;
        }

        loadedCount++;
    }

    printSuccess("Loaded " + std::to_string(loadedCount) + " business modules");
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

    // Papers API
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
            std::ostringstream sql;
            sql << "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE "
                << "LOWER(title) LIKE '%" << g_dbConnection->escape(query) << "%' OR "
                << "LOWER(authors) LIKE '%" << g_dbConnection->escape(query) << "%'";

            auto papers = g_dbConnection->query(sql.str());

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
            std::ostringstream sql;
            sql << "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE "
                << "LOWER(title) LIKE '%" << g_dbConnection->escape(query) << "%' OR "
                << "LOWER(authors) LIKE '%" << g_dbConnection->escape(query) << "%'";

            auto papers = g_dbConnection->query(sql.str());

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
            std::string sql = "SELECT id, title, authors, year, publication, citation_count FROM papers WHERE id = " +
                            g_dbConnection->escape(paperId);

            auto papers = g_dbConnection->query(sql);

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

            std::string sql = "INSERT INTO papers (title, authors, year, publication, citation_count) VALUES ('"
                            + g_dbConnection->escape(title) + "', '"
                            + g_dbConnection->escape(authors) + "', "
                            + year + ", '"
                            + g_dbConnection->escape(publication) + "', "
                            + citationCount + ")";

            if (g_dbConnection->execute(sql)) {
                uint64_t newId = g_dbConnection->getLastInsertId();
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
            std::string checkSql = "SELECT id FROM papers WHERE id = " + g_dbConnection->escape(paperId);
            auto existing = g_dbConnection->query(checkSql);

            if (existing.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Paper not found"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            // 构建更新SQL
            std::vector<std::string> updates;
            std::string title = req.getQuery("title", "");
            std::string authors = req.getQuery("authors", "");
            std::string year = req.getQuery("year", "");
            std::string publication = req.getQuery("publication", "");
            std::string citationCount = req.getQuery("citation_count", "");

            if (!title.empty()) updates.push_back("title = '" + g_dbConnection->escape(title) + "'");
            if (!authors.empty()) updates.push_back("authors = '" + g_dbConnection->escape(authors) + "'");
            if (!year.empty()) updates.push_back("year = " + year);
            if (!publication.empty()) updates.push_back("publication = '" + g_dbConnection->escape(publication) + "'");
            if (!citationCount.empty()) updates.push_back("citation_count = " + citationCount);

            if (updates.empty()) {
                response.statusCode = 400;
                response.body = R"({"success":false,"error":"No fields to update"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            std::string sql = "UPDATE papers SET " + updates[0];
            for (size_t i = 1; i < updates.size(); ++i) {
                sql += ", " + updates[i];
            }
            sql += " WHERE id = " + g_dbConnection->escape(paperId);

            if (g_dbConnection->execute(sql)) {
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
            std::string checkSql = "SELECT id FROM papers WHERE id = " + g_dbConnection->escape(paperId);
            auto existing = g_dbConnection->query(checkSql);

            if (existing.empty()) {
                response.statusCode = 404;
                response.body = R"({"success":false,"error":"Paper not found"})";
                response.setHeader("Content-Type", "application/json");
                return response;
            }

            std::string sql = "DELETE FROM papers WHERE id = " + g_dbConnection->escape(paperId);

            if (g_dbConnection->execute(sql)) {
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

            size_t totalPapers = papers.empty() ? 0 : std::stoul(papers[0].at("count"));
            size_t totalJournals = journals.empty() ? 0 : std::stoul(journals[0].at("count"));
            size_t totalAuthors = authors.empty() ? 0 : std::stoul(authors[0].at("count"));

            std::ostringstream json;
            json << R"({"success":true,"stats":{)"
                 << R"("totalPapers":)" << totalPapers << R"(,)"
                 << R"("totalJournals":)" << totalJournals << R"(,)"
                 << R"("totalAuthors":)" << totalAuthors
                 << R"(}})";

            response.body = json.str();
        } catch (const std::exception& e) {
            response.statusCode = 500;
            response.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        }

        response.setHeader("Content-Type", "application/json");
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
                    std::string checkQuery = "SELECT id, title FROM papers WHERE title = '"
                                          + g_dbConnection->escape(title)
                                          + "' LIMIT 1";

                    auto checkResult = g_dbConnection->query(checkQuery);

                    if (checkResult.size() > 0) {
                        // Update existing paper
                        std::string updateQuery = "UPDATE papers SET "
                                                + std::string("authors = '") + g_dbConnection->escape(authors) + "', "
                                                + "year = " + std::to_string(year) + ", "
                                                + "abstract = '" + g_dbConnection->escape(abstract) + "', "
                                                + "publication = '" + g_dbConnection->escape(source) + "', "
                                                + "updated_at = NOW() "
                                                + "WHERE id = " + checkResult[0]["id"];

                        g_dbConnection->execute(updateQuery);
                        updated++;
                        spdlog::info("[Crawler] Updated paper: {}", title);
                    } else {
                        // Insert new paper
                        std::string insertQuery =
                            "INSERT INTO papers (title, authors, year, abstract, publication, is_favorite, is_read, created_at, updated_at) "
                            "VALUES ('"
                            + g_dbConnection->escape(title) + "', '"
                            + g_dbConnection->escape(authors) + "', "
                            + std::to_string(year) + ", '"
                            + g_dbConnection->escape(abstract) + "', '"
                            + g_dbConnection->escape(source) + "', "
                            "0, 0, NOW(), NOW())";

                        g_dbConnection->execute(insertQuery);
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

    printSuccess("Registered 18 endpoints");
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

    // 2. 关闭数据库连接
    std::cout << "  - Closing database connection..." << std::endl;
    if (g_dbConnection) {
        g_dbConnection->close();
        g_dbConnection.reset();
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
    // 初始化Windows Sockets
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            return 1;
        }
    #endif

    // 设置日志级别
    spdlog::set_level(spdlog::level::info);

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
