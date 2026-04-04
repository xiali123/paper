# MessageBus集成指南

## 概述

MessageBus是PaperCrawler的模块间通信机制，允许模块在不直接依赖的情况下进行通信。当前只有UserApiModule使用MessageBus（8%覆盖率）。

## 当前使用模式

### UserApiModule示例

```cpp
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"

// 在模块初始化时订阅消息
auto& messageBus = MessageBus::getInstance();
messageBus.registerHandler(MessageType::CUSTOM,
    [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
        // 尝试转换为DatabaseConnectionMessage
        auto dbMsg = std::dynamic_pointer_cast<DatabaseConnectionMessage>(msg);
        if (dbMsg && dbMsg->isSuccess()) {
            database_ = dbMsg->getConnection();
            std::cout << "[ModuleName] ✅ Received database connection from MessageBus!" << std::endl;
        }

        // 返回确认消息
        return std::make_shared<ModuleMessage>(MessageType::ACK);
    }
);
```

## 集成步骤

### 步骤1：添加头文件

在模块的.cpp文件中添加：

```cpp
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"  // 如果需要数据库连接
```

### 步骤2：在Impl类中添加订阅逻辑

```cpp
class MyModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;

    void subscribeToMessages() {
        try {
            auto& messageBus = MessageBus::getInstance();

            messageBus.registerHandler(MessageType::CUSTOM,
                [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                    auto dbMsg = std::dynamic_pointer_cast<DatabaseConnectionMessage>(msg);
                    if (dbMsg && dbMsg->isSuccess()) {
                        database_ = dbMsg->getConnection();
                        std::cout << "[MyModule] ✅ Received database connection" << std::endl;
                    }
                    return std::make_shared<ModuleMessage>(MessageType::ACK);
                }
            );

            std::cout << "[MyModule] Successfully subscribed to MessageBus" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[MyModule] ❌ MessageBus subscription failed: " << e.what() << std::endl;
        }
    }
};
```

### 步骤3：在模块初始化时调用

```cpp
void MyModule::registerRoutes() {
    // 订阅MessageBus消息
    impl_->subscribeToMessages();

    // 注册路由...
}
```

## 推荐集成的模块

基于优先级，建议按以下顺序集成MessageBus：

1. **AuthApiModule** - 高优先级（需要数据库）
2. **PaperApiModule** - 高优先级（需要数据库）
3. **SearchApiModule** - 中优先级（已添加database参数）
4. **ExportApiModule** - 中优先级（已添加database参数）
5. **StatsApiModule** - 中优先级（已添加database参数）
6. **AiApiModule** - 低优先级（可选数据库）
7. **RecommendationApiModule** - 低优先级（可选数据库）

## 优势

✅ **解耦**：模块不直接依赖DatabaseModule
✅ **灵活**：运行时动态配置
✅ **可测试**：易于注入mock数据库
✅ **一致性**：统一的消息传递机制

## 当前状态

- **已集成**：UserApiModule (1/9 = 11%)
- **添加database参数**：6个模块（任务1完成）
- **待集成**：AuthApi, PaperApi, SearchApi, ExportApi, StatsApi, AiApi, RecommendationApi

## 下一步

建议在下一个迭代中：
1. 为AuthApiModule添加MessageBus集成
2. 为PaperApiModule添加MessageBus集成
3. 验证所有模块能正确接收database连接
4. 添加模块间通信的其他消息类型（如用户状态变更、论文更新等）
