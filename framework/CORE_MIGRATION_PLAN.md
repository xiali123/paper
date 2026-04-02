# PaperCrawler-Core 框架迁移计划

**Phase 1**: 核心框架提取
**时间**: 2-3周
**目标**: 创建独立可复用的PaperCrawler-Core库

---

## 📋 迁移清单

### 阶段1: 目录结构设置（1天）✅

- [x] 创建framework/Core目录结构
- [x] 创建CMakeLists.txt
- [x] 创建README.md
- [ ] 创建示例代码

### 阶段2: 核心代码迁移（3-5天）

#### 2.1 模块基础设施（1天）

**源文件**:
- `backend/include/core/ModuleBase.hpp`
- `backend/include/core/ServiceContainer.hpp`

**目标位置**:
- `framework/Core/include/core/ModuleBase.hpp`
- `framework/Core/include/core/ServiceContainer.hpp`

**改进点**:
- 移除PaperCrawler特定业务逻辑
- 通用化错误类型
- 添加完善的Doxygen注释

**验收标准**:
- [ ] 代码不包含任何业务逻辑
- [ ] 可以独立编译
- [ ] 包含完整API文档

#### 2.2 事件系统（1天）

**源文件**:
- `backend/include/modules/EventBusModule.hpp`
- `backend/include/core/MessageBus.hpp`
- `backend/include/core/UnifiedEventBus.hpp`

**目标位置**:
- `framework/Core/include/core/EventBus.hpp`
- `framework/Core/include/core/MessageBus.hpp`

**改进点**:
- 统一EventBus和MessageBus接口
- 移除模块特定的事件类型
- 添加事件契约验证

**验收标准**:
- [ ] 统一的事件接口
- [ ] 支持任意事件类型
- [ ] 完整的单元测试

#### 2.3 配置管理（0.5天）

**源文件**:
- `backend/include/core/ConfigManager.hpp`

**目标位置**:
- `framework/Core/include/core/ConfigManager.hpp`

**改进点**:
- 支持多种格式（JSON、YAML、TOML）
- 配置验证框架
- 配置热重载

**验收标准**:
- [ ] 支持至少3种配置格式
- [ ] 包含配置验证
- [ ] 完整的测试覆盖

#### 2.4 错误处理（0.5天）

**源文件**:
- `backend/include/core/ErrorHandler.hpp`

**目标位置**:
- `framework/Core/include/utils/ErrorHandler.hpp`

**改进点**:
- 通用化错误类型
- 移除PaperCrawler特定错误代码
- 添加错误恢复策略

**验收标准**:
- [ ] 通用错误类型系统
- [ ] 可扩展的错误处理
- [ ] 完整的文档

#### 2.5 线程池（1天）

**源文件**:
- `backend/include/core/ThreadPool.hpp`

**目标位置**:
- `framework/Core/include/core/ThreadPool.hpp`

**改进点**:
- 添加任务优先级
- 添加性能监控
- 支持协程（可选）

**验收标准**:
- [ ] 动态线程池
- [ ] 任务优先级支持
- [ ] 性能指标收集

#### 2.6 日志系统（0.5天）

**源文件**:
- `backend/include/utils/Logger.hpp`

**目标位置**:
- `framework/Core/include/utils/Logger.hpp`

**改进点**:
- 封装spdlog
- 提供统一接口
- 支持日志级别配置

**验收标准**:
- [ ] 统一的日志接口
- [ ] 支持多个后端
- [ ] 线程安全

#### 2.7 工具类（0.5天）

**源文件**:
- `backend/include/utils/` (所有工具类)

**目标位置**:
- `framework/Core/include/utils/`

**改进点**:
- 筛选通用工具类
- 移除业务特定工具

**验收标准**:
- [ ] 只包含通用工具
- [ ] 完整的测试

### 阶段3: API文档编写（2-3天）

- [ ] 每个公共类添加Doxygen注释
- [ ] 生成API文档（HTML）
- [ ] 编写快速入门教程
- [ ] 编写使用示例

**文档要求**:
- [ ] 100%公共API覆盖
- [ ] 每个类有使用示例
- [ ] 包含最佳实践指南

### 阶段4: 单元测试（3-4天）

