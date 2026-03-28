# PaperCrawler 模块导航指南

## 📚 快速导航

### 按功能分类查找模块

**我想找...**
- 📖 论文管理 → [PaperApiModule](#业务层-6个模块)
- 🔐 用户认证 → [AuthApiModule](#业务层-6个模块)
- 📊 系统统计 → [StatsApiModule](#业务层-6个模块)
- 👥 用户管理 → [UserApiModule](#业务层-6个模块)
- 🔍 高级搜索 → [SearchApiModule](#业务层-6个模块)
- 📤 数据导出 → [ExportApiModule](#业务层-6个模块)
- 🗄️ 数据库访问 → [DatabaseModule](#数据层-3个模块)
- 💾 缓存管理 → [CacheModule](#数据层-3个模块)
- 📁 文件存储 → [FileStorageModule](#数据层-3个模块)
- 🌐 HTTP服务 → [HttpServerModule](#网络层-2个模块)
- 🔌 WebSocket → [WebSocketModule](#网络层-2个模块)
- 🔒 安全加密 → [SecurityModule](#安全层-2个模块)
- 🔑 会话管理 → [SessionModule](#安全层-2个模块)
- ⚡ 性能优化 → [性能层-4个模块](#性能层-4个模块)
- 📝 日志监控 → [基础设施层-3个模块](#基础设施层-3个模块)
- 🔄 弹性容错 → [弹性层-2个模块](#弹性层-2个模块)
- ⏰ 任务调度 → [SchedulerModule](#运维层-8个模块)

---

## 🎯 模块分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    业务层 (6个模块)                          │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐  │
│  │ 论文API   │ 认证API   │ 统计API   │ 用户API   │ 搜索API   │  │
│  │ 导出API   │          │          │          │          │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    功能层 (14个模块)                         │
│  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐   │
│  │ 性能(4) │ 安全(2) │ 基础(3) │ 弹性(2) │ 运维(8)     │   │
│  └─────────┴─────────┴─────────┴─────────┴─────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    核心层 (9个模块)                          │
│  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐   │
│  │ 网络(2) │ 数据(3) │ 系统(8) │ 通信(3) │ 框架核心     │   │
│  └─────────┴─────────┴─────────┴─────────┴─────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 1️⃣ 业务层 (6个模块)

> **功能**: 提供业务API接口，处理具体业务逻辑

| 模块 | 功能 | API数量 | 路由前缀 | 状态 |
|------|------|--------|----------|------|
| [PaperApiModule](#paperapimodule) | 论文管理 | 12 | `/api/papers` | ✅ |
| [AuthApiModule](#authapimodule) | 用户认证 | 8 | `/api/auth` | ✅ |
| [StatsApiModule](#statsapimodule) | 系统统计 | 7 | `/api/stats` | ✅ |
| [UserApiModule](#userapimodule) | 用户管理 | 12 | `/api/users` | ✅ |
| [SearchApiModule](#searchapimodule) | 高级搜索 | 13 | `/api/search` | ✅ |
| [ExportApiModule](#exportapimodule) | 数据导出 | 12 | `/api/export` | ✅ |

### 📄 PaperApiModule - 论文管理
- **文件**: `src/business/PaperApiModule.cpp`
- **功能**: 论文CRUD、搜索、统计、导入导出、收藏标签
- **关键API**:
  - `GET /api/papers` - 论文列表
  - `POST /api/papers` - 创建论文
  - `GET /api/papers/:id` - 论文详情
  - `GET /api/papers/search` - 搜索论文

### 🔐 AuthApiModule - 用户认证
- **文件**: `src/business/AuthApiModule.cpp`
- **功能**: 登录、注册、JWT令牌、密码重置
- **关键API**:
  - `POST /api/auth/login` - 用户登录
  - `POST /api/auth/register` - 用户注册
  - `POST /api/auth/refresh` - 刷新令牌

### 📊 StatsApiModule - 系统统计
- **文件**: `src/business/StatsApiModule.cpp`
- **功能**: 系统资源监控、性能指标、业务统计
- **关键API**:
  - `GET /api/stats/system` - 系统资源
  - `GET /api/stats/performance` - 性能指标

### 👥 UserApiModule - 用户管理
- **文件**: `src/business/UserApiModule.cpp`
- **功能**: 用户CRUD、角色权限、密码管理
- **关键API**:
  - `GET /api/users` - 用户列表
  - `POST /api/users` - 创建用户
  - `PUT /api/users/:id` - 更新用户

### 🔍 SearchApiModule - 高级搜索
- **文件**: `src/business/SearchApiModule.cpp`
- **功能**: 全文搜索、高级过滤、搜索建议
- **关键API**:
  - `GET /api/search` - 基础搜索
  - `POST /api/search/advanced` - 高级搜索
  - `GET /api/search/suggest` - 搜索建议

### 📤 ExportApiModule - 数据导出
- **文件**: `src/business/ExportApiModule.cpp`
- **功能**: 多格式导出、异步任务、批量导出
- **关键API**:
  - `POST /api/export` - 创建导出任务
  - `GET /api/export/:taskId/download` - 下载文件

---

## 2️⃣ 功能层

### 🚀 性能层 (4个模块)

> **用途**: 优化系统性能，提升吞吐量

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [MultiLevelCacheModule](#multilevelcachemodule) | 多级缓存 | L1/L2/L3/L4缓存，命中率>95% |
| [CompressionModule](#compressionmodule) | 数据压缩 | Gzip/Brotli/Zstd，压缩率>70% |
| [AsyncTaskModule](#asynctaskmodule) | 异步任务 | 非阻塞处理，任务队列 |
| [ZeroCopyModule](#zerocopymodule) | 零拷贝 | 减少内存拷贝90% |

**使用场景**:
- 需要缓存热点数据 → MultiLevelCacheModule
- 需要减少网络传输 → CompressionModule
- 需要后台处理耗时任务 → AsyncTaskModule
- 需要高性能数据传输 → ZeroCopyModule

### 🔒 安全层 (2个模块)

> **用途**: 保障系统安全，保护数据和通信

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [SecurityModule](#securitymodule) | 安全加密 | JWT、bcrypt、AES-256-GCM |
| [SessionModule](#sessionmodule) | 会话管理 | 分布式会话、TTL过期 |

**使用场景**:
- 需要用户认证 → SecurityModule
- 需要会话管理 → SessionModule

### 🏗️ 基础设施层 (3个模块)

> **用途**: 提供日志、监控、配置等基础服务

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [LoggingModule](#loggingmodule) | 结构化日志 | 异步写入、日志轮转、JSON格式 |
| [MetricsModule](#metricsmodule) | 指标监控 | Prometheus导出、直方图统计 |
| [ConfigModule](#configmodule) | 配置管理 | 热加载、Schema验证 |

**使用场景**:
- 需要记录日志 → LoggingModule
- 需要性能监控 → MetricsModule
- 需要动态配置 → ConfigModule

### 🛡️ 弹性层 (2个模块)

> **用途**: 提高系统弹性，防止级联故障

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [CircuitBreakerModule](#circuitbreakermodule) | 熔断器 | CLOSED/OPEN/HALF_OPEN状态 |
| [EventBusModule](#eventbusmodule) | 事件总线 | 发布-订阅、事件持久化 |

**使用场景**:
- 防止故障扩散 → CircuitBreakerModule
- 模块间解耦通信 → EventBusModule

### ⚙️ 运维层 (8个模块)

> **用途**: 提供运维所需的各类工具和服务

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [ValidationModule](#validationmodule) | 请求验证 | JSON Schema验证 |
| [NotificationModule](#notificationmodule) | 多渠道通知 | 邮件/短信/推送 |
| [APIDocumentationModule](#apidocumentationmodule) | API文档 | Swagger/OpenAPI |
| [BackupModule](#backupmodule) | 自动备份 | 数据库/文件备份 |
| [ProxyModule](#proxymodule) | 反向代理 | 负载均衡、健康检查 |
| [SchedulerModule](#schedulermodule) | 任务调度 | Cron表达式 |
| [WatchdogModule](#watchdogmodule) | 看门狗监控 | 自动恢复 |
| [Router](#router) | 路由管理 | 请求路由和分发 |

**使用场景**:
- 验证输入数据 → ValidationModule
- 发送通知 → NotificationModule
- 生成API文档 → APIDocumentationModule
- 定时任务 → SchedulerModule

---

## 3️⃣ 核心层

### 🌐 网络层 (2个模块)

> **用途**: 处理网络通信，提供HTTP和WebSocket服务

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [HttpServerModule](#httpservermodule) | HTTP服务器 | 跨平台、多线程 |
| [WebSocketModule](#websocketmodule) | WebSocket服务 | 实时双向通信 |

### 💾 数据层 (3个模块)

> **用途**: 数据持久化和缓存

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [DatabaseModule](#databasemodule) | 数据库访问 | MySQL连接池、事务 |
| [CacheModule](#cachemodule) | 缓存访问 | Redis缓存、TTL |
| [FileStorageModule](#filestoragemodule) | 文件存储 | 抽象文件操作 |

### ⚙️ 系统层 (8个模块)

> **用途**: 提供系统核心功能

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [PoolModule](#poolmodule) | 资源池管理 | 内存池、线程池、对象池 |
| [FilterModule](#filtermodule) | 过滤器链 | 请求过滤 |
| [QueueModule](#queuemodule) | 请求队列 | 优先级队列 |
| [ResponseQueueModule](#responsequeuemodule) | 响应队列 | 响应排序 |
| [ResponseHandlerModule](#responsehandlermodule) | 响应处理 | 格式化响应 |
| [Router](#router) | 路由器 | 请求路由 |
| [PluginManager](#pluginmanager) | 插件管理 | 动态加载/卸载 |
| [MessageBus](#messagebus) | 消息总线 | 模块间通信 |

### 📡 通信层 (3个模块)

> **用途**: 模块间通信协议

| 模块 | 功能 | 关键特性 |
|------|------|----------|
| [UnifiedMessageProtocol](#unifiedmessage) | 统一消息协议 | 标准化消息格式 |
| [MessagePool](#messagepool) | 消息池 | 预分配消息对象 |
| [PoolCoordinator](#poolcoordinator) | 池协调器 | 三池联动 |

---

## 📂 目录结构导航

```
backend/
├── 📂 include/          # 头文件目录
│   ├── 📄 framework/    # 框架接口 (7个文件)
│   │   ├── IModule.hpp
│   │   ├── ModuleExports.hpp
│   │   ├── Router.hpp
│   │   ├── PluginManager.hpp
│   │   └── ...
│   │
│   ├── 📄 communication/# 通信层 (3个)
│   ├── 📄 data/          # 数据层 (3个)
│   ├── 📄 modules/       # 系统层 (8个)
│   │   ├── system/
│   │   ├── performance/
│   │   ├── security/
│   │   └── ...
│   │
│   └── 📄 business/      # 业务层 (6个) ⭐
│       ├── PaperApiModule.hpp
│       ├── AuthApiModule.hpp
│       ├── StatsApiModule.hpp
│       ├── UserApiModule.hpp
│       ├── SearchApiModule.hpp
│       └── ExportApiModule.hpp
│
├── 📂 src/              # 源文件目录
│   ├── main.cpp          # 主程序入口
│   ├── server/           # 服务器核心
│   ├── framework/        # 框架实现
│   ├── communication/    # 通信实现
│   ├── data/            # 数据层实现
│   ├── modules/         # 系统模块实现
│   └── business/        # 业务层实现 (6个) ⭐
│       ├── PaperApiModule.cpp
│       ├── AuthApiModule.cpp
│       ├── StatsApiModule.cpp
│       ├── UserApiModule.cpp
│       ├── SearchApiModule.cpp
│       └── ExportApiModule.cpp
│
├── 📂 config/           # 配置文件
│   └── modules.json     # 模块配置
│
└── 📄 CMakeLists.txt    # 构建配置
```

---

## 🔍 常见问题速查

### Q: 如何添加新的业务API模块？
**A**: 参考 `UserApiModule` 的实现模式：
1. 在 `include/business/` 创建头文件
2. 在 `src/business/` 创建实现文件
3. 继承 `IModule` 接口
4. 在 `CMakeLists.txt` 添加到 `BUSINESS_SOURCES`

### Q: 如何使用数据库？
**A**: 通过 `DatabaseModule` 访问MySQL：
```cpp
#include "data/DatabaseModule.hpp"
auto& db = DatabaseModule::getInstance();
db.executeQuery(sql);
```

### Q: 如何使用缓存？
**A**: 通过 `CacheModule` 访问Redis：
```cpp
#include "data/CacheModule.hpp"
auto& cache = CacheModule::getInstance();
cache.set(key, value, ttl);
```

### Q: 如何添加日志？
**A**: 使用 `LoggingModule`：
```cpp
#include "system/LoggingModule.hpp"
auto& logger = LoggingModule::getInstance();
logger.info("MyModule", "Operation completed");
```

### Q: 如何处理认证？
**A**: 使用 `SecurityModule` 生成JWT：
```cpp
#include "security/SecurityModule.hpp"
auto& security = SecurityModule::getInstance();
auto token = security.generateJWT(claims);
```

---

## 📖 深入阅读

- [业务模块完成报告](./BUSINESS_MODULES_COMPLETE.md)
- [最终架构总结](./FINAL_ARCHITECTURE_SUMMARY.md)
- [模块化架构状态](./MODULAR_ARCHITECTURE_STATUS.md)

---

**最后更新**: 2026-03-29
**版本**: v1.0.0
**模块总数**: 29个
**完成度**: 100% ✅
