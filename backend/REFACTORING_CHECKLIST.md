# main.cpp 重构迁移检查清单

## 使用说明

本检查清单用于跟踪 main.cpp 重构进度。每个阶段开始前，确保完成前置条件的所有检查项。

**检查项状态**:
- [ ] 待完成
- [x] 已完成
- [~] 进行中
- [!] 被阻塞
- [o] 不适用

---

## 阶段 0: 重构前准备

### 0.1 代码审查
- [ ] 完整阅读 main.cpp（3890 行）
- [ ] 识别所有功能模块
- [ ] 统计路由注册数量（37 个）
- [ ] 绘制依赖关系图
- [ ] 识别重复代码
- [ ] 识别硬编码配置
- [ ] 识别安全风险点

### 0.2 环境准备
- [ ] 创建重构分支 `feature/refactor-main-cpp`
- [ ] 设置开发环境（依赖、工具链）
- [ ] 备份当前数据库
- [ ] 记录性能基准（响应时间、资源使用）
- [ ] 记录当前 API 契约（请求/响应格式）

### 0.3 测试准备
- [ ] 创建测试框架结构
- [ ] 编写 API 集成测试骨架
- [ ] 记录所有 API 端点和路径
- [ ] 准备测试数据集
- [ ] 设置测试数据库

### 0.4 文档准备
- [ ] 记录当前系统架构
- [ ] 记录所有 API 路径和功能
- [ ] 记录配置项和默认值
- [ ] 准备重构计划文档
- [ ] 准备风险评估文档

---

## 阶段 1: 准备和基础设施（2-3 天）

### 1.1 配置管理系统
**文件**: `include/core/ConfigManager.hpp`, `src/core/ConfigManager.cpp`

#### 配置管理器实现
- [ ] 创建 `DatabaseConfig` 结构体
- [ ] 实现 `ConfigManager::loadDatabaseConfig()`
- [ ] 实现 `ConfigManager::loadHttpConfig()`
- [ ] 实现 `ConfigManager::loadModuleConfig()`
- [ ] 实现 `ConfigManager::validate()`
- [ ] 实现配置文件热重载（可选）

#### 配置文件创建
- [ ] 创建 `config/database.json`
  ```json
  {
    "host": "127.0.0.1",
    "port": 3306,
    "username": "root",
    "password": "123456",
    "database": "papercrawler_db",
    "poolSize": 10,
    "maxPoolSize": 20,
    "connectTimeoutSeconds": 5,
    "queryTimeoutSeconds": 30,
    "autoReconnect": true
  }
  ```
- [ ] 创建 `config/http.json`
- [ ] 创建 `config/modules.json`
- [ ] 创建 `config/app.json`
- [ ] 创建配置文件示例 `config/examples/`

#### 单元测试
- [ ] 测试配置文件加载
- [ ] 测试配置文件验证
- [ ] 测试配置缺失时的默认值
- [ ] 测试配置文件格式错误处理

#### 集成测试
- [ ] main.cpp 使用配置管理器加载配置
- [ ] 验证数据库连接使用配置文件
- [ ] 验证 HTTP 服务器使用配置文件

---

### 1.2 工具模块
**文件**: `include/utils/StringUtils.hpp`, `src/utils/StringUtils.cpp`

#### 字符串工具函数
- [ ] 迁移 `escapeJsonString()`
- [ ] 迁移 `base64_encode()`
- [ ] 迁移 `base64_decode()`
- [ ] 迁移 `getMapValue()`
- [ ] 添加单元测试

#### CMakeLists.txt 更新
- [ ] 添加 utils 库
- [ ] 链接 utils 到业务模块

#### 单元测试
- [ ] 测试 JSON 字符串转义（各种特殊字符）
- [ ] 测试 Base64 编码/解码（各种长度）
- [ ] 测试边界情况（空字符串、超大字符串）

---

### 1.3 Migration 系统
**文件**: `include/data/DatabaseMigration.hpp`, `src/data/DatabaseMigration.cpp`

#### Migration 类实现
- [ ] 实现 `DatabaseMigration::run()`
- [ ] 实现 `DatabaseMigration::rollback()`
- [ ] 实现 `DatabaseMigration::getStatus()`
- [ ] 实现 Migration 版本管理

#### Migration 脚本创建
- [ ] 创建 `migrations/002_create_users_table.sql`
  ```sql
  CREATE TABLE IF NOT EXISTS users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    name VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_email (email)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
  ```
