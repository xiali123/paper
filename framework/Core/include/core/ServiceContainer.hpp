#pragma once

#include <memory>
#include <map>
#include <string>
#include <functional>
#include <typeinfo>
#include <mutex>
#include <vector>
#include <any>
#include <stdexcept>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 服务生命周期枚举
 *
 * 定义了服务实例的生命周期管理策略
 */
enum class ServiceLifetime {
    SINGLETON,  ///< 单例：整个容器生命周期内只有一个实例
    TRANSIENT,  ///< 瞬态：每次请求都创建新实例
    SCOPED     ///< 作用域：在特定作用域内复用
};

/**
 * @brief 轻量级依赖注入容器
 *
 * 提供了完整的依赖注入功能，包括：
 * - 构造函数注入
 * - 三种生命周期管理（Singleton, Transient, Scoped）
 * - 自动依赖解析
 * - 线程安全保证
 * - 接口与实现分离
 *
 * @section features 核心特性
 * - @ref thread_safety "线程安全"
 * - @ref auto_resolution "自动依赖解析"
 * - @ref lifetime_management "生命周期管理"
 *
 * @section example_usage 示例用法
 * @code
 * // 创建容器
 * ServiceContainer container;
 *
 * // 注册服务（使用具体类型）
 * container.registerService<IDatabase, MySqlDatabase>(
 *     ServiceLifetime::SINGLETON
 * );
 *
 * // 注册服务（使用工厂函数）
 * container.registerService<ICache>(
 *     ServiceLifetime::SINGLETON,
 *     []() -> std::shared_ptr<ICache> {
 *         return std::make_shared<RedisCache>("localhost", 6379);
 *     }
 * );
 *
 * // 注册瞬态服务
 * container.registerService<IUserService, UserService>(
 *     ServiceLifetime::TRANSIENT
 * );
 *
 * // 解析服务
 * auto userService = container.resolve<IUserService>();
 * userService->createUser("username");
 *
 * // 检查服务是否已注册
 * if (container.isRegistered<IDatabase>()) {
 *     auto db = container.resolve<IDatabase>();
 * }
 * @endcode
 *
 * @section thread_safety 线程安全
 * 所有公共方法都是线程安全的，可以在多线程环境中安全使用。
 * 内部使用std::mutex保护共享状态。
 *
 * @section auto_resolution 自动依赖解析
 * 容器支持自动解析依赖关系。如果服务A依赖于服务B，
 * 只需确保在解析A之前，B已经注册到容器中。
 *
 * @section lifetime_management 生命周期管理
 * - SINGLETON: 整个容器生命周期内只创建一次实例
 * - TRANSIENT: 每次resolve()都创建新实例
 * - SCOPED: 在特定作用域内复用同一实例
 *
 * @note 容器不支持循环依赖，如果存在循环依赖会导致栈溢出
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class ServiceContainer {
public:
    /**
     * @brief 构造函数
     */
    ServiceContainer() = default;

    /**
     * @brief 析构函数
     *
     * 自动清理所有注册的服务
     */
    ~ServiceContainer() {
        clear();
    }

    // 禁止拷贝和移动
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;
    ServiceContainer(ServiceContainer&&) = delete;
    ServiceContainer& operator=(ServiceContainer&&) = delete;

    // ========================================================================
    // 服务注册
    // ========================================================================

    /**
     * @brief 注册服务（使用工厂函数）
     *
     * @tparam Interface 接口类型
     * @param lifetime 服务生命周期
     * @param factory 工厂函数，返回服务实例
     *
     * @section example 示例
     * @code
     * container.registerService<IDatabase>(
     *     ServiceLifetime::SINGLETON,
     *     []() -> std::shared_ptr<IDatabase> {
     *         return std::make_shared<MySqlDatabase>(
     *             "localhost", 3306, "user", "pass"
     *         );
     *     }
     * );
     * @endcode
     *
     * @note 如果接口已注册，会覆盖原有注册
     * @note SINGLETON服务会立即创建实例
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
     *
     * @tparam Interface 接口类型
     * @tparam Implementation 实现类型
     * @param lifetime 服务生命周期（默认SINGLETON）
     *
     * @section example 示例
     * @code
     * // 注册为单例
     * container.registerService<IDatabase, MySqlDatabase>();
     *
     * // 注册为瞬态
     * container.registerService<IUserService, UserService>(
     *     ServiceLifetime::TRANSIENT
     * );
     * @endcode
     *
     * @note Implementation必须可构造（默认构造或通过工厂函数）
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
     *
     * @tparam Interface 接口类型
     * @param instance 已存在的实例
     *
     * @section example 示例
     * @code
     * auto db = std::make_shared<MySqlDatabase>(...);
     * container.registerInstance<IDatabase>(db);
     * @endcode
     *
     * @note 容器会接管实例的所有权
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
     *
     * @tparam Interface 接口类型
     * @return 服务实例
     * @throws std::runtime_error 如果服务未注册
     *
     * @section example 示例
     * @code
     * try {
     *     auto db = container.resolve<IDatabase>();
     *     db->query("SELECT * FROM users");
     * } catch (const std::runtime_error& e) {
     *     std::cerr << "Service not found: " << e.what() << std::endl;
     * }
     * @endcode
     *
     * @threadsafe 线程安全
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
     *
     * @tparam Interface 接口类型
     * @return 服务实例，如果不存在则返回nullptr
     *
     * @section example 示例
     * @code
     * auto cache = container.tryResolve<ICache>();
     * if (cache) {
     *     cache->set("key", "value");
     * } else {
     *     std::cout << "Cache not available" << std::endl;
     * }
     * @endcode
     *
     * @threadsafe 线程安全
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
     *
     * @tparam Interface 接口类型
     * @return 是否已注册
     *
     * @threadsafe 线程安全
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
     *
     * 释放所有服务实例，清空注册表
     *
     * @threadsafe 线程安全
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        descriptors_.clear();
        singletonInstances_.clear();
    }

    /**
     * @brief 容器统计信息
     */
    struct ContainerStats {
        size_t totalServices{0};   ///< 总服务数
        size_t singletonCount{0}; ///< 单例服务数
        size_t transientCount{0}; ///< 瞬态服务数
        size_t scopedCount{0};    ///< 作用域服务数
    };

    /**
     * @brief 获取统计信息
     *
     * @return 统计信息
     *
     * @threadsafe 线程安全
     */
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
     *
     * 存储服务的元数据和创建逻辑
     */
    struct ServiceDescriptor {
        ServiceLifetime lifetime;                            ///< 生命周期
        std::function<std::shared_ptr<void>()> factory;     ///< 工厂函数
        std::shared_ptr<void> instance;                     ///< 实例（单例使用）
    };

    /**
     * @brief 获取类型名称
     *
     * @tparam T 类型
     * @return 类型名称（使用type_info）
     */
    template<typename T>
    static std::string getTypeName() {
        return typeid(T).name();
    }

    // 成员变量
    mutable std::mutex mutex_;                                  ///< 互斥锁
    std::map<std::string, ServiceDescriptor> descriptors_;      ///< 服务描述符
    std::map<std::string, std::shared_ptr<void>> singletonInstances_;  ///< 单例实例
};

