# PaperCrawler 架构重构实施计划

## 项目概述

**目标**: 将PaperCrawler后端系统重构为高度模块化、可复用的架构

**时间跨度**: 8-10周

**团队规模**: 2-3名开发人员

**优先级**: P0（高优先级，必须完成）

---

## 阶段一：紧急修复（Week 1-2）

### 目标
消除强耦合点，建立抽象接口层

### 任务清单

#### Task 1.1：创建抽象接口层
**负责人**: 架构师
**预计时间**: 2天
**优先级**: P0

**子任务**:
- [ ] 运行 `scripts/generate_interfaces.py` 生成接口文件
- [ ] 审查生成的接口定义
- [ ] 根据实际需求调整接口方法
- [ ] 编写接口使用文档

**验收标准**:
- 所有接口文件在 `backend/include/interfaces/` 目录下
- 接口文档完整
- 通过代码审查

---

#### Task 1.2：重构CrawlerApiModule
**负责人**: 后端开发A
**预计时间**: 3天
**优先级**: P0

**当前代码**:
```cpp
// business/CrawlerApiModule.hpp
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"

class CrawlerApiModule {
private:
    std::shared_ptr<TemplateCrawlerModule> templateCrawler_;
    std::shared_ptr<DistributedTaskModule> distributedTask_;
};
```

**目标代码**:
```cpp
// business/CrawlerApiModule.hpp
#include "interfaces/ICrawler.hpp"
#include "interfaces/IDistributedTask.hpp"

class CrawlerApiModule {
private:
    std::shared_ptr<ICrawler> crawler_;
    std::shared_ptr<IDistributedTask> task_;

public:
    void setCrawler(std::shared_ptr<ICrawler> crawler);
    void setTask(std::shared_ptr<IDistributedTask> task);
};
```

**步骤**:
1. 修改头文件，替换include
2. 修改构造函数，接受接口参数
3. 修改所有使用crawler_和distributedTask_的代码
4. 更新单元测试

**验收标准**:
- CrawlerApiModule不再直接包含具体模块头文件
- 单元测试通过（使用Mock对象）
- 集成测试通过

---

#### Task 1.3：重构CollaborativeWritingModule
**负责人**: 后端开发B
**预计时间**: 2天
**优先级**: P0

**步骤**:
1. 创建IWebSocket接口（如果尚未创建）
2. 修改CollaborativeWritingModule使用IWebSocket接口
3. 更新WebSocketModule实现IWebSocket接口
4. 编写单元测试

**验收标准**:
- CollaborativeWritingModule不依赖WebSocketModule.hpp
- 单元测试覆盖率>80%

---

#### Task 1.4：重构UnifiedAIWorkflow
**负责人**: 后端开发A
**预计时间**: 1天
**优先级**: P0

**步骤**:
1. 修改UnifiedAIWorkflow使用ICache接口
2. 更新ServiceContainer注册
3. 编写单元测试

**验收标准**:
- UnifiedAIWorkflow不依赖CacheModule.hpp
- 功能测试通过

---

#### Task 1.5：消除ServiceLayer循环依赖
**负责人**: 架构师 + 后端开发B
**预计时间**: 3天
**优先级**: P0

**当前问题**:
ServiceLayer依赖多个业务模块，这些模块可能反过来依赖ServiceLayer

**解决方案**:
1. 分析ServiceLayer职责
2. 引入事件驱动架构
3. 定义领域事件
4. 重构模块间通信使用事件

**代码示例**:
```cpp
// 定义事件
struct PaperCreatedEvent {
    int paperId;
    std::string title;
    std::chrono::system_clock::time_point timestamp;
};

// PaperApiModule发布事件
bool PaperApiModule::createPaper(const Paper& paper) {
    // 创建逻辑...

    // 发布事件
    EventBusModule::getInstance().publish(
        "PaperCreated",
        PaperCreatedEvent{...}
    );

    return true;
}

// ServiceLayer订阅事件
void ServiceLayer::initialize() {
    EventBusModule::getInstance().subscribe(
        "PaperCreated",
        [this](const Event& e) {
            auto data = std::any_cast<PaperCreatedEvent>(e.data);
            onPaperCreated(data);
        }
    );
}
```

**验收标准**:
- ServiceLayer不再直接包含其他业务模块头文件
- 通过事件总线通信
- 性能测试通过（事件延迟<10ms）

---

#### Task 1.6：修复数据访问层依赖
**负责人**: 后端开发B
**预计时间**: 2天
**优先级**: P1

**问题**:
1. MySqlConnection依赖DatabaseModule
2. RedisConnection依赖DatabaseModule

