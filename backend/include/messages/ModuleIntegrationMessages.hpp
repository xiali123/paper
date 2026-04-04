#pragma once

#include "core/ModuleMessage.hpp"
#include <string>
#include <map>
#include <chrono>

namespace PaperCrawler {
namespace Messages {

/**
 * @brief 用户状态变更消息
 *
 * 用于通知其他模块用户状态发生变化
 * 例如：用户登录/登出、账号激活/停用、密码修改等
 */
class UserStatusMessage : public ModuleMessage {
public:
    enum class Status {
        LOGGED_IN,       // 用户登录
        LOGGED_OUT,      // 用户登出
        ACTIVATED,       // 账号激活
        SUSPENDED,       // 账号停用
        PASSWORD_CHANGED, // 密码修改
        PROFILE_UPDATED   // 个人资料更新
    };

    UserStatusMessage(
        int userId,
        Status status,
        const std::string& username = ""
    ) : ModuleMessage(MessageType::CUSTOM, "UserModule", "AllModules"),
        userId_(userId),
        status_(status),
        username_(username),
        timestamp_(std::chrono::system_clock::now()) {

        setData("userId", userId);
        setData("status", static_cast<int>(status));
        setData("username", username);
        setData("timestamp", std::chrono::system_clock::to_time_t(timestamp_));
    }

    int getUserId() const { return userId_; }
    Status getStatus() const { return status_; }
    std::string getUsername() const { return username_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }

    std::string getStatusString() const {
        switch (status_) {
            case Status::LOGGED_IN: return "logged_in";
            case Status::LOGGED_OUT: return "logged_out";
            case Status::ACTIVATED: return "activated";
            case Status::SUSPENDED: return "suspended";
            case Status::PASSWORD_CHANGED: return "password_changed";
            case Status::PROFILE_UPDATED: return "profile_updated";
            default: return "unknown";
        }
    }

    static std::shared_ptr<UserStatusMessage> create(
        int userId,
        Status status,
        const std::string& username = "") {
        return std::make_shared<UserStatusMessage>(userId, status, username);
    }

private:
    int userId_;
    Status status_;
    std::string username_;
    std::chrono::system_clock::time_point timestamp_;
};

/**
 * @brief 论文更新消息
 *
 * 用于通知其他模块论文数据发生变化
 * 例如：新论文创建、论文更新、论文删除、论文被引用等
 */
class PaperUpdateMessage : public ModuleMessage {
public:
    enum class Action {
        CREATED,      // 新论文创建
        UPDATED,      // 论文更新
        DELETED,      // 论文删除
        CITED,        // 论文被引用
        FAVORITED,    // 论文被收藏
        TAG_ADDED,    // 标签添加
        EXPORTED      // 论文导出
    };

    PaperUpdateMessage(
        int paperId,
        Action action,
        int userId = 0,
        const std::string& title = ""
    ) : ModuleMessage(MessageType::CUSTOM, "PaperModule", "AllModules"),
        paperId_(paperId),
        action_(action),
        userId_(userId),
        title_(title),
        timestamp_(std::chrono::system_clock::now()) {

        setData("paperId", paperId);
        setData("action", static_cast<int>(action));
        setData("userId", userId);
        setData("title", title);
        setData("timestamp", std::chrono::system_clock::to_time_t(timestamp_));
    }

    int getPaperId() const { return paperId_; }
    Action getAction() const { return action_; }
    int getUserId() const { return userId_; }
    std::string getTitle() const { return title_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }

    std::string getActionString() const {
        switch (action_) {
            case Action::CREATED: return "created";
            case Action::UPDATED: return "updated";
            case Action::DELETED: return "deleted";
            case Action::CITED: return "cited";
            case Action::FAVORITED: return "favorited";
            case Action::TAG_ADDED: return "tag_added";
            case Action::EXPORTED: return "exported";
            default: return "unknown";
        }
    }

