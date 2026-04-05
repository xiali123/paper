#include "modules/DistributedTaskModule.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include "data/IDatabase.hpp"
#include "data/DatabaseModule.hpp"
#include "data/PreparedStatement.hpp"
using DataPreparedStatement = PaperCrawler::PreparedStatement;
#include "modules/CrawlerModule.hpp"
#include "features/LoggingModule.hpp"
#include "spdlog/spdlog.h"
#include <sstream>
#include <queue>
#include <algorithm>
#include <thread>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// Implementation Class
// ============================================================================

class DistributedTaskModule::Impl {
public:
    // 工作节点存储
    std::map<std::string, WorkerNode> workers_;
    std::mutex workersMutex_;

    // 任务队列（优先级队列）
    struct TaskQueueComparator {
        bool operator()(const TaskQueueItem& a, const TaskQueueItem& b) const {
            return a.priority < b.priority; // 优先级高的先出队
        }
    };

    std::priority_queue<
        TaskQueueItem,
        std::vector<TaskQueueItem>,
        TaskQueueComparator
    > taskQueue_;

    std::mutex queueMutex_;
    std::condition_variable queueCondition_;

    // 任务存储
    std::map<std::string, TaskQueueItem> activeTasks_;
    std::mutex tasksMutex_;

    // 线程
    std::thread assignmentThread_;
    std::thread heartbeatThread_;
    std::atomic<bool> running_{false};

    // 负载均衡策略
    LoadBalancingStrategy loadBalancingStrategy_ = LoadBalancingStrategy::LEAST_CONNECTIONS;
};

// ============================================================================
// Constructor and Destructor
// ============================================================================

DistributedTaskModule::DistributedTaskModule(
    std::shared_ptr<IDatabase> database,
    std::shared_ptr<WebSocketModule> websocket)
    : database_(database), websocket_(websocket), impl_(std::make_unique<Impl>()) {

}

DistributedTaskModule::~DistributedTaskModule() {
    stop();
}

// ============================================================================
// IModule Implementation
// ============================================================================

bool DistributedTaskModule::initialize() {
    auto logger = spdlog::get("DistributedTask");
    if (logger) {
        logger->info("Initializing DistributedTaskModule...");
    }

    // 从数据库加载现有工作节点
    loadWorkersFromDatabase();

    // 从数据库加载现有任务
    loadTasksFromDatabase();

    return true;
}

bool DistributedTaskModule::start() {
    auto logger = spdlog::get("DistributedTask");
    if (logger) {
        logger->info("Starting DistributedTaskModule...");
    }

    impl_->running_ = true;

    // 启动任务分配线程
    impl_->assignmentThread_ = std::thread(&DistributedTaskModule::assignmentLoop, this);

    // 启动心跳检测线程
    impl_->heartbeatThread_ = std::thread(&DistributedTaskModule::heartbeatLoop, this);

    if (logger) {
        logger->info("DistributedTaskModule started successfully");
    }

    return true;
}

bool DistributedTaskModule::stop() {
    auto logger = spdlog::get("DistributedTask");
    if (logger) {
        logger->info("Stopping DistributedTaskModule...");
    }

    impl_->running_ = false;

    // 唤醒所有等待的线程
    impl_->queueCondition_.notify_all();

    // 等待线程结束
    if (impl_->assignmentThread_.joinable()) {
        impl_->assignmentThread_.join();
    }

    if (impl_->heartbeatThread_.joinable()) {
        impl_->heartbeatThread_.join();
    }

    if (logger) {
        logger->info("DistributedTaskModule stopped");
    }

    return true;
}

void DistributedTaskModule::cleanup() {
    // 清理资源
    std::lock_guard<std::mutex> lock1(impl_->workersMutex_);
    std::lock_guard<std::mutex> lock2(impl_->tasksMutex_);

    impl_->workers_.clear();
    impl_->activeTasks_.clear();
}

