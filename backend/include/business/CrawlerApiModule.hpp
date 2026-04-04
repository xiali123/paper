#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "core/HttpTypes.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"
#include "../../core/external/nlohmann/json.hpp"
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
    HttpResponse handleCreateTemplate(const HttpRequest& req);

    /**
     * @brief 列出模板
     * GET /api/crawler/templates
     */
    HttpResponse handleListTemplates(const HttpRequest& req);

    /**
     * @brief 获取模板详情
     * GET /api/crawler/templates/:id
     */
    HttpResponse handleGetTemplate(const HttpRequest& req);

    /**
     * @brief 更新模板
     * PUT /api/crawler/templates/:id
     */
    HttpResponse handleUpdateTemplate(const HttpRequest& req);

    /**
     * @brief 删除模板
     * DELETE /api/crawler/templates/:id
     */
    HttpResponse handleDeleteTemplate(const HttpRequest& req);

    /**
     * @brief 验证模板
     * POST /api/crawler/templates/validate
     */
    HttpResponse handleValidateTemplate(const HttpRequest& req);

    /**
     * @brief 测试模板
     * POST /api/crawler/templates/:id/test
     */
    HttpResponse handleTestTemplate(const HttpRequest& req);

    /**
     * @brief 导出模板
     * GET /api/crawler/templates/:id/export
     */
    HttpResponse handleExportTemplate(const HttpRequest& req);

    /**
     * @brief 导入模板
     * POST /api/crawler/templates/import
     */
    HttpResponse handleImportTemplate(const HttpRequest& req);

    // ========================================================================
    // 任务管理接口
    // ========================================================================

    /**
     * @brief 创建爬取任务
     * POST /api/crawler/tasks
     */
    HttpResponse handleCreateTask(const HttpRequest& req);

    /**
     * @brief 列出任务
     * GET /api/crawler/tasks
     */
    HttpResponse handleListTasks(const HttpRequest& req);

    /**
     * @brief 获取任务详情
     * GET /api/crawler/tasks/:id
     */
    HttpResponse handleGetTask(const HttpRequest& req);

    /**
     * @brief 取消任务
     * DELETE /api/crawler/tasks/:id
     */
    HttpResponse handleCancelTask(const HttpRequest& req);

    /**
     * @brief 重试任务
     * POST /api/crawler/tasks/:id/retry
     */
    HttpResponse handleRetryTask(const HttpRequest& req);

    /**
     * @brief 获取任务日志
     * GET /api/crawler/tasks/:id/logs
     */
    HttpResponse handleGetTaskLogs(const HttpRequest& req);

    /**
     * @brief 获取任务统计
     * GET /api/crawler/tasks/statistics
     */
    HttpResponse handleGetTaskStatistics(const HttpRequest& req);

    // ========================================================================
    // 定时任务接口
    // ========================================================================

    /**
     * @brief 创建定时任务
     * POST /api/crawler/schedules
     */
    HttpResponse handleCreateSchedule(const HttpRequest& req);

    /**
     * @brief 列出定时任务
     * GET /api/crawler/schedules
     */
    HttpResponse handleListSchedules(const HttpRequest& req);

    /**
     * @brief 更新定时任务
     * PUT /api/crawler/schedules/:id
     */
    HttpResponse handleUpdateSchedule(const HttpRequest& req);

    /**
     * @brief 删除定时任务
     * DELETE /api/crawler/schedules/:id
     */
    HttpResponse handleDeleteSchedule(const HttpRequest& req);

    /**
     * @brief 启用定时任务
     * POST /api/crawler/schedules/:id/enable
     */
    HttpResponse handleEnableSchedule(const HttpRequest& req);

    /**
     * @brief 禁用定时任务
     * POST /api/crawler/schedules/:id/disable
     */
    HttpResponse handleDisableSchedule(const HttpRequest& req);

    /**
     * @brief 手动触发定时任务
     * POST /api/crawler/schedules/:id/trigger
     */
    HttpResponse handleTriggerSchedule(const HttpRequest& req);

    // ========================================================================
    // 工作节点接口
    // ========================================================================

    /**
     * @brief 列出工作节点
     * GET /api/crawler/workers
     */
    HttpResponse handleListWorkers(const HttpRequest& req);

    /**
     * @brief 获取工作节点详情
     * GET /api/crawler/workers/:id
     */
    HttpResponse handleGetWorker(const HttpRequest& req);

    /**
     * @brief 禁用工作节点
     * POST /api/crawler/workers/:id/disable
     */
    HttpResponse handleDisableWorker(const HttpRequest& req);

    /**
     * @brief 获取节点统计
     * GET /api/crawler/workers/:id/statistics
     */
    HttpResponse handleGetWorkerStatistics(const HttpRequest& req);

    // ========================================================================
    // 系统统计接口
    // ========================================================================

    /**
     * @brief 获取系统仪表盘数据
     * GET /api/crawler/dashboard
     */
    HttpResponse handleGetDashboard(const HttpRequest& req);

    /**
     * @brief 获取系统统计
     * GET /api/crawler/statistics
     */
    HttpResponse handleGetStatistics(const HttpRequest& req);

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
    HttpResponse buildJsonResponse(
        bool success,
        const std::string& message = "",
        const nlohmann::json& data = nullptr
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