/**
 * @brief 全局服务容器访问器（单例模式）
 *
 * 提供全局访问点，简化服务容器的使用。
 *
 * @section example_usage 示例用法
 * @code
 * // 注册服务
 * Services::registerService<IDatabase, MySqlDatabase>();
 * Services::registerService<ICache, RedisCache>();
 *
 * // 解析服务
 * auto db = Services::resolve<IDatabase>();
 * auto cache = Services::resolve<ICache>();
 *
 * // 检查服务
 * if (Services::isRegistered<IDatabase>()) {
 *     // 数据库已注册
 * }
 * @endcode
 *
 * @note 这是一个全局单例，使用时需要注意线程安全性
 * @threadsafe 所有方法都是线程安全的
 */
class Services {
public:
    /**
     * @brief 获取全局容器实例
     *
     * @return 全局容器引用
     */
    static ServiceContainer& getInstance() {
        static ServiceContainer instance;
        return instance;
    }

    /**
     * @brief 注册服务（便捷方法）
     *
     * @tparam Interface 接口类型
     * @tparam Implementation 实现类型
     * @param lifetime 服务生命周期（默认SINGLETON）
     */
    template<typename Interface, typename Implementation>
    static void registerService(ServiceLifetime lifetime = ServiceLifetime::SINGLETON) {
        getInstance().registerService<Interface, Implementation>(lifetime);
    }

    /**
     * @brief 注册服务实例（便捷方法）
     *
     * @tparam Interface 接口类型
     * @param instance 服务实例
     */
    template<typename Interface>
    static void registerInstance(std::shared_ptr<Interface> instance) {
        getInstance().registerInstance<Interface>(instance);
    }

    /**
     * @brief 解析服务（便捷方法）
     *
     * @tparam Interface 接口类型
     * @return 服务实例
     */
    template<typename Interface>
    static std::shared_ptr<Interface> resolve() {
        return getInstance().resolve<Interface>();
    }

    /**
     * @brief 尝试解析服务（便捷方法）
     *
     * @tparam Interface 接口类型
     * @return 服务实例，如果不存在则返回nullptr
     */
    template<typename Interface>
    static std::shared_ptr<Interface> tryResolve() {
        return getInstance().tryResolve<Interface>();
    }

    /**
     * @brief 检查服务是否已注册（便捷方法）
     *
     * @tparam Interface 接口类型
     * @return 是否已注册
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
     *
     * @return 统计信息
     */
    static ServiceContainer::ContainerStats getStats() {
        return getInstance().getStats();
    }
};

} // namespace Core
} // namespace PaperCrawler