**解决方案**:
1. 创建IConnection接口
2. MySqlConnection实现IConnection
3. RedisConnection实现IConnection
4. DatabaseModule通过IConnection使用具体实现

**验收标准**:
- MySqlConnection.hpp不包含DatabaseModule.hpp
- RedisConnection.hpp不包含DatabaseModule.hpp
- 编译无错误

---

### 阶段一总结

**交付物**:
- 抽象接口层（5个核心接口）
- 重构后的业务模块（3个）
- 事件驱动架构基础
- 修复后的数据访问层

**里程碑**: 所有P0级强耦合问题解决

---

## 阶段二：提取可复用组件（Week 3-6）

### 目标
提取通用基础设施为独立库

### 任务清单

#### Task 2.1：提取核心库（PaperCrawler-Core）
**负责人**: 架构师
**预计时间**: 1周
**优先级**: P1

**子任务**:
1. 创建 `PaperCrawler-Core` 目录结构
2. 移动核心文件到新目录
3. 创建CMakeLists.txt
4. 编写库文档
5. 编写单元测试

**目录结构**:
```
PaperCrawler-Core/
├── include/
│   ├── PaperCrawler/
│   │   ├── IM oudle.hpp
│   │   ├── ModuleBase.hpp
│   │   ├── ServiceContainer.hpp
│   │   ├── Router.hpp
│   │   ├── EventBus.hpp
│   │   ├── HttpTypes.hpp
│   │   └── ModuleExports.hpp
├── src/
│   ├── IM oudle.cpp
│   ├── ModuleBase.cpp
│   ├── ServiceContainer.cpp
│   ├── Router.cpp
│   └── EventBus.cpp
├── tests/
│   ├── test_ServiceContainer.cpp
│   ├── test_Router.cpp
│   └── test_EventBus.cpp
├── CMakeLists.txt
├── README.md
└── LICENSE
```

**CMakeLists.txt示例**:
```cmake
cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerCore VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 库配置
add_library(PaperCrawlerCore INTERFACE)
add_library(PaperCrawler::Core ALIAS PaperCrawlerCore)

target_include_directories(PaperCrawlerCore
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 安装配置
install(TARGETS PaperCrawlerCore
    EXPORT PaperCrawlerCoreTargets
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include/PaperCrawler
)

# 导出目标
install(EXPORT PaperCrawlerCoreTargets
    FILE PaperCrawlerCoreTargets.cmake
    NAMESPACE PaperCrawler::
    DESTINATION lib/cmake/PaperCrawlerCore
)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    PaperCrawlerCoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/PaperCrawlerCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PaperCrawlerCoreConfig.cmake
    INSTALL_DESTINATION lib/cmake/PaperCrawlerCore
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PaperCrawlerCoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PaperCrawlerCoreConfigVersion.cmake
    DESTINATION lib/cmake/PaperCrawlerCore
)
```

**验收标准**:
- 核心库可独立编译
- 单元测试覆盖率>90%
- 文档完整
- 可被其他项目引用

---

#### Task 2.2：提取网络库（PaperCrawler-Network）
**负责人**: 后端开发A
**预计时间**: 1周
**优先级**: P1

**子任务**:
1. 创建 `PaperCrawler-Network` 目录
2. 提取HttpServerModule, WebSocketModule
3. 创建INetworkServer接口
4. 编写单元测试
5. 编写文档

**目录结构**:
```
PaperCrawler-Network/
├── include/
│   ├── PaperCrawler/
│   │   ├── Network/
│   │   │   ├── IHttpServer.hpp
│   │   │   ├── IWebSocket.hpp
│   │   │   ├── IHttpClient.hpp
│   │   │   ├── HttpServer.hpp
│   │   │   ├── WebSocket.hpp
│   │   │   └── HttpClient.hpp
├── src/
├── tests/
├── examples/
│   └── simple_http_server.cpp
├── CMakeLists.txt
└── README.md
```

**验收标准**:
- 网络库可独立编译
- 依赖PaperCrawler::Core
- 单元测试通过
- 示例代码可运行

---

#### Task 2.3：提取数据访问库（PaperCrawler-Data）
**负责人**: 后端开发B
**预计时间**: 1周
**优先级**: P1

**子任务**:
1. 创建 `PaperCrawler-Data` 目录
2. 提取IDatabase, ICache, IConnection接口
3. 提取DatabaseModule实现
4. 编写单元测试
5. 编写文档

