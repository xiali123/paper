# PaperCrawler 后端架构分层图

## 1. 总体架构分层

```
┌─────────────────────────────────────────────────────────────────┐
│                         客户端层 (Client)                         │
│  Web前端 | 移动端API | 第三方集成                                 │
└────────────────────────────┬────────────────────────────────────┘
                             │ HTTP/WebSocket
┌────────────────────────────┴────────────────────────────────────┐
│                      网关层 (API Gateway)                        │
│                    ApiGatewayModule                              │
│           (路由 | 鉴权 | 限流 | 监控)                             │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────┴────────────────────────────────────┐
│                    业务逻辑层 (Business)                         │
├─────────────────────────────────────────────────────────────────┤
│ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐             │
│ │ PaperApi     │ │  AuthApi     │ │  UserApi     │             │
│ │  Module      │ │  Module      │ │  Module      │             │
│ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘             │
│        │                │                │                       │
│ ┌──────┴───────┐ ┌──────┴───────┐ ┌──────┴───────┐             │
│ │CrawlerApi    │ │ SearchApi    │ │  StatsApi    │             │
│ │  Module      │ │  Module      │ │  Module      │             │
│ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘             │
│        │                │                │                       │
│ ┌──────┴────────────────┴────────────────┴───────┐             │
│ │        ServiceLayer (服务编排层)                 │             │
│ │   (业务流程编排 | 事务管理 | 数据聚合)            │             │
│ └──────────────────────┬──────────────────────────┘             │
└─────────────────────────┼────────────────────────────────────────┘
                          │
┌─────────────────────────┴────────────────────────────────────────┐
│                  业务抽象层 (Abstract)                            │
│              BusinessModuleBase | ServerModuleBase               │
│                 (路由注册 | 中间件 | 认证)                        │
└─────────────────────────┬────────────────────────────────────────┘
                          │ 事件总线 (EventBus)
┌─────────────────────────┴────────────────────────────────────────┐
│                   中间件层 (Middleware)                           │
├─────────────────────────────────────────────────────────────────┤
│ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │
│ │ Security│ │ Session │ │Logging  │ │ Metrics │ │  Cache  │   │
│ │ Module  │ │ Module  │ │ Module  │ │ Module  │ │ Module  │   │
│ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘   │
│      │           │           │           │           │          │
│ ┌────┴────┐ ┌────┴────┐ ┌────┴────┐ ┌────┴────┐ ┌────┴────┐   │
│ │Circuit  │ │Scheduler│ │Validation│ │Compression││   Async  │   │
│ │Breaker  │ │ Module  │ │ Module  │ │  Module  │ │  Task   │   │
│ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘   │
└─────┼──────────┼──────────┼──────────┼──────────┼──────────┘   │
      │          │          │          │          │                │
┌─────┴──────────┴──────────┴──────────┴──────────┴────────────────┐
│                  数据访问层 (Data Access)                         │
├─────────────────────────────────────────────────────────────────┤
│ ┌──────────────────┐          ┌──────────────────┐              │
│ │  DatabaseModule  │          │   CacheModule    │              │
│ │  (连接池管理)     │          │  (Redis+内存)     │              │
│ └────────┬─────────┘          └────────┬─────────┘              │
│          │                             │                         │
│ ┌────────┴─────────┐          ┌────────┴─────────┐              │
│ │ MySqlConnection │          │ RedisConnection  │              │
│ └──────────────────┘          └──────────────────┘              │
└──────────────────────────────────────────────────────────────────┘
      │
┌─────┴────────────────────────────────────────────────────────────┐
│                  核心基础设施层 (Core)                            │
├─────────────────────────────────────────────────────────────────┤
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│ │ IModule  │ │  Router  │ │EventBus  │ │Service   │           │
│ │  (接口)  │ │ (路由器)  │ │ (事件总线)│ │Container │           │
│ └──────────┘ └──────────┘ └──────────┘ └──────────┘           │
│                                                                     │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│ │ModuleBase│ │HttpTypes │ │ThreadPool│ │ Message  │           │
│ │ (基类)   │ │ (HTTP类型)│ │ (线程池)  │ │  Bus     │           │
│ └──────────┘ └──────────┘ └──────────┘ └──────────┘           │
└──────────────────────────────────────────────────────────────────┘
      │
┌─────┴────────────────────────────────────────────────────────────┐
│                   网络层 (Network)                               │
├─────────────────────────────────────────────────────────────────┤
│ ┌──────────────────┐          ┌──────────────────┐              │
│ │ HttpServerModule │          │ WebSocketModule  │              │
│ │  (HTTP服务器)     │          │ (WebSocket服务器) │              │
│ └──────────────────┘          └──────────────────┘              │
│                                                                     │
│ ┌──────────────────┐          ┌──────────────────┐              │
│ │   HttpClient     │          │AsyncHttpClient   │              │
│ │  (HTTP客户端)     │          │ (异步HTTP客户端)  │              │
│ └──────────────────┘          └──────────────────┘              │
└──────────────────────────────────────────────────────────────────┘
      │
┌─────┴────────────────────────────────────────────────────────────┐
│                   模块层 (Modules)                               │
├─────────────────────────────────────────────────────────────────┤
│ ┌──────────────────┐          ┌──────────────────┐              │
│ │TemplateCrawler   │          │DistributedTask   │              │
│ │    Module        │          │    Module        │              │
│ │  (模板爬虫)       │          │  (分布式任务)     │              │
│ └──────────────────┘          └──────────────────┘              │
│                                                                     │
│ ┌──────────────────┐                                           │
│ │  CrawlerModule   │                                           │
│ │   (爬虫基类)      │                                           │
│ └──────────────────┘                                           │
└──────────────────────────────────────────────────────────────────┘
```