// ============================================================================
// Worker Management
// ============================================================================

bool DistributedTaskModule::registerWorker(const WorkerNode& worker) {
    try {
        // 保存到数据库
        PreparedStatement stmt(database_,
            "INSERT INTO crawler_workers "
            "(node_id, user_id, node_type, ip_address, location, capabilities, "
            "max_concurrent_tasks, current_tasks, status, first_seen) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 0, 'ONLINE', NOW()) "
            "ON DUPLICATE KEY UPDATE "
            "status = 'ONLINE', last_heartbeat = NOW()"
        );

        stmt.bind(1, worker.nodeId);
        stmt.bind(2, worker.userId);
        stmt.bind(3, static_cast<int>(worker.type));
        stmt.bind(4, worker.ipAddress);
        stmt.bind(5, worker.location);

        // Convert supportedTemplateTypes vector to JSON array string
        std::ostringstream capsJson;
        capsJson << "[";
        for (size_t i = 0; i < worker.supportedTemplateTypes.size(); ++i) {
            if (i > 0) capsJson << ",";
            capsJson << "\"" << worker.supportedTemplateTypes[i] << "\"";
        }
        capsJson << "]";
        stmt.bind(6, capsJson.str());

        stmt.bind(7, worker.maxConcurrentTasks);

        if (stmt.execute()) {
            // 更新内存
            {
                std::lock_guard<std::mutex> lock(impl_->workersMutex_);
                impl_->workers_[worker.nodeId] = worker;
            }

            auto logger = spdlog::get("DistributedTask");
            if (logger) {
                logger->info("Worker registered: " + worker.nodeId);
            }

            // 发送确认消息给工作节点
            sendRegistrationConfirmation(worker.nodeId);

            return true;
        }

    } catch (const std::exception& e) {
        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->error("Failed to register worker: " + std::string(e.what()));
        }
    }

    return false;
}

bool DistributedTaskModule::unregisterWorker(const std::string& nodeId) {
    try {
        PreparedStatement stmt(database_,
            "UPDATE crawler_workers SET status = 'OFFLINE' WHERE node_id = ?"
        );

        stmt.bind(1, nodeId);

        if (stmt.execute()) {
            // 从内存中移除
            {
                std::lock_guard<std::mutex> lock(impl_->workersMutex_);
                impl_->workers_.erase(nodeId);
            }

            auto logger = spdlog::get("DistributedTask");
            if (logger) {
                logger->info("Worker unregistered: " + nodeId);
            }

            return true;
        }

    } catch (const std::exception& e) {
        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->error("Failed to unregister worker: " + std::string(e.what()));
        }
    }

    return false;
}

bool DistributedTaskModule::updateWorkerHeartbeat(
    const std::string& nodeId,
    int currentTasks,
    NodeStatus status) {

    try {
        PreparedStatement stmt(database_,
            "UPDATE crawler_workers "
            "SET current_tasks = ?, status = ?, last_heartbeat = NOW(), last_seen = NOW() "
            "WHERE node_id = ?"
        );

        stmt.bind(1, currentTasks);
        stmt.bind(2, static_cast<int>(status));
        stmt.bind(3, nodeId);

        if (stmt.execute()) {
            // 更新内存中的节点信息
            {
                std::lock_guard<std::mutex> lock(impl_->workersMutex_);
                auto it = impl_->workers_.find(nodeId);
                if (it != impl_->workers_.end()) {
                    it->second.currentTasks = currentTasks;
                    it->second.status = status;
                }
            }

            return true;
        }

    } catch (const std::exception& e) {
        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->error("Failed to update worker heartbeat: " + std::string(e.what()));
        }
    }

    return false;
}

