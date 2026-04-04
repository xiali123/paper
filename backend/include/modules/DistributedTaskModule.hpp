#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include "network/WebSocketModule.hpp"
#include "modules/CrawlerModule.hpp"  // 引入 CrawledPaper 等类型
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace PaperCrawler {

// ============================================================================
// 前向声明
// ============================================================================

class IDatabase;
// CrawledPaper 已通过 CrawlerModule.hpp 引入
class CrawlerTemplate;

// ============================================================================
// 节点类型和状态
// ============================================================================

/**
 * @brief 工作节点类型
 */
enum class NodeType {
    BROWSER,    // 浏览器节点
    SERVER,     // 服务器节点
    HYBRID      // 混合节点
};

/**
 * @brief 节点状态
 */
enum class NodeStatus {
    ONLINE,
    OFFLINE,
    DISABLED,
    BUSY
};

/**
 * @brief 任务优先级
 */
enum class TaskPriority {
    LOW,
    NORMAL,
    HIGH,
    URGENT
};

/**
 * @brief 负载均衡策略
 */
enum class LoadBalancingStrategy {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    WEIGHTED_RESPONSE,
    CAPABILITY_BASED
};

// ============================================================================
// 工作节点信息
// ============================================================================

/**
 * @brief 工作节点配置
 */
struct WorkerNode {
    // 基本信息
    std::string nodeId;
    int userId;
    NodeType type;
    NodeStatus status;

    // 能力信息
    int maxConcurrentTasks = 5;
    int currentTasks = 0;
    std::vector<std::string> supportedTemplateTypes;
    bool supportsJsRendering = false;

    // 统计信息
    uint64_t tasksCompleted = 0;
    uint64_t tasksFailed = 0;
    double avgResponseTime = 0.0;
    double successRate = 100.0;

    // 网络信息
    std::string ipAddress;
    std::string userAgent;
    std::string location;

    // 心跳信息
    std::string lastHeartbeat;
    std::string firstSeen;
    std::string lastSeen;

    // 元数据
    std::map<std::string, std::string> metadata;
};

/**
 * @brief 任务分配
 */
struct TaskAssignment {
    std::string taskId;
    std::string workerNodeId;
    TaskPriority priority;
    std::string scheduledAt;
    std::string startedAt;
    std::string deadline;
    std::map<std::string, std::string> parameters;
};

// ============================================================================
// 分布式任务模块
// ============================================================================

/**
 * @brief 分布式任务调度模块
 *
 * 核心功能：
 * 1. 工作节点注册和管理
 * 2. 任务队列和分配
 * 3. 负载均衡
 * 4. 心跳检测和故障恢复
 * 5. 实时进度跟踪
 *
 * 使用示例：
 * ```cpp
 * DistributedTaskModule scheduler(database, websocket);
 * scheduler.registerWorker(workerInfo);
 * scheduler.assignTask(taskId, templateId, parameters);
 * ```
 */
class DistributedTaskModule : public IModule {
public:
    explicit DistributedTaskModule(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<WebSocketModule> websocket
    );
    ~DistributedTaskModule() override;

    // IModule接口实现
    std::string getName() const override { return "DistributedTask"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Distributed task scheduling and worker management";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    // ========================================================================
    // 工作节点管理
    // ========================================================================

    /**
     * @brief 注册工作节点
     */
    bool registerWorker(const WorkerNode& worker);

    /**
     * @brief 注销工作节点
     */
    bool unregisterWorker(const std::string& nodeId);

    /**
     * @brief 更新工作节点心跳
     */
    bool updateWorkerHeartbeat(
        const std::string& nodeId,
        int currentTasks,
        NodeStatus status
    );

    /**
     * @brief 获取工作节点信息
     */
    std::optional<WorkerNode> getWorker(const std::string& nodeId);

    /**
     * @brief 列出所有工作节点
     */
    std::vector<WorkerNode> listWorkers(NodeStatus statusFilter = NodeStatus::ONLINE);

    /**
     * @brief 禁用工作节点
     */
    bool disableWorker(const std::string& nodeId);

    /**
     * @brief 获取最佳工作节点
     */
    std::optional<std::string> selectBestWorker(
        const std::string& templateId,
        LoadBalancingStrategy strategy = LoadBalancingStrategy::LEAST_CONNECTIONS
    );

    // ========================================================================
    // 任务管理
    // ========================================================================

    /**
     * @brief 创建任务
     */
    std::string createTask(
        const std::string& templateId,
        const std::map<std::string, std::string>& parameters,
        TaskPriority priority = TaskPriority::NORMAL
    );

    /**
     * @brief 分配任务
     */
    bool assignTask(
        const std::string& taskId,
        const std::string& workerNodeId
    );

    /**
     * @brief 自动分配任务
     */
    std::optional<std::string> autoAssignTask(const std::string& taskId);

    /**
     * @brief 完成任务
     */
    bool completeTask(
        const std::string& taskId,
        const std::vector<Modules::CrawledPaper>& results,
        const std::string& errorMsg = ""
    );

    /**
     * @brief 重试任务
     */
    bool retryTask(const std::string& taskId);

    /**
     * @brief 取消任务
     */
    bool cancelTask(const std::string& taskId);

    /**
     * @brief 获取任务状态
     */
    std::optional<std::string> getTaskStatus(const std::string& taskId);

    /**
     * @brief 获取队列中的待处理任务
     */
    std::vector<std::string> getPendingTasks(int limit = 100);

    // ========================================================================
    // 负载均衡
    // ========================================================================

    /**
     * @brief 设置负载均衡策略
     */
    void setLoadBalancingStrategy(LoadBalancingStrategy strategy);

    /**
     * @brief 获取系统负载
     */
    std::map<std::string, double> getSystemLoad();

    /**
     * @brief 获取集群统计
     */
    std::map<std::string, uint64_t> getClusterStatistics();

    // ========================================================================
    // WebSocket通信
    // ========================================================================

    /**
     * @brief 发送任务给工作节点
     */
    bool sendTaskToWorker(
        const std::string& workerNodeId,
        const std::string& taskId,
        const CrawlerTemplate& tmpl,
        const std::map<std::string, std::string>& params
    );

    /**
     * @brief 处理工作节点结果
     */
    void handleWorkerResult(
        const std::string& taskId,
        const std::vector<Modules::CrawledPaper>& results
    );

    /**
     * @brief 广播任务取消
     */
    void broadcastTaskCancellation(const std::string& taskId);

    /**
     * @brief 从数据库加载工作节点
     */
    void loadWorkersFromDatabase();

    /**
     * @brief 从数据库加载任务
     */
    void loadTasksFromDatabase();

    /**
     * @brief 发送注册确认给工作节点
     */
    void sendRegistrationConfirmation(const std::string& workerNodeId);

    /**
     * @brief 构建URL（基于模板和参数）
     */
    std::string buildUrl(
        const CrawlerTemplate& tmpl,
        const std::map<std::string, std::string>& params
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<WebSocketModule> websocket_;

    // 任务队列
    struct TaskQueueItem {
        std::string taskId;
        std::string templateId;
        std::map<std::string, std::string> parameters;
        TaskPriority priority;
        std::chrono::system_clock::time_point scheduledAt;
    };

    std::priority_queue<TaskQueueItem> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 任务分配线程
     */
    void assignmentLoop();

    /**
     * @brief 心跳检测线程
     */
    void heartbeatLoop();

    /**
     * @brief 检测超时任务
     */
    void checkTimeoutTasks();

    /**
     * @brief 重新分配失败任务
     */
    void reassignFailedTasks();

    /**
     * @brief 更新节点统计
     */
    void updateWorkerStatistics(const std::string& nodeId, bool success, double responseTime);

    /**
     * @brief 保存任务到数据库
     */
    bool saveTaskToDatabase(
        const std::string& taskId,
        const std::string& templateId,
        const std::map<std::string, std::string>& parameters,
        TaskPriority priority
    );

    /**
     * @brief 更新任务状态
     */
    bool updateTaskStatus(
        const std::string& taskId,
        const std::string& status,
        const std::string& errorMsg = ""
    );

    /**
     * @brief 记录日志
     */
    void log(
        const std::string& taskId,
        const std::string& level,
        const std::string& message
    );
};

} // namespace PaperCrawler