- [ ] 创建 `migrations/003_create_user_sessions_table.sql`
- [ ] 创建 `migrations/004_create_papers_table.sql`
- [ ] 创建 `migrations/005_create_journals_table.sql`
- [ ] 创建 `migrations/006_create_authors_table.sql`
- [ ] 创建 `migrations/007_create_collections_table.sql`

#### Migration 测试
- [ ] 测试从空数据库开始完整迁移
- [ ] 测试 Migration 版本管理
- [ ] 测试 Migration 回滚
- [ ] 测试重复执行 Migration（幂等性）

#### 集成
- [ ] DatabaseModule 集成 Migration 系统
- [ ] main.cpp 移除硬编码表创建逻辑
- [ ] 添加 `migrate` 命令行参数

---

### 1.4 测试框架
**文件**: `tests/test_main_refactor.cpp`

#### 测试基础设施
- [ ] 创建 Google Test 框架
- [ ] 创建测试数据库（Docker 或临时文件）
- [ ] 创建测试配置文件
- [ ] 创建测试辅助函数（API 请求、断言）

#### API 兼容性测试
- [ ] 创建 API 契约测试（JSON Schema）
- [ ] 记录所有 API 的请求/响应格式
- [ ] 创建 API 响应快照测试

#### 性能基准测试
- [ ] 记录当前 API 响应时间
- [ ] 记录当前数据库查询性能
- [ ] 记录当前内存使用

---

### 阶段 1 验收标准
- [ ] 所有单元测试通过
- [ ] 配置文件正确加载
- [ ] Migration 系统可以创建表
- [ ] 现有功能不受影响（API 测试通过）
- [ ] 代码审查通过
- [ ] 文档更新（配置、Migration）

---

## 阶段 2: 迁移认证系统（3-4 天）

### 2.1 增强 AuthApiModule
**文件**: `src/business/AuthApiModule.cpp`, `include/business/AuthApiModule.hpp`

#### Session 管理迁移
- [ ] 从 main.cpp 迁移 `UserSession` 结构
- [ ] 从 main.cpp 迁移 `g_activeSessions` 全局变量
- [ ] 从 main.cpp 迁移 `g_sessionMutex` 全局变量
- [ ] 迁移 `createSession()` 函数
- [ ] 迁移 `validateSession()` 函数
- [ ] 迁移 `destroySession()` 函数
- [ ] 迁移 `generateToken()` 函数
- [ ] 迁移 `extractToken()` 函数

#### 密码哈希迁移
- [ ] 从 main.cpp 迁移 `hashPassword()` 函数
- [ ] 从 main.cpp 迁移 `verifyPassword()` 函数
- [ ] 从 main.cpp 迁移 `deterministic_hash()` 函数
- [ ] 从 main.cpp 迁移 `htobe64_custom()` 函数
- [ ] **安全升级**: 集成 OpenSSL bcrypt
  ```cpp
  // 推荐使用 bcrypt
  #include <crypt.h>
  std::string hashPasswordBcrypt(const std::string& password);
  bool verifyPasswordBcrypt(const std::string& password, const std::string& hash);
  ```
- [ ] 实现双重密码验证（新 bcrypt、旧自定义）
- [ ] 实现密码升级（登录成功后自动升级）

#### 认证 API 迁移
- [ ] 迁移 `POST /api/auth/register`（1037-1129 行）
- [ ] 迁移 `POST /api/auth/login`（1135-1206 行）
- [ ] 迁移 `POST /api/auth/logout`（1249-1275 行）
- [ ] 迁移 `GET /api/auth/me`（1281-1339 行）
- [ ] **删除**: `GET /api/auth/debug/users`（1209-1243 行）
- [ ] **删除**: `POST /auth/register/test`（3000-3034 行）
- [ ] **删除**: `POST /auth/login` 硬编码版本（3037-3120 行）

#### 安全加固
- [ ] 添加密码强度验证（长度、复杂度）
- [ ] 添加登录失败次数限制（防暴力破解）
- [ ] 添加 Token 过期自动清理（定时任务）
- [ ] 添加 Session 固定攻击防护
- [ ] 添加 CSRF Token（可选）

---

### 2.2 认证模块测试

#### 单元测试
- [ ] 测试密码哈希和验证
- [ ] 测试 Session 创建和验证
- [ ] 测试 Token 生成和解析
- [ ] 测试密码升级逻辑

#### 集成测试
- [ ] 测试注册流程（新用户）
- [ ] 测试登录流程（正确密码、错误密码）
- [ ] 测试登出流程
- [ ] 测试获取用户信息
- [ ] 测试 Token 过期
- [ ] 测试 Session 过期