std::optional<WorkerNode> DistributedTaskModule::getWorker(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(impl_->workersMutex_);

    auto it = impl_->workers_.find(nodeId);
    if (it != impl_->workers_.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::vector<WorkerNode> DistributedTaskModule::listWorkers(NodeStatus statusFilter) {
    std::vector<WorkerNode> workers;

    std::lock_guard<std::mutex> lock(impl_->workersMutex_);

    for (const auto& [nodeId, worker] : impl_->workers_) {
        if (statusFilter == NodeStatus::ONLINE || worker.status == statusFilter) {
            workers.push_back(worker);
        }
    }

    return workers;
}

std::optional<std::string> DistributedTaskModule::selectBestWorker(
    const std::string& templateId,
    LoadBalancingStrategy strategy) {

    std::vector<WorkerNode> availableWorkers;

    // 获取在线且未过载的工作节点
    {
        std::lock_guard<std::mutex> lock(impl_->workersMutex_);

        for (const auto& [nodeId, worker] : impl_->workers_) {
            if (worker.status == NodeStatus::ONLINE &&
                worker.currentTasks < worker.maxConcurrentTasks) {

                // 检查节点是否支持该模板类型
                // TODO: 检查capabilities中的supportedTemplateTypes

                availableWorkers.push_back(worker);
            }
        }
    }

    if (availableWorkers.empty()) {
        return std::nullopt;
    }

    // 根据策略选择最佳节点
    WorkerNode* selectedWorker = nullptr;

    switch (strategy) {
        case LoadBalancingStrategy::ROUND_ROBIN:
            // 简单轮询
            selectedWorker = &availableWorkers[0];
            break;

        case LoadBalancingStrategy::LEAST_CONNECTIONS:
            // 选择当前任务数最少的节点
            selectedWorker = &*std::min_element(availableWorkers.begin(), availableWorkers.end(),
                [](const WorkerNode& a, const WorkerNode& b) {
                    return a.currentTasks < b.currentTasks;
                });
            break;

        case LoadBalancingStrategy::WEIGHTED_RESPONSE:
            // 根据响应时间加权选择
            selectedWorker = &*std::min_element(availableWorkers.begin(), availableWorkers.end(),
                [](const WorkerNode& a, const WorkerNode& b) {
                    return a.avgResponseTime < b.avgResponseTime;
                });
            break;

        case LoadBalancingStrategy::CAPABILITY_BASED:
            // 根据节点能力匹配选择
            // TODO: 实现基于能力的匹配算法
            selectedWorker = &availableWorkers[0];
            break;
    }

    if (selectedWorker) {
        return selectedWorker->nodeId;
    }

    return std::nullopt;
}

// ============================================================================
// Task Management
// ============================================================================

std::string DistributedTaskModule::createTask(
    const std::string& templateId,
    const std::map<std::string, std::string>& parameters,
    TaskPriority priority) {

    // 生成任务ID
    std::string taskId = "task_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    // 创建任务对象
    TaskQueueItem taskItem;
    taskItem.taskId = taskId;
    taskItem.templateId = templateId;
    taskItem.parameters = parameters;
    taskItem.priority = priority;
    taskItem.scheduledAt = std::chrono::system_clock::now();

    // 保存到数据库
    if (saveTaskToDatabase(taskId, templateId, parameters, priority)) {
        // 添加到任务队列
        {
            std::lock_guard<std::mutex> lock(impl_->queueMutex_);
            impl_->taskQueue_.push(taskItem);
            impl_->activeTasks_[taskId] = taskItem;
        }

        // 通知分配线程
        impl_->queueCondition_.notify_one();

        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->info("Task created: " + taskId);
        }

        return taskId;
    }

    return "";
}

bool DistributedTaskModule::assignTask(
    const std::string& taskId,
    const std::string& workerNodeId) {

    // 获取任务详情
    std::map<std::string, std::string> taskInfo;
    {
        std::lock_guard<std::mutex> lock(impl_->tasksMutex_);
        auto it = impl_->activeTasks_.find(taskId);
        if (it == impl_->activeTasks_.end()) {
            return false;
        }

        taskInfo["templateId"] = it->second.templateId;
        taskInfo["taskId"] = it->second.taskId;
    }

    // 更新数据库
    PreparedStatement stmt(database_,
        "UPDATE distributed_crawl_tasks "
        "SET status = 'ASSIGNED', assigned_to = ?, started_at = NOW() "
        "WHERE task_id = ?"
    );

    stmt.bind(1, workerNodeId);
    stmt.bind(2, taskId);

    if (stmt.execute()) {
        // 更新节点任务计数
        PreparedStatement updateStmt(database_,
            "UPDATE crawler_workers SET current_tasks = current_tasks + 1 "
            "WHERE node_id = ?"
        );

        updateStmt.bind(1, workerNodeId);
        updateStmt.execute();

        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->info("Task assigned: " + taskId + " to worker: " + workerNodeId);
        }

        return true;
    }

    return false;
}