**目录结构**:
```
PaperCrawler-Data/
├── include/
│   ├── PaperCrawler/
│   │   ├── Data/
│   │   │   ├── IDatabase.hpp
│   │   │   ├── ICache.hpp
│   │   │   ├── IConnection.hpp
│   │   │   ├── DatabaseModule.hpp
│   │   │   ├── CacheModule.hpp
│   │   │   ├── MySqlConnection.hpp
│   │   │   └── RedisConnection.hpp
├── src/
├── tests/
├── CMakeLists.txt
└── README.md
```

**验收标准**:
- 数据访问库可独立编译
- 依赖PaperCrawler::Core
- 单元测试通过（包括集成测试）

---

#### Task 2.4：提取中间件库（PaperCrawler-Middleware）
**负责人**: 后端开发A
**预计时间**: 1周
**优先级**: P2

**子任务**:
1. 创建 `PaperCrawler-Middleware` 目录
2. 提取LoggingModule, MetricsModule等
3. 创建IMiddleware接口
4. 编写单元测试
5. 编写文档

**验收标准**:
- 中间件库可独立编译
- 依赖PaperCrawler::Core
- 单元测试通过

---

#### Task 2.5：更新PaperCrawler主项目
**负责人**: 全员
**预计时间**: 3天
**优先级**: P1

**子任务**:
1. 修改主CMakeLists.txt，使用新库
2. 更新构建脚本
3. 更新CI/CD配置
4. 迁移业务模块代码
5. 集成测试

**CMakeLists.txt修改**:
```cmake
# 查找依赖
find_package(PaperCrawlerCore REQUIRED)
find_package(PaperCrawlerNetwork REQUIRED)
find_package(PaperCrawlerData REQUIRED)
find_package(PaperCrawlerMiddleware REQUIRED)

# 业务模块
add_library(PaperApiModule SHARED
    business/PaperApiModule.cpp
)

target_link_libraries(PaperApiModule
    PUBLIC
        PaperCrawler::Core
        PaperCrawler::Data
)

target_include_directories(PaperApiModule
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)
```

**验收标准**:
- 主项目使用新库编译成功
- 所有测试通过
- CI/CD管道正常
- 性能无回退

---

### 阶段二总结

**交付物**:
- PaperCrawler-Core独立库
- PaperCrawler-Network独立库
- PaperCrawler-Data独立库
- PaperCrawler-Middleware独立库
- 更新后的主项目

**里程碑**: 可复用组件提取完成

---

## 阶段三：事件驱动架构（Week 7-8）

### 目标
引入事件驱动架构，完全解耦模块

### 任务清单

#### Task 3.1：设计领域事件模型
**负责人**: 架构师
**预计时间**: 2天
**优先级**: P1

**子任务**:
1. 识别所有领域事件
2. 定义事件结构
3. 设计事件命名规范
4. 创建事件定义文件

**事件定义示例**:
```cpp
// include/events/PaperEvents.hpp
namespace PaperCrawler::Events {

struct PaperCreatedEvent {
    int paperId;
    std::string title;
    std::string authors;
    int year;
    std::string journal;
    std::chrono::system_clock::time_point timestamp;
    int createdBy;
};

struct PaperUpdatedEvent {
    int paperId;
    std::vector<std::string> changedFields;
    std::chrono::system_clock::time_point timestamp;
    int updatedBy;
};

struct PaperDeletedEvent {
    int paperId;
    std::string title;
    std::chrono::system_clock::time_point timestamp;
    int deletedBy;
};

struct PaperSearchEvent {
    std::string query;
    std::map<std::string, std::string> filters;
    size_t resultCount;
    std::chrono::milliseconds executionTime;
};

} // namespace PaperCrawler::Events
```

**验收标准**:
- 事件定义完整
- 符合领域驱动设计原则
- 通过团队评审

---

#### Task 3.2：实现事件发布
**负责人**: 后端开发A
**预计时间**: 3天
**优先级**: P1

**子任务**:
1. 在PaperApiModule中发布事件
2. 在AuthApiModule中发布事件
3. 在UserApiModule中发布事件
4. 在CrawlerApiModule中发布事件
5. 单元测试

**代码示例**:
```cpp
// PaperApiModule.cpp
bool PaperApiModule::createPaper(const Paper& paper) {
    // 保存到数据库
    auto result = database_->execute(...);

    if (result.success) {
        // 发布事件
        Events::PaperCreatedEvent event;
        event.paperId = result.lastInsertId;
        event.title = paper.title;
        event.authors = paper.authors;
        event.timestamp = std::chrono::system_clock::now();
        event.createdBy = getCurrentUserId();

        eventBus_->publish("PaperCreated", event);
    }

    return result.success;
}
```

**验收标准**:
- 所有关键操作发布事件
- 事件发布不影响性能
- 单元测试通过

