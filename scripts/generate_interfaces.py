#!/usr/bin/env python3
"""
PaperCrawler 接口生成器
自动创建缺失的抽象接口，支持依赖倒置原则
"""

import os
import sys
from pathlib import Path

# 模板目录
TEMPLATES_DIR = Path(__file__).parent / "templates"
OUTPUT_DIR = Path(__file__).parent.parent / "backend" / "include" / "interfaces"

# 接口定义
INTERFACES = {
    "ICrawler": {
        "header": """#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace PaperCrawler {

// 前向声明
struct CrawlerTemplate;
struct CrawlerTemplateParams;
struct CrawledPaper;

/**
 * @brief 爬虫接口
 *
 * 定义爬虫的抽象能力，支持多种爬虫实现：
 * - TemplateCrawlerModule（模板爬虫）
 * - ApiCrawlerModule（API爬虫）
 * - BrowserCrawlerModule（浏览器爬虫）
 */
class ICrawler {
public:
    virtual ~ICrawler() = default;

    /**
     * @brief 使用模板爬取数据
     * @param templateId 模板ID
     * @param params 爬取参数
     * @return 爬取的论文列表
     */
    virtual std::vector<CrawledPaper> crawlWithTemplate(
        const std::string& templateId,
        const std::map<std::string, std::string>& params
    ) = 0;

    /**
     * @brief 验证模板
     * @param tmpl 模板配置
     * @return 验证结果
     */
    virtual bool validateTemplate(const CrawlerTemplate& tmpl) = 0;

    /**
     * @brief 测试模板
     * @param templateId 模板ID
     * @param params 测试参数
     * @return 测试结果
     */
    virtual std::optional<std::vector<CrawledPaper>> testTemplate(
        const std::string& templateId,
        const std::map<std::string, std::string>& params
    ) = 0;

    /**
     * @brief 获取模板列表
     * @param activeOnly 是否只返回启用的模板
     * @return 模板列表
     */
    virtual std::vector<CrawlerTemplate> listTemplates(bool activeOnly = true) = 0;

    /**
     * @brief 保存模板
     * @param tmpl 模板配置
     * @param createdBy 创建者ID
     * @return 是否成功
     */
    virtual bool saveTemplate(const CrawlerTemplate& tmpl, int createdBy) = 0;

    /**
     * @brief 删除模板
     * @param templateId 模板ID
     * @return 是否成功
     */
    virtual bool deleteTemplate(const std::string& templateId) = 0;
};

} // namespace PaperCrawler
""",
        "dependencies": ["CrawlerModule", "TemplateCrawlerModule"]
    },

    "IDistributedTask": {
        "header": """#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace PaperCrawler {

// 前向声明
struct TaskConfig;
struct TaskResult;
struct TaskProgress;

/**
 * @brief 分布式任务接口
 *
 * 定义分布式任务管理的抽象能力：
 * - 任务创建和管理
 * - 任务调度和执行
 * - 工作节点管理
 * - 任务监控
 */
class IDistributedTask {
public:
    virtual ~IDistributedTask() = default;

    /**
     * @brief 创建任务
     * @param config 任务配置
     * @return 任务ID
     */
    virtual std::string createTask(const TaskConfig& config) = 0;

    /**
     * @brief 取消任务
     * @param taskId 任务ID
     * @return 是否成功
     */
    virtual bool cancelTask(const std::string& taskId) = 0;

    /**
     * @brief 获取任务状态
     * @param taskId 任务ID
     * @return 任务状态
     */
    virtual std::optional<TaskResult> getTaskResult(const std::string& taskId) = 0;

    /**
     * @brief 获取任务进度
     * @param taskId 任务ID
     * @return 任务进度
     */
    virtual std::optional<TaskProgress> getTaskProgress(const std::string& taskId) = 0;

    /**
     * @brief 列出所有任务
     * @param status 任务状态过滤（可选）
     * @return 任务列表
     */
    virtual std::vector<std::string> listTasks(const std::string& status = "") = 0;

    /**
     * @brief 重试失败的任务
     * @param taskId 任务ID
     * @return 是否成功
     */
    virtual bool retryTask(const std::string& taskId) = 0;

    /**
     * @brief 获取工作节点列表
     * @return 工作节点列表
     */
    virtual std::vector<std::string> listWorkers() = 0;

    /**
     * @brief 禁用工作节点
     * @param workerId 工作节点ID
     * @return 是否成功
     */
    virtual bool disableWorker(const std::string& workerId) = 0;
};

} // namespace PaperCrawler
""",
        "dependencies": ["DistributedTaskModule"]
    },

    "IWebSocket": {
        "header": """#pragma once

#include <string>
#include <functional>
#include <memory>

namespace PaperCrawler {

// 前向声明
struct WebSocketMessage;

/**
 * @brief WebSocket消息处理器
 */
using WebSocketMessageHandler = std::function<void(const WebSocketMessage&)>;

/**
 * @brief WebSocket接口
 *
 * 定义WebSocket通信的抽象能力：
 * - 消息发送和接收
 * - 连接管理
 * - 广播和组播
 */
class IWebSocket {
public:
    virtual ~IWebSocket() = default;

    /**
     * @brief 发送消息到客户端
     * @param clientId 客户端ID
     * @param message 消息内容
     * @return 是否成功
     */
    virtual bool sendToClient(const std::string& clientId, const std::string& message) = 0;

    /**
     * @brief 广播消息到所有客户端
     * @param message 消息内容
     * @return 发送的客户端数量
     */
    virtual size_t broadcast(const std::string& message) = 0;

    /**
     * @brief 发送消息到指定组
     * @param group 组名
     * @param message 消息内容
     * @return 发送的客户端数量
     */
    virtual size_t sendToGroup(const std::string& group, const std::string& message) = 0;

    /**
     * @brief 注册消息处理器
     * @param messageType 消息类型
     * @param handler 处理器函数
     */
    virtual void registerHandler(const std::string& messageType, WebSocketMessageHandler handler) = 0;

    /**
     * @brief 获取连接的客户端列表
     * @return 客户端ID列表
     */
    virtual std::vector<std::string> getConnectedClients() = 0;

    /**
     * @brief 断开客户端连接
     * @param clientId 客户端ID
     * @return 是否成功
     */
    virtual bool disconnectClient(const std::string& clientId) = 0;

    /**
     * @brief 检查客户端是否在线
     * @param clientId 客户端ID
     * @return 是否在线
     */
    virtual bool isClientConnected(const std::string& clientId) = 0;
};

} // namespace PaperCrawler
""",
        "dependencies": ["WebSocketModule"]
    },

    "ICache": {
        "header": """#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 缓存接口
 *
 * 定义缓存操作的抽象能力：
 * - SET/GET/DELETE操作
 * - TTL管理
 * - 批量操作
 * - 支持多种缓存实现（Redis, Memcached, 内存缓存）
 */
class ICache {
public:
    virtual ~ICache() = default;

    /**
     * @brief SET操作
     * @param key 键
     * @param value 值
     * @param ttl 过期时间（0表示永不过期）
     * @return 是否成功
     */
    virtual bool set(const std::string& key, const std::string& value,
                    std::chrono::seconds ttl = std::chrono::seconds(0)) = 0;

    /**
     * @brief GET操作
     * @param key 键
     * @return 值（不存在时返回nullopt）
     */
    virtual std::optional<std::string> get(const std::string& key) = 0;

    /**
     * @brief DELETE操作
     * @param key 键
     * @return 是否成功
     */
    virtual bool del(const std::string& key) = 0;

    /**
     * @brief EXISTS操作
     * @param key 键
     * @return 是否存在
     */
    virtual bool exists(const std::string& key) = 0;

    /**
     * @brief 批量SET（MSET）
     * @param kvs 键值对
     * @return 是否成功
     */
    virtual bool mset(const std::map<std::string, std::string>& kvs) = 0;

    /**
     * @brief 批量GET（MGET）
     * @param keys 键列表
     * @return 键值对映射
     */
    virtual std::map<std::string, std::string> mget(const std::vector<std::string>& keys) = 0;

    /**
     * @brief 设置过期时间
     * @param key 键
     * @param ttl 过期时间
     * @return 是否成功
     */
    virtual bool expire(const std::string& key, std::chrono::seconds ttl) = 0;

    /**
     * @brief 获取剩余过期时间
     * @param key 键
     * @return 剩余时间（不存在时返回nullopt）
     */
    virtual std::optional<std::chrono::seconds> ttl(const std::string& key) = 0;

    /**
     * @brief 自增
     * @param key 键
     * @param delta 增量
     * @return 新值
     */
    virtual int64_t incr(const std::string& key, int64_t delta = 1) = 0;

    /**
     * @brief 自减
     * @param key 键
     * @param delta 减量
     * @return 新值
     */
    virtual int64_t decr(const std::string& key, int64_t delta = 1) = 0;

    /**
     * @brief 获取匹配模式的所有键
     * @param pattern 模式（如 "user:*"）
     * @return 键列表
     */
    virtual std::vector<std::string> keys(const std::string& pattern = "*") = 0;

    /**
     * @brief 清空所有数据
     * @return 是否成功
     */
    virtual bool flushAll() = 0;
};

} // namespace PaperCrawler
""",
        "dependencies": ["CacheModule", "MultiLevelCacheModule"]
    },

    "IConnection": {
        "header": """#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 连接配置
 */
struct ConnectionConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string database;
    int timeoutSeconds{30};
};

/**
 * @brief 查询结果
 */
struct Result {
    bool success;
    std::string errorMessage;
    std::vector<std::map<std::string, std::string>> rows;
    size_t affectedRows{0};
    uint64_t lastInsertId{0};
};

/**
 * @brief 数据库连接接口
 *
 * 定义数据库连接的抽象能力：
 * - 连接管理
 * - 查询执行
 * - 事务支持
 */
class IConnection {
public:
    virtual ~IConnection() = default;

    /**
     * @brief 连接到数据库
     * @param config 连接配置
     * @return 是否成功
     */
    virtual bool connect(const ConnectionConfig& config) = 0;

    /**
     * @brief 断开连接
     * @return 是否成功
     */
    virtual bool disconnect() = 0;

    /**
     * @brief 检查连接是否有效
     * @return 是否已连接
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief 执行查询（返回结果集）
     * @param sql SQL语句
     * @return 查询结果
     */
    virtual Result query(const std::string& sql) = 0;

    /**
     * @brief 执行语句（INSERT, UPDATE, DELETE）
     * @param sql SQL语句
     * @return 执行结果
     */
    virtual Result execute(const std::string& sql) = 0;

    /**
     * @brief 开始事务
     * @return 事务ID
     */
    virtual std::string beginTransaction() = 0;

    /**
     * @brief 提交事务
     * @param transactionId 事务ID
     * @return 是否成功
     */
    virtual bool commitTransaction(const std::string& transactionId) = 0;

    /**
     * @brief 回滚事务
     * @param transactionId 事务ID
     * @return 是否成功
     */
    virtual bool rollbackTransaction(const std::string& transactionId) = 0;

    /**
     * @brief Ping连接（保持活跃）
     * @return 是否成功
     */
    virtual bool ping() = 0;

    /**
     * @brief 转义字符串（防止SQL注入）
     * @param str 原始字符串
     * @return 转义后的字符串
     */
    virtual std::string escape(const std::string& str) = 0;
};

} // namespace PaperCrawler
""",
        "dependencies": ["MySqlConnection", "RedisConnection", "DatabaseModule"]
    }
}