#### 安全测试
- [ ] SQL 注入测试（登录、注册）
- [ ] XSS 测试（用户名、邮箱）
- [ ] 暴力破解测试（多次失败登录）
- [ ] Session 劫持测试

#### 性能测试
- [ ] 登录响应时间 <100ms
- [ ] Token 验证响应时间 <10ms
- [ ] 并发登录测试（100 用户）

---

### 2.3 main.cpp 清理
- [ ] 删除认证相关全局变量
- [ ] 删除认证相关函数
- [ ] 删除认证 API 路由注册
- [ ] 删除测试端点
- [ ] 删除硬编码密码哈希逻辑

---

### 阶段 2 验收标准
- [ ] 用户可以注册
- [ ] 用户可以登录
- [ ] 用户可以登出
- [ ] 用户信息可以获取
- [ ] Token 过期机制生效
- [ ] 密码升级逻辑生效
- [ ] 安全测试通过
- [ ] 性能测试通过
- [ ] main.cpp 减少约 800 行

---

## 阶段 3: 迁移核心业务 API（4-5 天）

### 3.1 增强 PaperApiModule
**文件**: `src/business/PaperApiModule.cpp`, `include/business/PaperApiModule.hpp`

#### Papers API 迁移
- [ ] 迁移 `GET /api/papers`（1344-1382 行）
- [ ] 迁移 `GET /api/papers/search`（1451-1512 行）
- [ ] 迁移 `GET /api/papers/:id`（1514-1559 行）
- [ ] 迁移 `POST /api/papers`（1562-1615 行）
- [ ] 迁移 `PUT /api/papers/:id`（1618-1711 行）
- [ ] 迁移 `DELETE /api/papers/:id`（1714-1750 行）

#### 功能增强
- [ ] 实现分页（page、pageSize）
- [ ] 实现排序（sortBy、order）
- [ ] 实现过滤（year、journal、author）
- [ ] 实现全文搜索（标题、摘要、作者）
- [ ] 优化数据库查询（添加索引）

---

### 3.2 创建 SearchApiModule
**文件**: `src/business/SearchApiModule.cpp`, `include/business/SearchApiModule.hpp`

#### Search API 迁移
- [ ] 迁移 `GET /api/search`（1386-1448 行）
- [ ] 实现高级搜索（多字段、布尔查询）
- [ ] 实现搜索结果高亮
- [ ] 实现搜索历史
- [ ] 集成 Elasticsearch（可选，未来增强）

---

### 3.3 增强 StatsApiModule
**文件**: `src/business/StatsApiModule.cpp`, `include/business/StatsApiModule.hpp`

#### Statistics API 迁移
- [ ] 迁移 `GET /api/stats`（1841-1880 行）
- [ ] 迁移 `GET /api/stats/papers-by-year`（1886-1919 行）
- [ ] 迁移 `GET /api/stats/top-conferences`（1925-1968 行）
- [ ] 迁移 `GET /api/stats/recent-trends`（1974-2009 行）

#### 功能增强
- [ ] 实现统计数据缓存（Redis、TTL 5 分钟）
- [ ] 实现更多统计维度（作者、期刊、领域）
- [ ] 实现统计导出（CSV、Excel）

---

### 3.4 创建新业务模块

#### JournalApiModule
**文件**: `src/business/JournalApiModule.cpp`, `include/business/JournalApiModule.hpp`
- [ ] 创建 `JournalApiModule` 类
- [ ] 迁移 `GET /api/journals`（1753-1788 行）
- [ ] 实现 `GET /api/journals/:id`
- [ ] 实现 `POST /api/journals`（管理员）
- [ ] 实现 `PUT /api/journals/:id`（管理员）
- [ ] 实现 `DELETE /api/journals/:id`（管理员）

#### AuthorApiModule
**文件**: `src/business/AuthorApiModule.cpp`, `include/business/AuthorApiModule.hpp`
- [ ] 创建 `AuthorApiModule` 类
- [ ] 迁移 `GET /api/authors`（1791-1828 行）
- [ ] 实现 `GET /api/authors/:id`
- [ ] 实现 `GET /api/authors/:id/papers`

