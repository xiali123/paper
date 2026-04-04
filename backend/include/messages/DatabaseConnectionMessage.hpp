#pragma once

#include "core/ModuleMessage.hpp"
#include "data/IDatabase.hpp"
#include <memory>
#include <string>

namespace PaperCrawler {
namespace Messages {

/**
 * @brief 数据库连接可用消息
 *
 * 用于DatabaseModule通知业务模块数据库连接已就绪
 * 业务模块订阅此消息来获取数据库连接
 */
class DatabaseConnectionMessage : public ModuleMessage {
public:
    /**
     * @brief 构造数据库连接消息
     * @param connection 数据库连接智能指针
     * @param success 是否成功
     * @param errorMessage 错误信息
     */
    DatabaseConnectionMessage(
        std::shared_ptr<IDatabase> connection,
        bool success = true,
        const std::string& errorMessage = ""
    ) : ModuleMessage(MessageType::CUSTOM, "DatabaseModule", "AllModules"),
        connection_(connection),
        success_(success),
        errorMessage_(errorMessage) {
        // 将连接指针存储在data中以便传递
        setData("connection", connection);
        setData("success", success);
        setData("errorMessage", errorMessage);
    }

    /**
     * @brief 获取数据库连接
     */
    std::shared_ptr<IDatabase> getConnection() const {
        return connection_;
    }

    /**
     * @brief 是否成功
     */
    bool isSuccess() const {
        return success_;
    }

    /**
     * @brief 获取错误信息
     */
    std::string getErrorMessage() const {
        return errorMessage_;
    }

    /**
     * @brief 静态创建函数（便于使用）
     */
    static std::shared_ptr<DatabaseConnectionMessage> create(
        std::shared_ptr<IDatabase> connection,
        bool success = true,
        const std::string& errorMessage = ""
    ) {
        return std::make_shared<DatabaseConnectionMessage>(connection, success, errorMessage);
    }

private:
    std::shared_ptr<IDatabase> connection_;
    bool success_;
    std::string errorMessage_;
};

} // namespace Messages
} // namespace PaperCrawler