std::optional<std::string> DistributedTaskModule::autoAssignTask(const std::string& taskId) {
    // 获取任务信息
    std::string templateId;
    {
        std::lock_guard<std::mutex> lock(impl_->tasksMutex_);
        auto it = impl_->activeTasks_.find(taskId);
        if (it == impl_->activeTasks_.end()) {
            return std::nullopt;
        }
        templateId = it->second.templateId;
    }

    // 选择最佳工作节点
    auto workerNodeId = selectBestWorker(templateId, impl_->loadBalancingStrategy_);
    if (workerNodeId.has_value()) {
        if (assignTask(taskId, workerNodeId.value())) {
            return workerNodeId;
        }
    }

    return std::nullopt;
}

bool DistributedTaskModule::completeTask(
    const std::string& taskId,
    const std::vector<CrawledPaper>& results,
    const std::string& errorMsg) {

    // 更新数据库
    PreparedStatement stmt(database_,
        "UPDATE distributed_crawl_tasks "
        "SET status = ?, completed_at = NOW(), "
        "papers_found = ?, papers_added = ?, "
        "error_message = ? "
        "WHERE task_id = ?"
    );

    stmt.bind(1, errorMsg.empty() ? "COMPLETED" : "FAILED");
    stmt.bind(2, static_cast<int>(results.size()));
    stmt.bind(3, static_cast<int>(results.size())); // TODO: 区分新增和更新
    stmt.bind(4, errorMsg);
    stmt.bind(5, taskId);

    if (stmt.execute()) {
        // 从活动任务中移除
        {
            std::lock_guard<std::mutex> lock(impl_->tasksMutex_);
            impl_->activeTasks_.erase(taskId);
        }

        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->info("Task completed: " + taskId + " with " +
                std::to_string(results.size()) + " papers");
        }

        return true;
    }

    return false;
}

// ============================================================================
// Load Balancing
// ============================================================================

void DistributedTaskModule::setLoadBalancingStrategy(LoadBalancingStrategy strategy) {
    impl_->loadBalancingStrategy_ = strategy;
}

std::map<std::string, double> DistributedTaskModule::getSystemLoad() {
    std::map<std::string, double> metrics;

    {
        std::lock_guard<std::mutex> lock(impl_->workersMutex_);

        int totalCapacity = 0;
        int usedCapacity = 0;

        for (const auto& [nodeId, worker] : impl_->workers_) {
            if (worker.status == NodeStatus::ONLINE) {
                totalCapacity += worker.maxConcurrentTasks;
                usedCapacity += worker.currentTasks;
            }
        }

        metrics["total_capacity"] = totalCapacity;
        metrics["used_capacity"] = usedCapacity;
        metrics["utilization_rate"] = totalCapacity > 0 ?
            static_cast<double>(usedCapacity) / totalCapacity : 0.0;
    }

    {
        std::lock_guard<std::mutex> lock(impl_->queueMutex_);
        metrics["pending_tasks"] = impl_->taskQueue_.size();
    }

    {
        std::lock_guard<std::mutex> lock(impl_->tasksMutex_);
        metrics["active_tasks"] = impl_->activeTasks_.size();
    }

    return metrics;
}

