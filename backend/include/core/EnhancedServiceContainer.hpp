#pragma once

#include "ServiceContainer.hpp"
#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <map>

namespace PaperCrawler {

/**
 * @brief 增强的服务容器
 *
 * 新增特性：
 * 1. 装饰器模式支持（AOP）
 * 2. 条件注册（基于环境）
 * 3. 生命周期回调
 * 4. 依赖图可视化
 * 5. 装饰器链
 */
class EnhancedServiceContainer : public ServiceContainer {
public:
    /**
     * @brief 服务装饰器（用于AOP）
     *
     * 示例：
     * @code
     * auto loggingDecorator = [](auto service) {
     *     return std::make_shared<LoggingService<T>>(service);
     * };
     * @endcode
     */
    template<typename Interface>
    using Decorator = std::function<std::shared_ptr<Interface>(
        std::shared_ptr<Interface>
    )>;

    /**
     * @brief 装饰器链
     */
    template<typename Interface>
    using DecoratorChain = std::vector<Decorator<Interface>>;

    /**
     * @brief 带装饰器的服务注册
     *
     * @tparam Interface 接口类型
     * @tparam Implementation 实现类型
     * @param lifetime 服务生命周期
     * @param decorators 装饰器链（按顺序应用）
     *
     * 示例：
     * @code
     * DecoratorChain<ICrawlerService> decorators;
     * decorators.push_back(createLoggingDecorator<ICrawlerService>);
     * decorators.push_back(createCachingDecorator<ICrawlerService>);
     * decorators.push_back(createMetricsDecorator<ICrawlerService>);
     *
     * container.registerServiceWithDecorators<ICrawlerService, TemplateCrawlerModule>(
     *     ServiceLifetime::SINGLETON,
     *     decorators
     * );
     * @endcode
     */
    template<typename Interface, typename Implementation>
    void registerServiceWithDecorators(
        ServiceLifetime lifetime = ServiceLifetime::SINGLETON,
        DecoratorChain<Interface> decorators = {}
    ) {
        // 创建基础实例
        auto baseInstance = std::make_shared<Implementation>();

        // 应用装饰器链
        std::shared_ptr<Interface> decoratedInstance =
            std::static_pointer_cast<Interface>(baseInstance);

        for (auto& decorator : decorators) {
            decoratedInstance = decorator(decoratedInstance);
        }

        // 注册装饰后的实例
        registerInstance<Interface>(decoratedInstance);
    }

    /**
     * @brief 条件注册（基于环境变量）
     *
     * @tparam Interface 接口类型
     * @tparam Implementation 实现类型
     * @param envVar 环境变量名
     * @param expectedValue 期望的环境变量值
     * @param lifetime 服务生命周期
     *
     * 示例：
     * @code
     * // 只有在环境变量 USE_MYSQL=true 时才注册
     * container.registerServiceIf<IDatabase, MySqlDatabase>(
     *     "USE_MYSQL", "true"
     * );
     * @endcode
     */
    template<typename Interface, typename Implementation>
    void registerServiceIf(
        const std::string& envVar,
        const std::string& expectedValue,
        ServiceLifetime lifetime = ServiceLifetime::SINGLETON
    ) {
        const char* actualValue = std::getenv(envVar.c_str());
        if (actualValue && std::string(actualValue) == expectedValue) {
            registerService<Interface, Implementation>(lifetime);
        }
    }

    /**
     * @brief 注册解析回调
     *
     * 在服务被解析时触发回调，可用于：
     * - 性能监控
     * - 日志记录
     * - 依赖追踪
     *
     * @tparam Interface 接口类型
     * @param callback 回调函数
     *
     * 示例：
     * @code
     * container.onResolve<ICrawlerService>([](auto service) {
     *     spdlog::info("ICrawlerService resolved: {}",
     *         service->getName());
     * });
     * @endcode
     */
    template<typename Interface>
    void onResolve(std::function<void(std::shared_ptr<Interface>)> callback) {
        std::string key = getTypeName<Interface>();
        resolveCallbacks_[key].push_back([callback](std::shared_ptr<void> service) {
            callback(std::static_pointer_cast<Interface>(service));
        });
    }

