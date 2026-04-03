#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"
#include <memory>
#include <map>
#include <string>
#include <functional>

namespace PaperCrawler {

// ============================================================================
// 前向声明
// ============================================================================

class IDatabase;
class WebSocketModule;

// ============================================================================
// 爬虫API模块
// ============================================================================

/**
 * @brief 爬虫系统REST API和WebSocket接口
 *
 * 核心功能：
 * 1. 模板管理接口
 * 2. 任务管理接口
 * 3. 定时任务接口
 * 4. 工作节点接口
 * 5. 实时WebSocket通信
 *
 * API端点：
 * - POST   /api/crawler/templates
 * - GET    /api/crawler/templates
 * - POST   /api/crawler/tasks
 * - GET    /api/crawler/tasks
 * - WS     /api/crawler/ws
 *
 * 使用示例：
 * ```cpp
 * CrawlerApiModule apiModule(database);
 * apiModule.setTemplateCrawler(templateCrawler);
 * apiModule.setDistributedTask(distributedTask);
 * apiModule.initialize();
 * ```
 */
class CrawlerApiModule : public BusinessModuleBase {
public:
    // 默认构造函数（用于DLL导出）
    CrawlerApiModule();

    // 构造函数（可注入数据库）
    explicit CrawlerApiModule(std::shared_ptr<IDatabase> database);

    ~CrawlerApiModule() override;

    std::string getName() const override { return "CrawlerApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Crawler system REST API and WebSocket interface";
    }

    // ========================================================================
    // 依赖注入
    // ========================================================================

    /**
     * @brief 设置模板爬虫模块
     */
    void setTemplateCrawler(std::shared_ptr<TemplateCrawlerModule> module);

    /**
     * @brief 设置分布式任务模块
     */
    void setDistributedTask(std::shared_ptr<DistributedTaskModule> module);

    /**
     * @brief 设置WebSocket模块
     */
    void setWebSocket(std::shared_ptr<WebSocketModule> module);

private:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<TemplateCrawlerModule> templateCrawler_;
    std::shared_ptr<DistributedTaskModule> distributedTask_;
    std::shared_ptr<WebSocketModule> websocket_;

    // ========================================================================
    // 路由注册
    // ========================================================================

    void registerRoutes() override;

    // ========================================================================
    // 模板管理接口
    // ========================================================================

    /**
     * @brief 创建模板
     * POST /api/crawler/templates
     */
    std::string handleCreateTemplate(const std::string& body);