std::map<std::string, uint64_t> DistributedTaskModule::getClusterStatistics() {
    std::map<std::string, uint64_t> stats;

    {
        std::lock_guard<std::mutex> lock(impl_->workersMutex_);
        stats["online_workers"] = std::count_if(impl_->workers_.begin(), impl_->workers_.end(),
            [](const auto& p) { return p.second.status == NodeStatus::ONLINE; });
        stats["offline_workers"] = std::count_if(impl_->workers_.begin(), impl_->workers_.end(),
            [](const auto& p) { return p.second.status == NodeStatus::OFFLINE; });
    }

    {
        std::lock_guard<std::mutex> lock(impl_->queueMutex_);
        stats["pending_tasks"] = impl_->taskQueue_.size();
    }

    {
        std::lock_guard<std::mutex> lock(impl_->tasksMutex_);
        stats["active_tasks"] = impl_->activeTasks_.size();
    }

    return stats;
}

// ============================================================================
// WebSocket Communication
// ============================================================================

bool DistributedTaskModule::sendTaskToWorker(
    const std::string& workerNodeId,
    const std::string& taskId,
    const CrawlerTemplate& tmpl,
    const std::map<std::string, std::string>& params) {

    if (!websocket_) {
        return false;
    }

    // 构建任务消息
    std::ostringstream message;
    message << "{";
    message << "\"type\":\"task_assigned\",";
    message << "\"taskId\":\"" << taskId << "\",";
    message << "\"task\":{";
    message << "\"template\":" << tmpl.toJson() << ",";
    message << "\"url\":\"" << buildUrl(tmpl, params) << "\",";
    message << "\"method\":\"" << tmpl.method << "\",";
    message << "\"timeout\":" << tmpl.timeout;
    message << "}}";

    // 通过WebSocket发送消息
    // TODO: websocket_->send(workerNodeId, message.str());

    return true;
}

void DistributedTaskModule::handleWorkerResult(
    const std::string& taskId,
    const std::vector<CrawledPaper>& results) {

    completeTask(taskId, results, "");
}

// ============================================================================
// Internal Methods
// ============================================================================

