# PaperCrawler 热插拔架构完整方案

## 📋 文档导航

本方案包含以下完整文档，请按顺序阅读：

### 1. [完整架构设计](./HOT_PLUGGABLE_ARCHITECTURE_DESIGN.md) ⭐ 必读
**内容：**
- 架构概述和设计原则
- 当前问题分析
- 目标架构设计（含架构图）
- 核心组件详细设计
- 模块自动路由注册机制
- 配置文件设计
- 实施步骤和测试验证方法

**适合：** 架构师、技术负责人、需要全面了解方案的开发者

### 2. [实施指南](./HOT_PLUGGABLE_IMPLEMENTATION_GUIDE.md) 📖 实操
**内容：**
- 分步骤实施指南
- 详细的代码示例
- 每个步骤的代码修改
- 编译和测试方法
- 验证清单

**适合：** 负责实施开发的工程师

### 3. [快速参考](./HOT_PLUGGABLE_QUICK_REFERENCE.md) 🚀 快查
**内容：**
- 架构概览
- 快速开始指南
- 路由注册模式
- 依赖管理
- 错误处理和调试
- 最佳实践
- 常见问题解答

**适合：** 日常开发参考、新手入门

### 4. [迁移脚本](./migrate_to_hot_pluggable.sh) 🛠️ 自动化
**功能：**
- 自动备份现有文件
- 自动更新接口定义
- 自动生成新代码
- 交互式迁移流程

**适合：** 需要快速迁移现有代码

### 5. [测试脚本](./test_hot_pluggable.sh) ✅ 验证
**功能：**
- 自动化测试套件
- 功能验证
- 性能测试
- 生成测试报告

**适合：** 验证架构实施效果

---

## 🎯 核心目标

### 主要指标

| 指标 | 当前 | 目标 | 改进 |
|------|------|------|------|
| main.cpp行数 | 3890 | <300 | ↓ 92% |
| 编译时间 | 120s | 30s | ↓ 75% |
| 新增模块工作量 | 修改main.cpp | 仅编译DLL | ⬇️ 90% |
| 代码可维护性 | 低 | 高 | ⬆️ 显著提升 |

### 核心特性

✅ **完全解耦** - main.cpp不包含任何业务逻辑
✅ **自动发现** - 自动扫描并加载所有模块DLL
✅ **热插拔** - 运行时加载/卸载模块无需重启
✅ **故障隔离** - 单个模块失败不影响其他模块
✅ **零配置** - 模块名自动映射到路由前缀
✅ **依赖管理** - 支持模块依赖声明和自动排序

---

## 🚀 快速开始

### 5分钟快速体验

```bash
# 1. 阅读架构设计文档
cat HOT_PLUGGABLE_ARCHITECTURE_DESIGN.md

# 2. 运行迁移脚本
bash migrate_to_hot_pluggable.sh

# 3. 编译项目
cd build
cmake --build . --config Release

# 4. 运行服务器
cd Release
./src/core/main.exe

# 5. 测试API
curl http://localhost:8080/api/papers
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"test"}'

# 6. 运行测试套件
cd ../..
bash test_hot_pluggable.sh
```

---

## 📊 架构对比

### 传统架构
```
main.cpp (3890行)
  ├── 1000+ 行硬编码路由注册
  ├── 500+ 行手动创建模块实例
  ├── 300+ 行手动依赖注入
  └── 2000+ 行业务逻辑

问题：
❌ 修改路由需要改main.cpp
❌ 新增模块需要改main.cpp
❌ 模块无法独立开发测试
❌ 编译时间长（全量重编译）
❌ 代码耦合严重
```

### 热插拔架构
```
main.cpp (<300行)
  ├── 加载配置 (50行)
  ├── 初始化日志 (30行)
  ├── PluginManager::scanAndLoadModules() (20行)
  ├── 自动路由注册 (10行)
  └── 启动HTTP服务器 (50行)

优势：
✅ 添加模块只需编译DLL
✅ 模块独立开发测试
✅ 支持热插拔
✅ 编译速度快
✅ 代码清晰易维护
```

---

## 📦 已编译模块

当前已有7个业务模块成功编译：

```
build/Release/modules/dynamic/Release/
├── libAiApiModule.dll              → /api/ai/*
├── libAuthApiModule.dll            → /api/auth/*
├── libExportApiModule.dll          → /api/export/*
├── libRecommendationApiModule.dll  → /api/recommendation/*
├── libSearchApiModule.dll          → /api/search/*
└── libUserApiModule.dll            → /api/users/*
```

---

## 🔧 技术栈

### 核心技术
- **C++17** - 核心语言
- **CMake** - 构建系统
- **spdlog** - 日志库
- **nlohmann/json** - JSON处理
- **Windows API / POSIX** - 动态库加载

### 架构模式
- **插件模式** - 动态模块加载
- **依赖注入** - 模块间依赖管理
- **策略模式** - 路由处理器
- **工厂模式** - 模块实例创建
- **观察者模式** - 模块生命周期管理

---

## 📖 使用场景

### 场景1: 添加新的API模块

```cpp
// 1. 创建模块类
class NewApiModule : public IModule {
    void registerRoutes() override {
        auto prefix = getRoutePrefix(); // 自动生成: /api/new
        router.get(prefix + "/items", [this](auto& req) {
            return handleGetItems(req);
        });
    }
};

// 2. 编译为DLL
cmake --build . --target NewApiModule

// 3. 放入modules目录
cp libNewApiModule.dll modules/dynamic/

// 4. 重启服务器（自动加载）
./main.exe
```

### 场景2: 模块间依赖

