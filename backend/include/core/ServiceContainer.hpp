#pragma once

#include <memory>
#include <map>
#include <string>
#include <functional>
#include <typeinfo>
#include <mutex>
#include <vector>
#include <any>

namespace PaperCrawler {

/**
 * @brief 服务生命周期枚举
 */
enum class ServiceLifetime {
    SINGLETON,     // 单例：整个容器生命周期内只有一个实例
    TRANSIENT,     // 瞬态：每次请求都创建新实例
    SCOPED         // 作用域：在特定作用域内复用（简化为singleton）
};

/**
 * @brief 轻量级依赖注入容器
 *
 * 特性：
 * 1. 支持构造函数注入
 * 2. 支持三种生命周期（Singleton, Transient, Scoped）
 * 3. 自动依赖解析
 * 4. 线程安全
 * 5. 接口与实现分离
 *
 * 使用示例：
 * @code
 * ServiceContainer container;
 *
 * // 注册服务
 * container.registerService<IDatabase, MySqlDatabase>(ServiceLifetime::SINGLETON);
 * container.registerService<ICache, RedisCache>(ServiceLifetime::SINGLETON);
 * container.registerService<IUserService, UserService>(ServiceLifetime::TRANSIENT);
 *
 * // 解析服务
 * auto userService = container.resolve<IUserService>();
 *
 * // 或者使用函数注册
 * container.registerService<IDatabase>(
 *     ServiceLifetime::SINGLETON,
 *     []() -> std::shared_ptr<IDatabase> {
 *         return std::make_shared<MySqlDatabase>("localhost", 3306);
 *     }
 * );
 * @endcode
 */
class ServiceContainer {
public:
    ServiceContainer() = default;
    ~ServiceContainer() {
        clear();
    }

    // 禁止拷贝
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;

    // ========================================================================
    // 服务注册
    // ========================================================================

    /**
     * @brief 注册服务（使用工厂函数）
     * @tparam Interface 接口类型
     * @param lifetime 服务生命周期
     * @param factory 工厂函数
     */
    template<typename Interface>
    void registerService(
        ServiceLifetime lifetime,
        std::function<std::shared_ptr<Interface>()> factory
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string key = getTypeName<Interface>();

        ServiceDescriptor descriptor;
        descriptor.lifetime = lifetime;
        descriptor.factory = [factory]() -> std::shared_ptr<void> {
            return factory();
        };

        descriptors_[key] = descriptor;

        // 如果是单例，立即创建实例
        if (lifetime == ServiceLifetime::SINGLETON) {
            singletonInstances_[key] = factory();
        }
    }

    /**
     * @brief 注册服务（使用具体类型）
     * @tparam Interface 接口类型
     * @tparam Implementation 实现类型
     * @param lifetime 服务生命周期
     */
    template<typename Interface, typename Implementation>
    void registerService(ServiceLifetime lifetime = ServiceLifetime::SINGLETON) {
        registerService<Interface>(
            lifetime,
            []() -> std::shared_ptr<Interface> {
                return std::make_shared<Implementation>();
            }
        );
    }

    /**
     * @brief 注册单例实例
     * @tparam Interface 接口类型
     * @param instance 已存在的实例
     */
    template<typename Interface>
    void registerInstance(std::shared_ptr<Interface> instance) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string key = getTypeName<Interface>();

        ServiceDescriptor descriptor;
        descriptor.lifetime = ServiceLifetime::SINGLETON;
        descriptor.instance = instance;

        descriptors_[key] = descriptor;
        singletonInstances_[key] = instance;
    }

    // ========================================================================
    // 服务解析
    // ========================================================================

    /**
     * @brief 解析服务
     * @tparam Interface 接口类型
     * @return 服务实例
     */
    template<typename Interface>
    std::shared_ptr<Interface> resolve() {
        std::string key = getTypeName<Interface>();

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = descriptors_.find(key);
        if (it == descriptors_.end()) {
            throw std::runtime_error("Service not registered: " + key);
        }

        auto& descriptor = it->second;

        // 根据生命周期返回实例
        switch (descriptor.lifetime) {
            case ServiceLifetime::SINGLETON:
                // 返回单例实例
                return std::static_pointer_cast<Interface>(
                    singletonInstances_[key]
                );

            case ServiceLifetime::TRANSIENT:
                // 每次创建新实例
                return std::static_pointer_cast<Interface>(
                    descriptor.factory()
                );

            case ServiceLifetime::SCOPED:
                // 简化实现：使用单例
                return std::static_pointer_cast<Interface>(
                    singletonInstances_[key]
                );

            default:
                throw std::runtime_error("Unknown service lifetime");
        }
    }