#### CollectionApiModule
**文件**: `src/business/CollectionApiModule.cpp`, `include/business/CollectionApiModule.hpp`
- [ ] 创建 `CollectionApiModule` 类
- [ ] 迁移 `GET /api/collections`（1831-1838 行）
- [ ] 实现 `GET /api/collections/:id`
- [ ] 实现 `POST /api/collections`（创建收藏夹）
- [ ] 实现 `PUT /api/collections/:id`（更新收藏夹）
- [ ] 实现 `DELETE /api/collections/:id`（删除收藏夹）
- [ ] 实现 `POST /api/collections/:id/papers`（添加论文到收藏夹）
- [ ] 实现 `DELETE /api/collections/:id/papers/:paperId`（从收藏夹移除论文）

---

### 3.5 业务模块测试

#### PaperApiModule 测试
- [ ] 测试论文列表查询
- [ ] 测试论文详情查询
- [ ] 测试论文创建
- [ ] 测试论文更新
- [ ] 测试论文删除
- [ ] 测试论文搜索
- [ ] 测试分页、排序、过滤

#### SearchApiModule 测试
- [ ] 测试基本搜索
- [ ] 测试高级搜索
- [ ] 测试搜索结果排序
- [ ] 测试搜索性能

#### StatsApiModule 测试
- [ ] 测试统计数据准确性
- [ ] 测试缓存机制
- [ ] 测试统计导出

#### 其他模块测试
- [ ] JournalApiModule 单元测试
- [ ] AuthorApiModule 单元测试
- [ ] CollectionApiModule 单元测试

---

### 3.6 main.cpp 清理
- [ ] 删除所有 Papers API 路由
- [ ] 删除所有 Search API 路由
- [ ] 删除所有 Stats API 路由
- [ ] 删除所有 Journals API 路由
- [ ] 删除所有 Authors API 路由
- [ ] 删除所有 Collections API 路由
- [ ] 删除所有 JSON 构建代码（业务逻辑）

---

### 阶段 3 验收标准
- [ ] 所有 Paper CRUD 操作正常
- [ ] 搜索功能正常
- [ ] 统计数据准确
- [ ] 分页、排序、过滤正常
- [ ] 所有新模块测试通过
- [ ] API 响应格式向后兼容
- [ ] 性能测试通过（无 N+1 查询）
- [ ] main.cpp 减少约 2000 行

---

## 阶段 4: 迁移辅助 API（2-3 天）

### 4.1 增强 AiApiModule
**文件**: `src/business/AiApiModule.cpp`, `include/business/AiApiModule.hpp`

#### AI Co-Pilot API 迁移
- [ ] 迁移 `POST /api/ai-co-pilot/review/generate`（3500-3570 行）
- [ ] 迁移 `POST /api/ai-co-pilot/literature-review/generate`（3575-3640 行）
- [ ] 迁移 `POST /api/ai-co-pilot/brainstorm/generate`（3645-3700 行）

#### 功能增强
- [ ] 集成 OpenAI API（替换 Mock）
- [ ] 实现请求限流（防止滥用）
- [ ] 实现计费统计（Token 使用量）
- [ ] 实现结果缓存（相同请求复用）
- [ ] 实现异步生成（长时间任务）

---

### 4.2 创建 ManagementApiModule
**文件**: `src/business/ManagementApiModule.cpp`, `include/business/ManagementApiModule.hpp`

#### 模块管理 API 迁移
- [ ] 迁移 `GET /api/modules`（946-953 行）
- [ ] 迁移 `POST /api/modules/load`（955-962 行）
- [ ] 迁移 `POST /api/modules/unload`（964-971 行）
- [ ] 迁移 `POST /api/modules/reload`（973-980 行）
- [ ] 迁移 `GET /api/modules/:name/stats`（982-989 行）

#### 功能实现
- [ ] 实现真实的模块加载（从动态库）
- [ ] 实现真实的模块卸载
- [ ] 实现真实的模块重载
- [ ] 实现模块统计信息
- [ ] 添加权限控制（仅管理员）

---

### 4.3 创建 HealthApiModule
**文件**: `src/business/HealthApiModule.cpp`, `include/business/HealthApiModule.hpp`

#### 健康检查 API 迁移
- [ ] 迁移 `GET /health`（992-999 行）
- [ ] 迁移 `GET /health/components`（1002-1008 行）
- [ ] 迁移 `GET /api/health`（1011-1019 行）
- [ ] 迁移 `GET /api/health/components`（1021-1027 行）

#### 功能实现
- [ ] 实现真实的健康检查（数据库、缓存、队列）
- [ ] 实现 `/health` 统一路由（删除重复）
- [ ] 实现 Prometheus 指标导出
- [ ] 实现健康检查评分系统