```cpp
// AnalysisModule依赖DatabaseModule和CacheModule
class AnalysisModule : public IModule {
    std::vector<ModuleDependency> getDependencies() const override {
        return {
            {"DatabaseModule", "1.0.0", true},
            {"CacheModule", "1.0.0", false}
        };
    }

    void setDependencies(const std::map<std::string, IModule*>& deps) override {
        databaseModule_ = deps["DatabaseModule"];
        cacheModule_ = deps["CacheModule"];
    }
};
```

### 场景3: 路径参数处理

```cpp
// GET /api/papers/:id
router.get(prefix + "/papers/:id", [this](auto& req) {
    std::string paperId = req.pathParams["id"];
    return handleGetPaper(paperId);
});
```

---

## 🧪 测试验证

### 自动化测试

```bash
# 运行完整测试套件
bash test_hot_pluggable.sh

# 测试内容：
# ✓ main.cpp行数检查
# ✓ 模块DLL文件检查
# ✓ 接口定义检查
# ✓ 编译测试
# ✓ 服务器启动测试
# ✓ 模块加载测试
# ✓ 路由注册测试
# ✓ API路由测试
# ✓ 性能测试
```

### 手动测试

```bash
# 1. 测试认证API
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"test"}'

# 2. 测试论文API
curl http://localhost:8080/api/papers
curl http://localhost:8080/api/papers/1

# 3. 测试用户API
curl http://localhost:8080/api/users

# 4. 测试AI API
curl -X POST http://localhost:8080/api/ai/chat \
  -H "Content-Type: application/json" \
  -d '{"message":"Hello"}'
```

---

## 📈 实施路线图

### 阶段1: 基础架构（1-2天）
- [x] 增强IModule接口
- [x] 增强Router
- [x] 增强PluginManager
- [x] 创建新main.cpp

### 阶段2: 模块适配（2-3天）
- [ ] 更新AuthApiModule
- [ ] 更新PaperApiModule
- [ ] 更新UserApiModule
- [ ] 更新AiApiModule
- [ ] 更新其他业务模块

### 阶段3: 测试验证（1-2天）
- [ ] 单元测试
- [ ] 集成测试
- [ ] 性能测试
- [ ] 压力测试

### 阶段4: 优化完善（1天）
- [ ] 配置文件加载
- [ ] 监控指标
- [ ] 文档完善
- [ ] 代码优化

**总估计时间：** 5-8天

---

## 🎓 学习资源

### 推荐阅读顺序

1. **新手** → 快速参考 → 实施指南 → 架构设计
2. **架构师** → 架构设计 → 实施指南 → 快速参考
3. **开发者** → 实施指南 → 快速参考 → 架构设计

### 相关文档链接

- [CMake官方文档](https://cmake.org/documentation/)
- [C++插件开发最佳实践](https://isocpp.org/)
- [微服务架构设计模式](https://microservices.io/patterns/microservices.html)

---

## 🤝 贡献指南

### 代码规范

- 遵循C++17标准
- 使用Google代码风格
- 添加详细的注释
- 编写单元测试

### 提交流程

1. Fork项目
2. 创建特性分支
3. 提交变更
4. 推送到分支
5. 创建Pull Request

---

## ❓ 常见问题

### Q1: 这个架构适合什么规模的项目？

**A:** 适合中大型项目，特别是：
- 模块数量 > 5个
- 团队规模 > 3人
- 需要频繁添加新功能
- 需要独立开发和测试

### Q2: 性能开销有多大？

**A:** 性能开销很小：
- 模块加载：一次性，启动时完成（< 5秒）
- 路由匹配：O(1)哈希查找，< 1ms
- 函数调用：虚函数调用，< 1μs

### Q3: 可以混合使用静态模块和动态模块吗？

**A:** 可以，架构完全支持：
- 核心基础设施模块（如DatabaseModule）可以是静态链接
- 业务API模块使用动态加载
- Router统一处理所有路由

### Q4: 如何调试模块加载问题？

**A:** 使用以下方法：
1. 启用debug级别日志
2. 检查DLL依赖（使用`ldd`或`Dependency Walker`）
3. 查看加载日志中的错误信息
4. 使用调试器附加到进程

### Q5: 支持跨平台吗？

**A:** 完全支持：
- Windows: 使用`.dll`和`LoadLibrary`
- Linux: 使用`.so`和`dlopen`
- macOS: 使用`.dylib`和`dlopen`

---

## 📞 获取帮助

### 文档资源
- 📖 [完整架构设计](./HOT_PLUGGABLE_ARCHITECTURE_DESIGN.md)
- 📝 [实施指南](./HOT_PLUGGABLE_IMPLEMENTATION_GUIDE.md)
- 🚀 [快速参考](./HOT_PLUGGABLE_QUICK_REFERENCE.md)

### 工具脚本
- 🛠️ [迁移脚本](./migrate_to_hot_pluggable.sh)
- ✅ [测试脚本](./test_hot_pluggable.sh)

### 日志文件
- 服务器日志：`logs/papercrawler.log`
- 构建日志：`build/build.log`

---

## 📝 更新日志

### v1.0.0 (2026-04-04)
- ✅ 初始架构设计
- ✅ 完整文档编写
- ✅ 迁移脚本开发
- ✅ 测试脚本开发

---

## 📄 许可证

本项目采用MIT许可证。详见LICENSE文件。

---

## 🙏 致谢

感谢所有为这个架构设计做出贡献的开发者和架构师。

---

**文档版本:** 1.0.0
**最后更新:** 2026-04-04
**维护者:** Backend Architect
**状态:** ✅ 设计完成，待实施