---

#### Task 3.3：实现事件订阅
**负责人**: 后端开发B
**预计时间**: 3天
**优先级**: P1

**子任务**:
1. ServiceLayer订阅论文事件
2. MetricsModule订阅所有事件（统计）
3. LoggingModule订阅错误事件
4. CacheModule订阅数据变更事件
5. 单元测试

**代码示例**:
```cpp
// ServiceLayer.cpp
void ServiceLayer::initialize() {
    // 订阅论文创建事件
    eventBus_->subscribe("PaperCreated", [this](const Event& e) {
        auto data = std::any_cast<Events::PaperCreatedEvent>(e.data);
        onPaperCreated(data);
    });

    // 订阅论文更新事件
    eventBus_->subscribe("PaperUpdated", [this](const Event& e) {
        auto data = std::any_cast<Events::PaperUpdatedEvent>(e.data);
        onPaperUpdated(data);
    });
}

void ServiceLayer::onPaperCreated(const Events::PaperCreatedEvent& event) {
    // 更新搜索索引
    searchModule_->indexPaper(event.paperId);

    // 更新推荐系统
    recommendationModule_->onNewPaper(event);

    // 发送通知
    notificationModule_->notifyPaperCreated(event);
}
```

**验收标准**:
- 事件订阅正确
- 事件处理逻辑正确
- 单元测试通过

---

#### Task 3.4：实现事件持久化
**负责人**: 架构师
**预计时间**: 3天
**优先级**: P2

**子任务**:
1. 设计事件存储表结构
2. 实现事件持久化逻辑
3. 实现事件重放功能
4. 性能优化
5. 单元测试

**数据库表设计**:
```sql
CREATE TABLE events (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    event_id VARCHAR(64) UNIQUE NOT NULL,
    event_type VARCHAR(100) NOT NULL,
    event_data JSON NOT NULL,
    source_module VARCHAR(50) NOT NULL,
    timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    processed BOOLEAN DEFAULT FALSE,
    INDEX idx_event_type (event_type),
    INDEX idx_timestamp (timestamp),
    INDEX idx_processed (processed)
);
```

**验收标准**:
- 事件正确持久化
- 支持事件重放
- 性能影响<5%

---

#### Task 3.5：事件监控和调试工具
**负责人**: 后端开发A
**预计时间**: 2天
**优先级**: P2

**子任务**:
1. 创建事件监控面板
2. 实现事件追踪
3. 实现事件回放工具
4. 性能监控

**验收标准**:
- 可视化事件流
- 支持事件查询
- 性能指标收集

---

### 阶段三总结

**交付物**:
- 完整的领域事件模型
- 事件发布和订阅机制
- 事件持久化系统
- 事件监控工具

**里程碑**: 事件驱动架构上线

---

## 阶段四：完善和优化（Week 9-10）

### 目标
完善架构，优化性能，编写文档

### 任务清单

#### Task 4.1：完善依赖注入
**负责人**: 架构师
**预计时间**: 2天
**优先级**: P1

**子任务**:
1. 注册所有服务到ServiceContainer
2. 实现自动依赖解析
3. 实现生命周期管理
4. 编写文档

**代码示例**:
```cpp
// main.cpp
void initializeServices() {
    // 核心服务
    Services::registerService<IDatabase, DatabaseModule>(
        ServiceLifetime::SINGLETON
    );
    Services::registerService<ICache, CacheModule>(
        ServiceLifetime::SINGLETON
    );
    Services::registerService<IWebSocket, WebSocketModule>(
        ServiceLifetime::SINGLETON
    );
    Services::registerService<ICrawler, TemplateCrawlerModule>(
        ServiceLifetime::TRANSIENT
    );
    Services::registerService<IDistributedTask, DistributedTaskModule>(
        ServiceLifetime::SINGLETON
    );

    // 业务模块
    Services::registerService<IPaperService, PaperApiModule>(
        ServiceLifetime::SCOPED
    );
    Services::registerService<IAuthService, AuthApiModule>(
        ServiceLifetime::SCOPED
    );
}
```

**验收标准**:
- 所有服务通过容器管理
- 依赖自动解析
- 文档完整

---

#### Task 4.2：性能优化
**负责人**: 后端开发A + 后端开发B
**预计时间**: 3天
**优先级**: P1

**优化重点**:
1. 事件总线性能优化
2. 数据库连接池优化
3. 缓存策略优化
4. HTTP服务器性能优化
5. 内存优化

**性能目标**:
- 事件延迟<10ms
- 数据库查询<50ms
- API响应<100ms
- 内存使用<500MB