#### Prometheus 指标
- [ ] HTTP 请求总数（`http_requests_total`）
- [ ] HTTP 请求延迟（`http_request_duration_seconds`）
- [ ] 数据库连接池使用率（`db_pool_usage`）
- [ ] 活跃 Session 数（`active_sessions`）
- [ ] 内存使用（`memory_usage_bytes`）

---

### 4.4 main.cpp 清理
- [ ] 删除所有 AI API 路由
- [ ] 删除所有管理 API 路由
- [ ] 删除所有健康检查 API 路由
- [ ] 删除重复的路由（/health 和 /api/health）

---

### 阶段 4 验收标准
- [ ] AI 功能正常生成响应
- [ ] 模块管理功能正常
- [ ] 健康检查反映真实状态
- [ ] Prometheus 指标可以采集
- [ ] 所有测试通过
- [ ] main.cpp 减少约 200 行

---

## 阶段 5: 精简 main.cpp 和清理（1-2 天）

### 5.1 创建 ApplicationBootstrap
**文件**: `include/core/ApplicationBootstrap.hpp`, `src/core/ApplicationBootstrap.cpp`

#### ApplicationBootstrap 实现
- [ ] 实现 `ApplicationBootstrap::initialize()`
  - 初始化框架核心（MessageBus、Router、PluginManager）
  - 加载配置文件
  - 初始化数据库
  - 运行数据库 Migration
- [ ] 实现 `ApplicationBootstrap::start()`
  - 启动所有模块（按依赖顺序）
  - 启动 HTTP 服务器
  - 注册路由
- [ ] 实现 `ApplicationBootstrap::stop()`
  - 停止 HTTP 服务器
  - 停止所有模块
- [ ] 实现 `ApplicationBootstrap::cleanup()`
  - 清理所有资源
- [ ] 实现 `ApplicationBootstrap::shutdown()`
  - 优雅关闭（由信号处理程序调用）

#### 错误处理
- [ ] 启动失败时提供详细错误信息
- [ ] 实现启动失败自动回滚
- [ ] 实现模块启动失败隔离（不影响其他模块）

---

### 5.2 重写 main.cpp
**文件**: `src/core/main.cpp`

#### 保留内容
- [ ] 全局变量声明（`g_running`、`g_httpServer`）
- [ ] 信号处理函数
- [ ] 欢迎信息打印
- [ ] 主函数（ApplicationBootstrap 调用）

#### 删除内容
- [ ] 所有业务逻辑（已迁移到模块）
- [ ] 所有数据库初始化（已迁移到 DatabaseModule）
- [ ] 所有认证逻辑（已迁移到 AuthApiModule）
- [ ] 所有 API 路由注册（已迁移到业务模块）
- [ ] 所有工具函数（已迁移到 Utils）
- [ ] 所有辅助函数（保留或简化）

#### 精简辅助函数
- [ ] 保留 `printWelcome()`、`printReady()`
- [ ] 简化 `printStep()`、`printSuccess()`、`printError()`
- [ ] 删除调试相关函数

---

### 5.3 代码清理
- [ ] 删除所有已迁移的代码
- [ ] 删除未使用的头文件
- [ ] 删除未使用的全局变量
- [ ] 删除注释掉的代码
- [ ] 删除调试代码
- [ ] 统一代码风格（格式化）

---

### 5.4 CMakeLists.txt 更新
- [ ] 添加新的业务模块目标
- [ ] 移除已废弃的源文件
- [ ] 更新链接库
- [ ] 更新包含路径

---

### 5.5 文档更新
- [ ] 更新 README.md（新的启动流程）
- [ ] 更新 API 文档（所有端点）
- [ ] 更新架构设计文档
- [ ] 更新部署文档
- [ ] 更新开发文档（如何添加新模块）

---

### 阶段 5 验收标准
- [ ] main.cpp 行数 <300
- [ ] 所有模块正常加载
- [ ] 所有 API 正常响应
- [ ] 优雅关闭正常工作
- [ ] 内存泄漏检查通过（Valgrind）
- [ ] 代码审查通过
- [ ] 文档完整

---

## 最终验收

### 功能验收
- [ ] 用户认证流程正常（注册、登录、登出）
- [ ] 论文管理正常（CRUD）
- [ ] 搜索功能正常
- [ ] 统计功能正常
- [ ] AI 功能正常
- [ ] 模块管理正常
- [ ] 健康检查正常