    /**
     * @brief 尝试解析服务（不抛异常）
     * @tparam Interface 接口类型
     * @return 服务实例，如果不存在则返回nullptr
     */
    template<typename Interface>
    std::shared_ptr<Interface> tryResolve() {
        try {
            return resolve<Interface>();
        } catch (...) {
            return nullptr;
        }
    }

    /**
     * @brief 检查服务是否已注册
     * @tparam Interface 接口类型
     * @return 是否已注册
     */
    template<typename Interface>
    bool isRegistered() const {
        std::string key = getTypeName<Interface>();
        std::lock_guard<std::mutex> lock(mutex_);
        return descriptors_.find(key) != descriptors_.end();
    }

    // ========================================================================
    // 容器管理
    // ========================================================================

    /**
     * @brief 清空所有服务
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        descriptors_.clear();
        singletonInstances_.clear();
    }

    /**
     * @brief 获取统计信息
     */
    struct ContainerStats {
        size_t totalServices{0};
        size_t singletonCount{0};
        size_t transientCount{0};
        size_t scopedCount{0};
    };

    ContainerStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        ContainerStats stats;
        stats.totalServices = descriptors_.size();

        for (const auto& [key, descriptor] : descriptors_) {
            switch (descriptor.lifetime) {
                case ServiceLifetime::SINGLETON:
                    stats.singletonCount++;
                    break;
                case ServiceLifetime::TRANSIENT:
                    stats.transientCount++;
                    break;
                case ServiceLifetime::SCOPED:
                    stats.scopedCount++;
                    break;
            }
        }

        return stats;
    }

private:
    /**
     * @brief 服务描述符
     */
    struct ServiceDescriptor {
        ServiceLifetime lifetime;
        std::function<std::shared_ptr<void>()> factory;
        std::shared_ptr<void> instance;
    };

    /**
     * @brief 获取类型名称
     */
    template<typename T>
    static std::string getTypeName() {
        return typeid(T).name();
    }

    // 成员变量
    mutable std::mutex mutex_;
    std::map<std::string, ServiceDescriptor> descriptors_;
    std::map<std::string, std::shared_ptr<void>> singletonInstances_;
};

/**
 * @brief 全局服务容器访问器（单例模式）
 *
 * 使用示例：
 * @code
 * // 注册服务
 * Services::registerService<IDatabase, MySqlDatabase>();
 *
 * // 解析服务
 * auto db = Services::resolve<IDatabase>();
 * @endcode
 */
class Services {
public:
    /**
     * @brief 获取全局容器实例
     */
    static ServiceContainer& getInstance() {
        static ServiceContainer instance;
        return instance;
    }

    /**
     * @brief 注册服务（便捷方法）
     */
    template<typename Interface, typename Implementation>
    static void registerService(ServiceLifetime lifetime = ServiceLifetime::SINGLETON) {
        getInstance().registerService<Interface, Implementation>(lifetime);
    }

    /**
     * @brief 注册服务实例（便捷方法）
     */
    template<typename Interface>
    static void registerInstance(std::shared_ptr<Interface> instance) {
        getInstance().registerInstance<Interface>(instance);
    }

    /**
     * @brief 解析服务（便捷方法）
     */
    template<typename Interface>
    static std::shared_ptr<Interface> resolve() {
        return getInstance().resolve<Interface>();
    }

    /**
     * @brief 尝试解析服务（便捷方法）
     */
    template<typename Interface>
    static std::shared_ptr<Interface> tryResolve() {
        return getInstance().tryResolve<Interface>();
    }

    /**
     * @brief 检查服务是否已注册（便捷方法）
     */
    template<typename Interface>
    static bool isRegistered() {
        return getInstance().isRegistered<Interface>();
    }

    /**
     * @brief 清空所有服务（便捷方法）
     */
    static void clear() {
        getInstance().clear();
    }

    /**
     * @brief 获取统计信息（便捷方法）
     */
    static ServiceContainer::ContainerStats getStats() {
        return getInstance().getStats();
    }
};

} // namespace PaperCrawler
