# PaperCrawler 架构设计文档索引

## 📋 概述

本目录包含PaperCrawler项目的完整架构设计文档，涵盖从核心框架到具体实现的各个层面。

**架构师**: Backend Architect
**设计原则**: 极致解耦、可扩展、可维护、高性能
**目标**: 构建企业级分布式爬虫系统

---

## 📚 文档目录

### 1. [模块化框架设计方案](./模块化框架设计方案.md)
**核心架构文档**

- ✅ 当前模块系统评估
- 🎯 架构设计原则（DIP、ISP、SRP）
- 🏗️ 整体架构分层（5层架构）
- 💡 核心框架层设计
  - 增强的服务容器（AOP支持）
  - 统一事件系统
  - 配置管理系统
- 📦 领域层设计
  - 领域服务接口
  - 领域模型
- 🔧 应用服务层设计（用例层）
- 🔌 插件系统增强
- 📋 实施路线图

**适合人群**: 架构师、技术负责人、高级工程师

---

### 2. [API设计规范](./API设计规范.md)
**API开发和集成指南**

- 🌐 RESTful API设计原则
- 🔗 URL设计和资源命名
- 📨 请求/响应格式规范
- 🔐 认证和授权机制
- ⚡ 速率限制策略
- 📖 核心API端点文档
  - 论文管理API
  - 爬虫管理API
  - 用户管理API
- 🔧 版本控制策略
- ✅ 最佳实践和示例

**适合人群**: 前后端工程师、API集成开发者

---

## 🏗️ 架构分层概览

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (Business Modules: PaperAPI, AuthAPI, CrawlerAPI...)      │
│  职责：处理HTTP请求，路由分发，响应处理                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                  Domain Layer (New!)                        │
│  (Domain Services: ICrawlerService, IUserService...)       │
│  (Domain Models: Paper, User, Template...)                 │
│  职责：定义业务逻辑，领域模型，业务规则                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│               Application Services Layer                    │
│  (Use Cases: CrawlPaperUseCase, AuthenticateUser...)       │
│  职责：编排业务流程，事务管理，跨服务协调                    │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                  Infrastructure Layer                       │
│  (Technical Capabilities: Database, Cache, HTTP, Queue...)  │
│  职责：提供技术能力，与外部系统交互                          │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Core Framework Layer                      │
│  (PluginManager, ServiceContainer, EventBus, Router...)     │
│  职责：模块管理，依赖注入，事件总线，路由分发                │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔑 核心设计原则

### 1. 依赖倒置原则 (DIP)
```cpp
// ✅ 正确：依赖抽象
class TemplateCrawlerModule {
    std::shared_ptr<Domain::ICrawlerService> crawlerService_;
};

// ❌ 错误：依赖具体实现
class TemplateCrawlerModule {
    std::shared_ptr<DatabaseModule> database_;
};
```

### 2. 接口隔离原则 (ISP)
```cpp
// ✅ 接口细粒度拆分
class IReadable {
    virtual std::string read(const std::string& key) = 0;
};

class IWritable {
    virtual bool write(const std::string& key, const std::string& value) = 0;
};

class ICache : public IReadable, public IWritable {
    // 组合接口
};
```

### 3. 单一职责原则 (SRP)
```cpp
// ✅ 职责分离
class CrawlerOrchestrator {      // 协调爬取任务
    void orchestrateCrawl(const CrawlTask& task);
};

class HtmlParser {                // HTML解析
    ParseResult parse(const std::string& html);
};

class DataExtractor {             // 数据提取
    std::map<std::string, std::string> extract(const ParseResult& parsed);
};
```

---

## 📊 架构优势分析

### 优势
| 特性 | 说明 | 收益 |
|------|------|------|
| **极致解耦** | 依赖抽象接口，模块间零耦合 | 易于维护、测试、替换 |
| **高可测试性** | 依赖注入，便于Mock | 单元测试覆盖率提升 |
| **灵活扩展** | 插件架构，动态加载 | 新功能热插拔 |
| **事件驱动** | 统一事件总线 | 异步解耦通信 |
| **配置管理** | 集中化配置，热重载 | 环境隔离，运维友好 |

### 权衡
| 方面 | 影响 | 缓解措施 |
|------|------|----------|
| **复杂度增加** | 更多抽象层次 | 提供详细文档和示例 |
| **学习曲线** | 团队需理解DDD | 培训和代码审查 |
| **性能开销** | DI和事件总线 | 性能测试和优化 |

---

## 🚀 实施路线图

### Phase 1: 核心框架重构（2-3周）
**目标**: 建立坚实的基础框架

- [x] 实现`EnhancedServiceContainer`
- [x] 实现`UnifiedEventBus`
- [ ] 实现`ConfigurationManager`
- [ ] 编写单元测试