    /**
     * @brief 列出模板
     * GET /api/crawler/templates
     */
    std::string handleListTemplates(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取模板详情
     * GET /api/crawler/templates/:id
     */
    std::string handleGetTemplate(const std::map<std::string, std::string>& params);

    /**
     * @brief 更新模板
     * PUT /api/crawler/templates/:id
     */
    std::string handleUpdateTemplate(const std::map<std::string, std::string>& params, const std::string& body);

    /**
     * @brief 删除模板
     * DELETE /api/crawler/templates/:id
     */
    std::string handleDeleteTemplate(const std::map<std::string, std::string>& params);

    /**
     * @brief 验证模板
     * POST /api/crawler/templates/validate
     */
    std::string handleValidateTemplate(const std::string& body);

    /**
     * @brief 测试模板
     * POST /api/crawler/templates/:id/test
     */
    std::string handleTestTemplate(const std::map<std::string, std::string>& params, const std::string& body);

    /**
     * @brief 导出模板
     * GET /api/crawler/templates/:id/export
     */
    std::string handleExportTemplate(const std::map<std::string, std::string>& params);

    /**
     * @brief 导入模板
     * POST /api/crawler/templates/import
     */
    std::string handleImportTemplate(const std::string& body);

    // ========================================================================
    // 任务管理接口
    // ========================================================================

    /**
     * @brief 创建爬取任务
     * POST /api/crawler/tasks
     */
    std::string handleCreateTask(const std::string& body);

    /**
     * @brief 列出任务
     * GET /api/crawler/tasks
     */
    std::string handleListTasks(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取任务详情
     * GET /api/crawler/tasks/:id
     */
    std::string handleGetTask(const std::map<std::string, std::string>& params);

    /**
     * @brief 取消任务
     * DELETE /api/crawler/tasks/:id
     */
    std::string handleCancelTask(const std::map<std::string, std::string>& params);

    /**
     * @brief 重试任务
     * POST /api/crawler/tasks/:id/retry
     */
    std::string handleRetryTask(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取任务日志
     * GET /api/crawler/tasks/:id/logs
     */
    std::string handleGetTaskLogs(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取任务统计
     * GET /api/crawler/tasks/statistics
     */
    std::string handleGetTaskStatistics(const std::map<std::string, std::string>& params);

    // ========================================================================
    // 定时任务接口
    // ========================================================================

    /**
     * @brief 创建定时任务
     * POST /api/crawler/schedules
     */
    std::string handleCreateSchedule(const std::string& body);

    /**
     * @brief 列出定时任务
     * GET /api/crawler/schedules
     */
    std::string handleListSchedules(const std::map<std::string, std::string>& params);

    /**
     * @brief 更新定时任务
     * PUT /api/crawler/schedules/:id
     */
    std::string handleUpdateSchedule(const std::map<std::string, std::string>& params, const std::string& body);

    /**
     * @brief 删除定时任务
     * DELETE /api/crawler/schedules/:id
     */
    std::string handleDeleteSchedule(const std::map<std::string, std::string>& params);

    /**
     * @brief 启用定时任务
     * POST /api/crawler/schedules/:id/enable
     */
    std::string handleEnableSchedule(const std::map<std::string, std::string>& params);

    /**
     * @brief 禁用定时任务
     * POST /api/crawler/schedules/:id/disable
     */
    std::string handleDisableSchedule(const std::map<std::string, std::string>& params);

    /**
     * @brief 手动触发定时任务
     * POST /api/crawler/schedules/:id/trigger
     */
    std::string handleTriggerSchedule(const std::map<std::string, std::string>& params);

    // ========================================================================
    // 工作节点接口
    // ========================================================================

    /**
     * @brief 列出工作节点
     * GET /api/crawler/workers
     */
    std::string handleListWorkers(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取工作节点详情
     * GET /api/crawler/workers/:id
     */
    std::string handleGetWorker(const std::map<std::string, std::string>& params);

    /**
     * @brief 禁用工作节点
     * POST /api/crawler/workers/:id/disable
     */
    std::string handleDisableWorker(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取节点统计
     * GET /api/crawler/workers/:id/statistics
     */
    std::string handleGetWorkerStatistics(const std::map<std::string, std::string>& params);

    // ========================================================================
    // 系统统计接口
    // ========================================================================

    /**
     * @brief 获取系统仪表盘数据
     * GET /api/crawler/dashboard
     */
    std::string handleGetDashboard(const std::map<std::string, std::string>& params);

    /**
     * @brief 获取系统统计
     * GET /api/crawler/statistics
     */
    std::string handleGetStatistics(const std::map<std::string, std::string>& params);

    // ========================================================================
    // WebSocket消息处理
    // ========================================================================

    /**
     * @brief 处理WebSocket消息
     */
    void handleWebSocketMessage(const WebSocketMessage& message);

    /**
     * @brief 处理工作节点注册
     */
    void handleWorkerRegister(const WebSocketMessage& message);

    /**
     * @brief 处理工作节点心跳
     */
    void handleWorkerHeartbeat(const WebSocketMessage& message);

    /**
     * @brief 处理任务结果
     */
    void handleTaskResult(const WebSocketMessage& message);

    /**
     * @brief 处理任务进度更新
     */
    void handleTaskProgress(const WebSocketMessage& message);

    /**
     * @brief 处理错误报告
     */
    void handleErrorReport(const WebSocketMessage& message);

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 构建JSON响应
     */
    std::string buildJsonResponse(
        bool success,
        const std::string& message = "",
        const std::map<std::string, std::string>& data = {}
    );

    /**
     * @brief 解析请求参数
     */
    std::map<std::string, std::string> parseRequestParams(const std::string& url);

    /**
     * @brief 提取路径参数
     */
    std::string extractPathParam(
        const std::string& url,
        const std::string& paramName
    );

    /**
     * @brief 记录API访问日志
     */
    void logApiAccess(
        const std::string& endpoint,
        const std::string& method,
        const std::string& clientIp
    );

    /**
     * @brief 转义JSON字符串
     */
    std::string escapeJson(const std::string& str);
};

} // namespace PaperCrawler
