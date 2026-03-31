#pragma once

#include "data/DatabaseModule.hpp"
#include "database/PreparedStatement.hpp"
#include <memory>
#include <string>

namespace PaperCrawler {

/**
 * @brief 连接池包装器 - 自动归还连接到池
 *
 * 这个类包装了从DatabaseModule连接池获取的连接，
 * 在析构时自动归还连接到池，实现RAII模式。
 *
 * 使用示例：
 * auto pooledConn = databaseModule.getConnection();
 * // 使用连接...
 * // 连接在作用域结束时自动归还
 */
class PooledConnection : public DatabaseConnection {
public:
    /**
     * @brief 构造函数
     * @param connection 从池中获取的实际连接
     * @param returnCallback 归还连接的回调函数
     */
    PooledConnection(std::shared_ptr<DatabaseConnection> connection,
                    std::function<void(std::shared_ptr<DatabaseConnection>)> returnCallback)
        : connection_(connection)
        , returnCallback_(returnCallback)
        , returned_(false) {
    }

    /**
     * @brief 析构函数 - 自动归还连接到池
     */
    ~PooledConnection() override {
        returnConnection();
    }

    // 禁止拷贝
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    // 支持移动
    PooledConnection(PooledConnection&& other) noexcept
        : connection_(std::move(other.connection_))
        , returnCallback_(std::move(other.returnCallback_))
        , returned_(other.returned_) {
        other.returned_ = true;
    }

    PooledConnection& operator=(PooledConnection&& other) noexcept {
        if (this != &other) {
            returnConnection();
            connection_ = std::move(other.connection_);
            returnCallback_ = std::move(other.returnCallback_);
            returned_ = other.returned_;
            other.returned_ = true;
        }
        return *this;
    }

    /**
     * @brief 手动归还连接到池（也可以等待析构自动归还）
     */
    void returnConnection() {
        if (!returned_ && connection_) {
            if (returnCallback_) {
                returnCallback_(connection_);
            }
            returned_ = true;
        }
    }

    // ========== DatabaseConnection接口实现 ==========

    std::vector<std::map<std::string, std::string>> query(const std::string& sql) override {
        ensureConnectionValid();
        return connection_->query(sql);
    }

    bool execute(const std::string& sql) override {
        ensureConnectionValid();
        return connection_->execute(sql);
    }

    bool beginTransaction() override {
        ensureConnectionValid();
        return connection_->beginTransaction();
    }

    bool commitTransaction() override {
        ensureConnectionValid();
        return connection_->commitTransaction();
    }

    bool rollbackTransaction() override {
        ensureConnectionValid();
        return connection_->rollbackTransaction();
    }

    uint64_t getLastInsertId() override {
        ensureConnectionValid();
        return connection_->getLastInsertId();
    }

    size_t getAffectedRows() override {
        ensureConnectionValid();
        return connection_->getAffectedRows();
    }

    std::string escape(const std::string& str) override {
        ensureConnectionValid();
        return connection_->escape(str);
    }

    bool isConnected() override {
        return connection_ && connection_->isConnected();
    }

    void close() override {
        if (connection_) {
            connection_->close();
        }
        // 注意：不归还连接，因为连接已被关闭
        returned_ = true;
    }

    bool ping() override {
        ensureConnectionValid();
        return connection_->ping();
    }

    /**
     * @brief 创建预处理语句（仅支持MySqlConnection）
     */
    std::shared_ptr<PreparedStatement> prepare(const std::string& sql) {
        ensureConnectionValid();

        // 尝试dynamic_cast到MySqlConnection
        auto mysqlConn = std::dynamic_pointer_cast<MySqlConnection>(connection_);
        if (mysqlConn) {
            return mysqlConn->prepare(sql);
        }

        // 如果不是MySQL连接，返回nullptr
        return nullptr;
    }

    /**
     * @brief 获取底层连接（谨慎使用）
     */
    std::shared_ptr<DatabaseConnection> getUnderlyingConnection() const {
        return connection_;
    }

private:
    std::shared_ptr<DatabaseConnection> connection_;
    std::function<void(std::shared_ptr<DatabaseConnection>)> returnCallback_;
    bool returned_;

    void ensureConnectionValid() const {
        if (!connection_) {
            throw std::runtime_error("PooledConnection: null connection");
        }
    }
};

} // namespace PaperCrawler