---

## 2. 依赖关系图

### 核心层 (Core - 零依赖)

```
IM oudle
  ↓
ModuleBase
  ↓
ServiceContainer
  ↓
Router
  ↓
EventBus
```

### 网络层 (Network - 依赖核心)

```
HttpServerModule ──→ IM oudle
WebSocketModule ──→ IM oudle
HttpClient (零依赖)
```

### 数据层 (Data - 依赖核心)

```
IDatabase (接口)
  ↓
DatabaseModule ──→ ModuleBase + IDatabase
CacheModule ──→ IM oudle
MySqlConnection ──→ IConnection (建议)
RedisConnection ──→ IConnection (建议)
```

### 中间件层 (Middleware - 依赖核心)

```
ConfigModule ──→ IM oudle
LoggingModule ──→ IM oudle
MetricsModule ──→ IM oudle
SessionModule ──→ IM oudle
SecurityModule ──→ IM oudle
CompressionModule ──→ IM oudle
AsyncTaskModule ──→ IM oudle
CircuitBreakerModule ──→ IM oudle
SchedulerModule ──→ IM oudle
ValidationModule ──→ IM oudle
ProxyModule ──→ IM oudle
```

### 业务层 (Business - 依赖所有层)

```
PaperApiModule ──→ ModuleBase + IDatabase ✅
AuthApiModule ──→ ModuleBase + IDatabase ✅
UserApiModule ──→ ModuleBase + IDatabase ✅
SearchApiModule ──→ ModuleBase ✅
CrawlerApiModule ──→ ModuleBase + ICrawler + IDistributedTask ❌ (需修复)
AiApiModule ──→ IM oudle ✅
RecommendationApiModule ──→ IM oudle ✅
ExportApiModule ──→ IM oudle ✅
StatsApiModule ──→ IM oudle + ModuleRegistry ⚠️ (建议优化)
AnalyticsIntelligenceModule ──→ ModuleBase ✅
AiCoPilotModule ──→ ModuleBase ✅
CollaborativeWritingModule ──→ ModuleBase + IWebSocket ❌ (需修复)
UnifiedAIWorkflow ──→ IM oudle + ICache ❌ (需修复)
ServiceLayer ──→ EventBus ✅ (建议使用事件)
```

**图例**:
- `→` 依赖关系
- `✅` 合理依赖
- `❌` 需要解耦的强依赖
- `⚠️` 建议优化的依赖

---

## 3. 可复用组件提取建议

### 100%可复用 - 可提取为独立库

#### PaperCrawler-Core
```
├─ IM oudle, ModuleBase
├─ ServiceContainer
├─ Router, EventBus
└─ HttpTypes, ModuleExports
```

#### PaperCrawler-Network
```
├─ HttpServerModule
├─ WebSocketModule
└─ HttpClient
```

#### PaperCrawler-Data
```
├─ IDatabase, ICache, IConnection (接口)
├─ DatabaseModule, CacheModule
└─ MySqlConnection, RedisConnection
```

#### PaperCrawler-Middleware
```
├─ LoggingModule, MetricsModule
├─ SecurityModule, CompressionModule
├─ CircuitBreakerModule, SchedulerModule
└─ ValidationModule, ProxyModule
```

### 项目特定 - 保留在项目中

#### PaperCrawler-Business
```
├─ PaperApiModule, AuthApiModule, UserApiModule
├─ SearchApiModule, CrawlerApiModule
├─ AiApiModule, RecommendationApiModule
├─ AnalyticsIntelligenceModule, AiCoPilotModule
├─ CollaborativeWritingModule
├─ TemplateCrawlerModule
└─ ServiceLayer
```

---

## 4. 架构健康度评分

### 当前状态 (重构前)

**总分: 70/100**

| 指标 | 分数 | 说明 |
|------|------|------|
| 分层设计 | 85/100 | 良好的分层结构 |
| 模块化 | 80/100 | 模块划分清晰 |
| 可复用性 | 75/100 | 部分组件可复用 |
| 耦合度 | 50/100 | 存在强耦合 |
| 可测试性 | 60/100 | 依赖注入不充分 |
| 可维护性 | 70/100 | 需要改进 |

### 目标状态 (重构后)

**总分: 90/100**

| 指标 | 分数 | 说明 |
|------|------|------|
| 分层设计 | 95/100 | 严格执行分层 |
| 模块化 | 95/100 | 高度模块化 |
| 可复用性 | 95/100 | 大量可复用组件 |
| 耦合度 | 90/100 | 松耦合设计 |
| 可测试性 | 95/100 | 完整依赖注入 |
| 可维护性 | 90/100 | 易于维护和扩展 |

---

## 5. 关键指标

### 模块统计

| 层级 | 模块数 | 可复用性 |
|------|--------|----------|
| 核心层 | 10个 | 100%可复用 |
| 网络层 | 4个 | 80%可复用 |
| 数据层 | 8个 | 75%可复用 |
| 中间件层 | 15个 | 85%可复用 |
| 业务层 | 15个 | 0%可复用 |
| 模块层 | 3个 | 30%可复用 |

### 耦合点统计

- **强耦合点**: 7个 (必须修复)
  - P0级: 4个 (立即修复)
  - P1级: 3个 (短期修复)
- **循环依赖风险**: 2个 (必须消除)
- **合理依赖**: 50+个 (保持不变)

### 预期收益

| 指标 | 提升幅度 |
|------|----------|
| 新功能开发速度 | +30% |
| Bug修复时间 | -50% |
| 系统可用性 | >99.9% |
| 部署时间 | -60% |
| 代码复用率 | +200% |

---

**文档版本**: 1.0
**创建时间**: 2026-04-03
**维护者**: PaperCrawler架构团队