- [ ] ModuleBase测试套件
- [ ] ServiceContainer测试套件
- [ ] EventBus测试套件
- [ ] ConfigManager测试套件
- [ ] ErrorHandler测试套件
- [ ] ThreadPool测试套件
- [ ] Logger测试套件

**测试覆盖率要求**: ≥90%

### 阶段5: 示例代码（1-2天）

- [ ] 最小模块示例
- [ ] 依赖注入示例
- [ ] 事件驱动示例
- [ ] 完整应用示例

---

## 🔧 改进优先级

### P0 - 必须完成

1. **移除业务依赖**
   - 移除所有PaperCrawler特定概念
   - 移除硬编码的业务逻辑
   - 通用化所有命名

2. **完整API文档**
   - 每个公共API有Doxygen注释
   - 包含使用示例
   - 生成HTML文档

3. **单元测试**
   - 覆盖率≥90%
   - 所有边界情况测试
   - 性能基准测试

### P1 - 应该完成

1. **性能优化**
   - 虚函数调用优化
   - 编译期优化
   - 零拷贝优化

2. **使用示例**
   - 最小示例
   - 中等复杂度示例
   - 完整应用示例

### P2 - 可以完成

1. **高级特性**
   - 协程支持
   - 插件系统
   - 性能分析工具

---

## 📊 验收标准

### 功能完整性

- [ ] 所有核心组件可独立编译
- [ ] 不依赖任何PaperCrawler业务代码
- [ ] 可以用于任意C++项目

### 代码质量

- [ ] 代码覆盖率≥90%
- [ ] 静态分析无警告
- [ ] 符合C++Core Guidelines

### 文档完整性

- [ ] API文档100%覆盖
- [ ] 快速入门教程
- [ ] 至少3个使用示例

### 性能标准

- [ ] 虚函数开销<5ns
- [ ] 事件延迟<1ms
- [ ] 内存占用<2MB

---

## 🚀 发布计划

### Alpha版本（内部测试）

**目标**: 完成基本功能迁移
**时间**: 1周
**内容**:
- 核心组件迁移完成
- 基本单元测试通过
- 初步API文档

### Beta版本（公开测试）

**目标**: 功能完整，性能优化
**时间**: 2周
**内容**:
- 所有功能完成
- 测试覆盖率≥90%
- 完整文档和示例

### 1.0版本（正式发布）

**目标**: 生产就绪
**时间**: 3周
**内容**:
- 所有P0/P1任务完成
- 性能优化完成
- 生产环境验证

---

## 📁 目录结构

```
framework/Core/
├── include/
│   ├── core/
│   │   ├── ModuleBase.hpp          # 模块基类
│   │   ├── ServiceContainer.hpp    # 服务容器
│   │   ├── EventBus.hpp            # 事件总线
│   │   ├── MessageBus.hpp          # 消息总线
│   │   ├── ConfigManager.hpp       # 配置管理
│   │   ├── ThreadPool.hpp          # 线程池
│   │   └── Types.hpp               # 核心类型定义
│   └── utils/
│       ├── Logger.hpp              # 日志系统
│       ├── ErrorHandler.hpp        # 错误处理
│       └── Helpers.hpp             # 辅助工具
├── src/
│   └── core/
│       ├── ModuleBase.cpp
│       ├── ServiceContainer.cpp
│       ├── EventBus.cpp
│       └── ...
├── tests/
│   ├── ModuleBaseTest.cpp
│   ├── ServiceContainerTest.cpp
│   ├── EventBusTest.cpp
│   └── ...
├── examples/
│   ├── minimal_module/
│   │   └── main.cpp
│   ├── dependency_injection/
│   │   └── main.cpp
│   └── event_driven/
│       └── main.cpp
├── docs/
│   ├── API.md
│   ├── TUTORIAL.md
│   └── BEST_PRACTICES.md
├── CMakeLists.txt
└── README.md
```

---

## 🎯 下一步行动

### 立即可做

1. **开始代码迁移** - 从ModuleBase开始
2. **建立测试框架** - 集成GoogleTest
3. **编写第一个示例** - minimal_module

### 本周完成

4. **完成核心组件迁移**
5. **建立CI/CD**
6. **生成初步文档**

### 本月完成

7. **Beta版本发布**
8. **性能优化**
9. **完整文档**

---

**创建时间**: 2026-04-03
**负责人**: PaperCrawler架构团队
**状态**: 🟡 进行中