def generate_interface(interface_name: str, interface_def: dict) -> None:
    """生成接口文件"""
    output_file = OUTPUT_DIR / f"{interface_name}.hpp"

    # 创建输出目录
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # 写入接口定义
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(interface_def['header'])

    print(f"✅ 生成接口: {output_file}")


def generate_interfaces_readme() -> None:
    """生成接口目录README"""
    readme_file = OUTPUT_DIR / "README.md"

    content = """# PaperCrawler 抽象接口层

## 概述

本目录包含PaperCrawler系统的所有抽象接口定义，用于实现依赖倒置原则（DIP）。

## 接口列表

### 核心接口

| 接口 | 职责 | 实现模块 |
|------|------|----------|
| `ICrawler` | 爬虫能力抽象 | TemplateCrawlerModule |
| `IDistributedTask` | 分布式任务抽象 | DistributedTaskModule |
| `IWebSocket` | WebSocket通信抽象 | WebSocketModule |
| `ICache` | 缓存操作抽象 | CacheModule, MultiLevelCacheModule |
| `IConnection` | 数据库连接抽象 | MySqlConnection, RedisConnection |

## 使用指南

### 1. 在业务模块中使用接口

```cpp
// ❌ 错误：直接依赖具体实现
#include "modules/TemplateCrawlerModule.hpp"

class CrawlerApiModule {
private:
    std::shared_ptr<TemplateCrawlerModule> crawler_;
};

// ✅ 正确：依赖抽象接口
#include "interfaces/ICrawler.hpp"

class CrawlerApiModule {
private:
    std::shared_ptr<ICrawler> crawler_;
};
```

### 2. 注册服务到依赖注入容器

```cpp
// main.cpp
#include "interfaces/ICrawler.hpp"
#include "modules/TemplateCrawlerModule.hpp"

// 注册实现
Services::registerService<ICrawler, TemplateCrawlerModule>();

// 解析服务
auto crawler = Services::resolve<ICrawler>();
```

### 3. 在单元测试中使用Mock对象

```cpp
// 测试代码
class MockCrawler : public ICrawler {
    std::vector<CrawledPaper> crawlWithTemplate(...) override {
        return {};  // 返回测试数据
    }
};

TEST(CrawlerApiTest, TestCrawl) {
    auto mockCrawler = std::make_shared<MockCrawler>();
    CrawlerApiModule api(nullptr);
    api.setCrawler(mockCrawler);
    // 测试逻辑...
}
```

## 设计原则

1. **依赖倒置原则（DIP）**：高层模块不应依赖低层模块，都应依赖抽象
2. **接口隔离原则（ISP）**：接口应该小而专一，不应强迫实现不必要的方法
3. **开闭原则（OCP）**：对扩展开放，对修改关闭
4. **里氏替换原则（LSP）**：子类必须能够替换父类

## 添加新接口

当需要添加新接口时：

1. 在本目录创建接口文件（如 `INewService.hpp`）
2. 定义纯虚函数接口
3. 在具体模块中实现接口
4. 在 `ServiceContainer` 中注册实现
5. 更新本README文档

## 维护指南

- 接口变更应慎重，避免破坏现有实现
- 新增方法应提供默认实现（如果可能）
- 废弃的接口应保留至少一个版本周期
- 所有接口变更必须更新文档

---

**最后更新**: 2026-04-03
**维护者**: PaperCrawler架构团队
"""

    with open(readme_file, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"✅ 生成README: {readme_file}")


def main():
    """主函数"""
    print("================================================")
    print("PaperCrawler 接口生成器")
    print("================================================")
    print()

    # 生成所有接口
    for interface_name, interface_def in INTERFACES.items():
        generate_interface(interface_name, interface_def)

    print()
    print("================================================")
    print("生成接口文档")
    print("================================================")

    # 生成README
    generate_interfaces_readme()

    print()
    print("================================================")
    print("完成！")
    print("================================================")
    print()
    print(f"接口文件已生成到: {OUTPUT_DIR}")
    print()
    print("下一步:")
    print("1. 检查生成的接口文件")
    print("2. 在具体模块中实现这些接口")
    print("3. 在ServiceContainer中注册实现")
    print("4. 重构业务模块使用接口而非具体实现")
    print()


if __name__ == "__main__":
    main()