    static std::shared_ptr<PaperUpdateMessage> create(
        int paperId,
        Action action,
        int userId = 0,
        const std::string& title = "") {
        return std::make_shared<PaperUpdateMessage>(paperId, action, userId, title);
    }

private:
    int paperId_;
    Action action_;
    int userId_;
    std::string title_;
    std::chrono::system_clock::time_point timestamp_;
};

/**
 * @brief 爬虫任务状态变更消息
 *
 * 用于通知其他模块爬虫任务状态变化
 * 例如：任务创建、任务完成、任务失败、任务进度更新等
 */
class CrawlerTaskMessage : public ModuleMessage {
public:
    enum class Status {
        CREATED,       // 任务创建
        RUNNING,       // 任务运行中
        COMPLETED,     // 任务完成
        FAILED,        // 任务失败
        CANCELLED,     // 任务取消
        PROGRESS_UPDATED  // 进度更新
    };

    CrawlerTaskMessage(
        const std::string& taskId,
        Status status,
        int progress = 0,
        const std::string& errorMessage = ""
    ) : ModuleMessage(MessageType::CUSTOM, "CrawlerModule", "AllModules"),
        taskId_(taskId),
        status_(status),
        progress_(progress),
        errorMessage_(errorMessage),
        timestamp_(std::chrono::system_clock::now()) {

        setData("taskId", taskId);
        setData("status", static_cast<int>(status));
        setData("progress", progress);
        setData("errorMessage", errorMessage);
        setData("timestamp", std::chrono::system_clock::to_time_t(timestamp_));
    }

    std::string getTaskId() const { return taskId_; }
    Status getStatus() const { return status_; }
    int getProgress() const { return progress_; }
    std::string getErrorMessage() const { return errorMessage_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }

    std::string getStatusString() const {
        switch (status_) {
            case Status::CREATED: return "created";
            case Status::RUNNING: return "running";
            case Status::COMPLETED: return "completed";
            case Status::FAILED: return "failed";
            case Status::CANCELLED: return "cancelled";
            case Status::PROGRESS_UPDATED: return "progress_updated";
            default: return "unknown";
        }
    }

    static std::shared_ptr<CrawlerTaskMessage> create(
        const std::string& taskId,
        Status status,
        int progress = 0,
        const std::string& errorMessage = "") {
        return std::make_shared<CrawlerTaskMessage>(taskId, status, progress, errorMessage);
    }

private:
    std::string taskId_;
    Status status_;
    int progress_;
    std::string errorMessage_;
    std::chrono::system_clock::time_point timestamp_;
};

/**
 * @brief 推荐更新消息
 *
 * 用于通知其他模块推荐结果已更新
 * 例如：新的推荐生成、用户反馈记录等
 */
class RecommendationUpdateMessage : public ModuleMessage {
public:
    enum class Type {
        GENERATED,     // 新推荐生成
        FEEDBACK,      // 用户反馈
        UPDATED        // 推荐更新
    };

    RecommendationUpdateMessage(
        int userId,
        Type type,
        int paperId = 0,
        double score = 0.0
    ) : ModuleMessage(MessageType::CUSTOM, "RecommendationModule", "AllModules"),
        userId_(userId),
        type_(type),
        paperId_(paperId),
        score_(score),
        timestamp_(std::chrono::system_clock::now()) {

        setData("userId", userId);
        setData("type", static_cast<int>(type));
        setData("paperId", paperId);
        setData("score", score);
        setData("timestamp", std::chrono::system_clock::to_time_t(timestamp_));
    }

    int getUserId() const { return userId_; }
    Type getType() const { return type_; }
    int getPaperId() const { return paperId_; }
    double getScore() const { return score_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }

    std::string getTypeString() const {
        switch (type_) {
            case Type::GENERATED: return "generated";
            case Type::FEEDBACK: return "feedback";
            case Type::UPDATED: return "updated";
            default: return "unknown";
        }
    }

    static std::shared_ptr<RecommendationUpdateMessage> create(
        int userId,
        Type type,
        int paperId = 0,
        double score = 0.0) {
        return std::make_shared<RecommendationUpdateMessage>(userId, type, paperId, score);
    }

private:
    int userId_;
    Type type_;
    int paperId_;
    double score_;
    std::chrono::system_clock::time_point timestamp_;
};

} // namespace Messages
} // namespace PaperCrawler