void DistributedTaskModule::assignmentLoop() {
    auto logger = spdlog::get("DistributedTask");

    while (impl_->running_) {
        TaskQueueItem taskItem;

        // 从队列中获取任务
        {
            std::unique_lock<std::mutex> lock(impl_->queueMutex_);

            // 等待任务或停止信号
            impl_->queueCondition_.wait(lock, [this] {
                return !impl_->taskQueue_.empty() || !impl_->running_;
            });

            if (!impl_->running_) break;

            if (!impl_->taskQueue_.empty()) {
                taskItem = impl_->taskQueue_.top();
                impl_->taskQueue_.pop();
            } else {
                continue; // 队列为空，继续等待
            }
        }

        // 自动分配任务
        if (autoAssignTask(taskItem.taskId)) {
            if (logger) {
                logger->debug("Task auto-assigned: " + taskItem.taskId);
            }
        } else {
            if (logger) {
                logger->warn("Failed to assign task: " + taskItem.taskId);
            }

            // 重新入队（稍后重试）
            {
                std::lock_guard<std::mutex> lock(impl_->queueMutex_);
                impl_->taskQueue_.push(taskItem);
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void DistributedTaskModule::heartbeatLoop() {
    auto logger = spdlog::get("DistributedTask");

    while (impl_->running_) {
        // 检测超时的工作节点
        checkTimeoutTasks();

        // 重新分配失败任务
        reassignFailedTasks();

        // 每30秒检查一次
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void DistributedTaskModule::checkTimeoutTasks() {
    // TODO: 检测超过60秒未发送心跳的节点
    // 标记为OFFLINE
}

void DistributedTaskModule::reassignFailedTasks() {
    // TODO: 检查状态为FAILED的任务
    // 重新加入队列
}

bool DistributedTaskModule::saveTaskToDatabase(
    const std::string& taskId,
    const std::string& templateId,
    const std::map<std::string, std::string>& parameters,
    TaskPriority priority) {

    try {
        PreparedStatement stmt(database_,
            "INSERT INTO distributed_crawl_tasks "
            "(task_id, template_id, task_type, priority, parameters, status) "
            "VALUES (?, ?, 'SINGLE_PAPER', ?, 'PENDING')"
        );

        stmt.bind(1, taskId);
        stmt.bind(2, templateId);
        stmt.bind(3, static_cast<int>(priority));
        stmt.bind(4, "{TODO: convert to JSON}"); // TODO: 参数转JSON

        return stmt.execute();

    } catch (const std::exception& e) {
        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->error("Failed to save task to database: " + std::string(e.what()));
        }
    }

    return false;
}

void DistributedTaskModule::loadWorkersFromDatabase() {
    try {
        // 使用IDatabase直接执行查询（替代QueryBuilder）
        auto rows = database_->query(
            "SELECT node_id, user_id, node_type, ip_address, "
            "max_concurrent_tasks, current_tasks, status "
            "FROM crawler_workers "
            "WHERE status != 'DISABLED'"
        );

        for (const auto& row : rows) {
            WorkerNode worker;
            worker.nodeId = row.at("node_id");
            worker.userId = std::stoi(row.at("user_id"));
            worker.type = static_cast<NodeType>(std::stoi(row.at("node_type")));
            worker.ipAddress = row.at("ip_address");
            worker.maxConcurrentTasks = std::stoi(row.at("max_concurrent_tasks"));
            worker.currentTasks = std::stoi(row.at("current_tasks"));
            worker.status = static_cast<NodeStatus>(std::stoi(row.at("status")));

            impl_->workers_[worker.nodeId] = worker;
        }

        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->info("Loaded " + std::to_string(impl_->workers_.size()) + " workers from database");
        }

    } catch (const std::exception& e) {
        auto logger = spdlog::get("DistributedTask");
        if (logger) {
            logger->error("Failed to load workers from database: " + std::string(e.what()));
        }
    }
}

void DistributedTaskModule::loadTasksFromDatabase() {
    // TODO: 从数据库加载PENDING和ASSIGNED状态的任务
    // 重新加入队列
}

std::string DistributedTaskModule::buildUrl(
    const CrawlerTemplate& tmpl,
    const std::map<std::string, std::string>& params) {

    std::string url = tmpl.baseUrl;

    // 替换URL模板参数
    for (const auto& [key, value] : params) {
        std::string placeholder = "{" + key + "}";
        size_t pos = url.find(placeholder);
        while (pos != std::string::npos) {
            url.replace(pos, placeholder.length(), value);
            pos = url.find(placeholder, pos + value.length());
        }
    }

    return url;
}

void DistributedTaskModule::sendRegistrationConfirmation(const std::string& nodeId) {
    if (!websocket_) return;

    std::ostringstream message;
    message << "{";
    message << "\"type\":\"worker_registered\",";
    message << "\"nodeId\":\"" << nodeId << "\",";
    message << "\"assignment\":{";
    message << "\"maxConcurrentTasks\":5";
    message << "}}";

    // TODO: websocket_->send(nodeId, message.str());
}

void DistributedTaskModule::log(
    const std::string& taskId,
    const std::string& level,
    const std::string& message) {

    auto logger = spdlog::get("DistributedTask");
    if (logger) {
        std::string logMsg = "Task[" + taskId + "]: " + message;

        if (level == "ERROR") {
            logger->error(logMsg);
        } else if (level == "WARN") {
            logger->warn(logMsg);
        } else if (level == "INFO") {
            logger->info(logMsg);
        } else {
            logger->debug(logMsg);
        }
    }
}

} // namespace PaperCrawler