### 性能验收
- [ ] API 响应时间不增加 >10%（对比基准）
- [ ] 内存使用不增加 >20%（对比基准）
- [ ] 启动时间不增加 >50%（对比基准）
- [ ] 并发测试通过（1000 用户）

### 安全验收
- [ ] SQL 注入测试通过
- [ ] XSS 测试通过
- [ ] CSRF 测试通过
- [ ] 密码哈希使用 bcrypt
- [ ] Token 安全生成

### 代码质量验收
- [ ] main.cpp 行数 <300
- [ ] 代码重复率 <5%
- [ ] 测试覆盖率 >80%
- [ ] 代码审查通过率 >90%
- [ ] 无编译警告（-Wall -Wextra）

### 文档验收
- [ ] README.md 更新
- [ ] API 文档完整
- [ ] 架构文档完整
- [ ] 部署文档完整
- [ ] 重构总结报告

---

## 发布准备

### 预发布检查
- [ ] 所有测试通过（单元、集成、E2E）
- [ ] 性能基准测试通过
- [ ] 安全测试通过
- [ ] 代码审查通过
- [ ] 文档审查通过

### 发布计划
- [ ] 准备发布说明（Release Notes）
- [ ] 准备回滚计划
- [ ] 准备监控和告警
- [ ] 通知团队成员
- [ ] 安排发布时间窗口

### 发布执行
- [ ] 备份生产数据库
- [ ] 部署到测试环境
- [ ] 测试环境验证（冒烟测试）
- [ ] 部署到生产环境（灰度发布）
- [ ] 生产环境验证
- [ ] 监控关键指标（1 小时）
- [ ] 通知用户系统升级

### 发布后
- [ ] 持续监控 24 小时
- [ ] 收集用户反馈
- [ ] 处理紧急问题
- [ ] 总结发布经验

---

## 附录

### A. 相关文件路径
```
PaperCrawler/backend/
├── include/
│   ├── core/
│   │   ├── ApplicationBootstrap.hpp    # 新建
│   │   └── ConfigManager.hpp           # 增强
│   ├── business/
│   │   ├── AuthApiModule.hpp           # 增强
│   │   ├── PaperApiModule.hpp          # 增强
│   │   ├── SearchApiModule.hpp         # 增强
│   │   ├── StatsApiModule.hpp          # 增强
│   │   ├── AiApiModule.hpp             # 增强
│   │   ├── ManagementApiModule.hpp     # 新建
│   │   ├── HealthApiModule.hpp         # 新建
│   │   ├── JournalApiModule.hpp        # 新建
│   │   ├── AuthorApiModule.hpp         # 新建
│   │   └── CollectionApiModule.hpp     # 新建
│   └── utils/
│       └── StringUtils.hpp             # 新建
├── src/
│   ├── core/
│   │   ├── main.cpp                    # 重写（目标 <300 行）
│   │   ├── ApplicationBootstrap.cpp    # 新建
│   │   └── ConfigManager.cpp           # 增强
│   ├── business/
│   │   ├── AuthApiModule.cpp           # 增强
│   │   ├── PaperApiModule.cpp          # 增强
│   │   ├── SearchApiModule.cpp         # 增强
│   │   ├── StatsApiModule.cpp          # 增强
│   │   ├── AiApiModule.cpp             # 增强
│   │   ├── ManagementApiModule.cpp     # 新建
│   │   └── HealthApiModule.cpp         # 新建
│   └── utils/
│       └── StringUtils.cpp             # 新建
├── config/
│   ├── database.json                   # 新建
│   ├── http.json                       # 新建
│   ├── modules.json                    # 已有
│   └── app.json                        # 新建
├── migrations/
│   ├── 002_create_users_table.sql      # 新建
│   ├── 003_create_user_sessions_table.sql  # 新建
│   └── ...                             # 其他表
└── tests/
    ├── unit/                           # 单元测试
    ├── integration/                    # 集成测试
    └── e2e/                            # 端到端测试
```

### B. 命令速查
```bash
# 构建项目
cd build && cmake .. && make

# 运行单元测试
ctest --output-on-failure

# 运行内存泄漏检测
valgrind --leak-check=full ./PaperCrawler

# 检查代码重复
lizard src/core/main.cpp

# 代码格式化
clang-format -i src/core/main.cpp

# 生成 API 文档
doxygen Doxyfile
```

---

**检查清单版本**: 1.0
**最后更新**: 2025-04-04
**使用方法**: 打印或导入到任务管理系统（如 Jira、Trello）