**交付物**:
- `backend/include/core/EnhancedServiceContainer.hpp` ✅
- `backend/include/core/UnifiedEventBus.hpp` ✅
- `backend/tests/core/` 单元测试套件

---

### Phase 2: 领域层设计（2-3周）
**目标**: 定义清晰的业务边界

- [x] 定义领域服务接口
- [ ] 实现领域模型
- [ ] 编写领域层单元测试
- [ ] 集成现有爬虫逻辑

**交付物**:
- `backend/include/domain/ICrawlerService.hpp` ✅
- `backend/include/domain/IPaperRepository.hpp` ✅
- `backend/include/domain/models/` 领域模型

---

### Phase 3: 应用服务层（2周）
**目标**: 实现业务用例编排

- [x] 实现用例层
- [ ] 重构现有业务模块
- [ ] 编写集成测试

**交付物**:
- `backend/include/application/services/TemplateCrawlerService.hpp` ✅
- `backend/include/application/useCases/` 用例实现

---

### Phase 4: 插件系统增强（1-2周）
**目标**: 完善插件生态

- [ ] 实现`PluginMetadata`
- [ ] 增强`PluginManager`
- [ ] 实现插件兼容性检查

**交付物**:
- `backend/include/core/PluginMetadata.hpp`
- `backend/include/core/EnhancedPluginManager.hpp`

---

### Phase 5: 迁移和优化（3-4周）
**目标**: 现有代码迁移和性能优化

- [ ] 逐步迁移现有模块
- [ ] 性能测试和优化
- [ ] 文档完善
- [ ] 监控和日志集成

**交付物**:
- 迁移指南
- 性能测试报告
- 监控仪表板

---

## 📁 关键文件位置

### 核心框架
```
backend/include/core/
├── EnhancedServiceContainer.hpp    # 增强的服务容器 ✅
├── UnifiedEventBus.hpp              # 统一事件总线 ✅
├── ConfigurationManager.hpp         # 配置管理器（待实现）
├── PluginManager.hpp                # 插件管理器
└── ModuleRegistry.hpp               # 模块注册表
```

### 领域层
```
backend/include/domain/
├── ICrawlerService.hpp              # 爬虫服务接口 ✅
├── IPaperRepository.hpp             # 论文仓储接口 ✅
├── IAuthenticationService.hpp       # 认证服务接口（待实现）
└── models/
    ├── Paper.hpp                    # 论文领域模型（待实现）
    ├── User.hpp                     # 用户领域模型（待实现）
    └── Template.hpp                 # 模板领域模型（待实现）
```

### 应用服务层
```
backend/include/application/
├── services/
│   └── TemplateCrawlerService.hpp   # 模板爬虫服务 ✅
└── useCases/
    ├── CrawlPaperUseCase.hpp        # 爬取论文用例（待实现）
    └── AuthenticateUserUseCase.hpp  # 认证用例（待实现）
```

---

## 🔗 相关资源

### 外部参考
- [Domain-Driven Design (Eric Evans)](https://www.domainlanguage.com/ddd/)
- [Clean Architecture (Robert C. Martin)](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html)
- [SOLID Principles](https://en.wikipedia.org/wiki/SOLID)
- [RESTful API Design](https://restfulapi.net/)

### 内部文档
- [开发指南](../开发指南/)
- [数据库设计](../数据库设计/)
- [部署文档](../部署文档/)

---

## 📈 性能指标

| 指标 | 目标值 | 当前值 | 状态 |
|------|--------|--------|------|
| 模块加载时间 | < 100ms | - | 🟡 待测 |
| 服务解析时间 | < 1ms | - | 🟡 待测 |
| 事件发布延迟 | < 10ms | - | 🟡 待测 |
| HTTP响应时间 | P95 < 200ms | - | 🟡 待测 |
| 单元测试覆盖率 | > 80% | - | 🟡 待测 |

---

## 🤝 贡献指南

### 架构评审流程
1. 提交架构设计文档
2. 技术评审会议
3. 实施方案确认
4. 编码和测试
5. 代码审查
6. 合并和部署

### 文档更新规范
- 重大架构变更：更新主架构文档
- API变更：更新API设计规范
- 新增模块：添加模块设计文档

---

## 📞 联系方式

**架构团队**: backend-architects@papercrawler.com
**技术支持**: support@papercrawler.com
**问题反馈**: [GitHub Issues](https://github.com/papercrawler/issues)

---

## 📝 变更日志

### v1.0.0 (2026-04-03)
- ✅ 初始架构设计文档
- ✅ 模块化框架设计方案
- ✅ API设计规范
- ✅ 核心框架代码实现

### 即将发布
- [ ] 配置管理器实现
- [ ] 领域模型完整实现
- [ ] 性能测试报告

---

**最后更新**: 2026-04-03
**文档版本**: v1.0.0
**维护者**: Backend Architect
**状态**: ✅ 已完成初稿，待评审
