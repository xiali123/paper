#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include "communication/UnifiedMessage.hpp"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 数据库模块 - MySQL数据库访问
 *
 * 功能：
 * 1. 连接池管理
 * 2. SQL查询执行
 * 3. 事务支持
 * 4. 统一消息协议接口
 */
class DatabaseModule : public IModule {
public:
    DatabaseModule();
    ~DatabaseModule() override;

    // IModule接口实现
    std::string getName() const override { return "Database"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "MySQL database access module with connection pool";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 执行SQL查询（使用统一消息协议）
     */
    MessageResponse executeQuery(const UnifiedMessage& message);

    /**
     * @brief 事务支持
     */
    bool beginTransaction(const std::string& transactionId);
    bool commitTransaction(const std::string& transactionId);
    bool rollbackTransaction(const std::string& transactionId);

    /**
     * @brief 获取连接池统计
     */
    struct PoolStats {
        size_t totalConnections;
        size_t activeConnections;
        size_t idleConnections;
        size_t waitingRequests;
    };
    PoolStats getPoolStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 数据库配置
    struct DatabaseConfig {
        std::string host{"localhost"};
        int port{3306};
        std::string database{"papercrawler"};
        std::string username{"root"};
        std::string password;
        size_t poolSize{10};
        size_t maxPoolSize{50};
    } config_;

    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