**验收标准**:
- 所有性能指标达标
- 性能测试通过

---

#### Task 4.3：编写完整文档
**负责人**: 全员
**预计时间**: 3天
**优先级**: P1

**文档清单**:
1. 架构设计文档
2. API接口文档
3. 开发指南
4. 部署指南
5. 运维手册
6. 故障排查手册

**验收标准**:
- 文档完整
- 示例代码可运行
- 通过团队评审

---

#### Task 4.4：代码审查和重构
**负责人**: 架构师
**预计时间**: 2天
**优先级**: P1

**子任务**:
1. 全面代码审查
2. 重构不符合规范的代码
3. 代码格式化
4. 添加注释

**验收标准**:
- 代码规范检查通过
- 代码覆盖率>80%
- 技术债务清单清零

---

#### Task 4.5：集成测试和压力测试
**负责人**: 全员
**预计时间**: 2天
**优先级**: P0

**测试清单**:
1. 完整的集成测试套件
2. API接口测试
3. 性能测试
4. 压力测试
5. 稳定性测试

**验收标准**:
- 所有测试通过
- 无P0/P1级Bug
- 性能指标达标

---

### 阶段四总结

**交付物**:
- 完善的依赖注入系统
- 优化后的代码
- 完整的文档
- 全面的测试

**里程碑**: 项目重构完成

---

## 风险管理

### 风险识别

| 风险 | 影响 | 概率 | 应对措施 |
|------|------|------|----------|
| 重构影响现有功能 | 高 | 中 | 完整的回归测试，分阶段发布 |
| 性能回退 | 中 | 中 | 性能基准测试，持续监控 |
| 学习曲线陡峭 | 中 | 高 | 培训，文档，导师制 |
| 时间延期 | 高 | 中 | 预留缓冲时间，MVP优先 |
| 团队抵触 | 中 | 低 | 沟通，演示，培训 |

### 应对计划

1. **每周进度评审**
   - 时间：每周五下午
   - 参与者：全体开发人员
   - 内容：进度汇报，风险识别，问题解决

2. **代码审查机制**
   - 所有PR必须经过审查
   - 架构师审查架构相关代码
   - 同行审查业务逻辑代码

3. **持续集成/持续部署**
   - 每次提交自动运行测试
   - 每日构建成功
   - 测试覆盖率监控

---

## 质量保证

### 代码质量标准

1. **代码规范**
   - 遵循Google C++ Style Guide
   - 使用clang-format格式化
   - 使用clang-tidy静态分析

2. **测试覆盖率**
   - 单元测试覆盖率>80%
   - 集成测试覆盖率>60%
   - 关键路径覆盖率>95%

3. **性能标准**
   - API响应时间P95<200ms
   - 事件延迟P95<20ms
   - 内存使用<1GB
   - CPU使用<80%

4. **文档标准**
   - 所有公共API有文档
   - 复杂逻辑有注释
   - 架构决策有ADR

---

## 成功标准

### 技术指标

- [ ] 所有P0级强耦合问题解决
- [ ] 提取4个独立库
- [ ] 事件驱动架构上线
- [ ] 单元测试覆盖率>80%
- [ ] 集成测试覆盖率>60%
- [ ] 性能无回退

### 业务指标

- [ ] 新功能开发速度提升30%
- [ ] Bug修复时间缩短50%
- [ ] 系统可用性>99.9%
- [ ] 部署时间缩短60%

### 团队指标

- [ ] 代码审查通过率>95%
- [ ] 技术债务清零
- [ ] 团队满意度提升
- [ ] 新人上手时间缩短

---

## 附录

### A. 参考资料

1. 《Clean Architecture》- Robert C. Martin
2. 《Domain-Driven Design》- Eric Evans
3. 《Patterns of Enterprise Application Architecture》- Martin Fowler
4. Google C++ Style Guide
5. C++ Best Practices

### B. 工具清单

1. **构建工具**: CMake 3.15+
2. **编译器**: GCC 9+ / Clang 10+ / MSVC 2019+
3. **测试框架**: Google Test
4. **代码分析**: clang-tidy, cppcheck
5. **性能分析**: perf, valgrind
6. **文档生成**: Doxygen
7. **版本控制**: Git
8. **CI/CD**: GitHub Actions / GitLab CI

### C. 沟通计划

1. **每日站会**: 15分钟，同步进度
2. **周报**: 每周，汇报进展和风险
3. **月度评审**: 月度，展示成果
4. **即时沟通**: Slack/钉钉群

---

**文档版本**: 1.0
**创建时间**: 2026-04-03
**最后更新**: 2026-04-03
**维护者**: PaperCrawler架构团队