    /**
     * @brief 生成依赖图（DOT格式）
     *
     * 可用于可视化服务依赖关系：
     * @code
     * auto dotGraph = container.generateDependencyGraph();
     * std::ofstream("dependencies.dot") << dotGraph;
     * // 使用 Graphviz 生成图片：
     * // dot -Tpng dependencies.dot -o dependencies.png
     * @endcode
     *
     * @return DOT格式的依赖图字符串
     */
    std::string generateDependencyGraph() const {
        std::stringstream ss;
        ss << "digraph ServiceDependencies {\n";
        ss << "  rankdir=LR;\n";
        ss << "  node [shape=box, style=rounded];\n";
        ss << "  edge [dir=back];\n\n";

        auto stats = getStats();

        // 添加统计信息节点
        ss << "  \"Total Services: " << stats.totalServices << "\"\n";
        ss << "  \"Singletons: " << stats.singletonCount << "\"\n";
        ss << "  \"Transients: " << stats.transientCount << "\"\n";
        ss << "  \"Scoped: " << stats.scopedCount << "\"\n\n";

        // 添加服务节点（这里需要扩展ServiceContainer以跟踪依赖关系）
        ss << "  // Service nodes\n";
        ss << "  // (需要扩展实现以跟踪服务间的依赖关系)\n";

        ss << "}\n";
        return ss.str();
    }

    /**
     * @brief 获取服务装饰器历史
     *
     * @return 装饰器应用历史记录
     */
    struct DecoratorHistory {
        std::string interfaceName;
        std::vector<std::string> decorators;
    };
    std::vector<DecoratorHistory> getDecoratorHistory() const {
        return decoratorHistory_;
    }

    /**
     * @brief 清除所有装饰器历史
     */
    void clearDecoratorHistory() {
        decoratorHistory_.clear();
    }

private:
    /**
     * @brief 记录装饰器应用
     */
    template<typename Interface>
    void recordDecorator(const std::string& decoratorName) {
        std::string interfaceName = getTypeName<Interface>();

        auto it = std::find_if(decoratorHistory_.begin(), decoratorHistory_.end(),
            [&interfaceName](const DecoratorHistory& history) {
                return history.interfaceName == interfaceName;
            });

        if (it != decoratorHistory_.end()) {
            it->decorators.push_back(decoratorName);
        } else {
            DecoratorHistory history;
            history.interfaceName = interfaceName;
            history.decorators.push_back(decoratorName);
            decoratorHistory_.push_back(history);
        }
    }

    template<typename T>
    static std::string getTypeName() {
        return typeid(T).name();
    }

    // 成员变量
    std::map<std::string, std::vector<std::function<void(std::shared_ptr<void>)>>> resolveCallbacks_;
    std::vector<DecoratorHistory> decoratorHistory_;
};

/**
 * @brief 常用装饰器工厂函数
 */
namespace ServiceDecorators {

/**
 * @brief 创建日志装饰器
 */
template<typename Interface>
std::shared_ptr<Interface> createLoggingDecorator(std::shared_ptr<Interface> service) {
    // 实现日志装饰器逻辑
    return service;
}

/**
 * @brief 创建缓存装饰器
 */
template<typename Interface>
std::shared_ptr<Interface> createCachingDecorator(std::shared_ptr<Interface> service) {
    // 实现缓存装饰器逻辑
    return service;
}

/**
 * @brief 创建性能监控装饰器
 */
template<typename Interface>
std::shared_ptr<Interface> createMetricsDecorator(std::shared_ptr<Interface> service) {
    // 实现性能监控装饰器逻辑
    return service;
}

/**
 * @brief 创建重试装饰器
 */
template<typename Interface>
std::shared_ptr<Interface> createRetryDecorator(std::shared_ptr<Interface> service) {
    // 实现重试装饰器逻辑
    return service;
}

} // namespace ServiceDecorators

} // namespace PaperCrawler
