#pragma once

#include <memory>
#include <map>
#include <string>
#include <typeindex>
#include <stdexcept>
#include <iostream>

namespace PaperCrawler {

// ============================================================================
// 服务定位器/依赖注入系统
// ============================================================================

/**
 * @brief 服务定位器（Service Locator）
 *
 * 提供简单的依赖注入机制，支持注册和解析服务实例。
 *
 * 使用示例：
 * ```cpp
 * // 注册服务
 * auto logging = std::make_shared<LoggingModule>();
 * Services::registerService<LoggingModule>(logging);
 *
 * // 解析服务
 * auto logging = Services::resolve<LoggingModule>();
 * if (logging) {
 *     logging->info("Service resolved");
 * }
 * ```
 */
class Services {
public:
    /**
     * @brief 注册服务
     *
     * @tparam T 服务类型
     * @param service 服务实例（shared_ptr）
     */
    template<typename T>
    static void registerService(std::shared_ptr<T> service) {
        std::type_index key(typeid(T));
        services()[key] = service;
        std::cout << "[Services] Registered service: " << typeid(T).name() << std::endl;
    }

    /**
     * @brief 解析服务
     *
     * @tparam T 服务类型
     * @return 服务实例的shared_ptr，如果未注册则返回nullptr
     */
    template<typename T>
    static std::shared_ptr<T> resolve() {
        std::type_index key(typeid(T));
        auto it = services().find(key);

        if (it == services().end()) {
            std::cout << "[Services] Warning: Service not registered: "
                      << typeid(T).name() << std::endl;
            return nullptr;
        }

        return std::static_pointer_cast<T>(it->second);
    }

    /**
     * @brief 检查服务是否已注册
     *
     * @tparam T 服务类型
     * @return true如果服务已注册
     */
    template<typename T>
    static bool isRegistered() {
        std::type_index key(typeid(T));
        return services().find(key) != services().end();
    }

    /**
     * @brief 注销服务
     *
     * @tparam T 服务类型
     */
    template<typename T>
    static void unregister() {
        std::type_index key(typeid(T));
        auto it = services().find(key);

        if (it != services().end()) {
            services().erase(it);
            std::cout << "[Services] Unregistered service: " << typeid(T).name() << std::endl;
        }
    }

    /**
     * @brief 清除所有已注册的服务
     */
    static void clear() {
        services().clear();
        std::cout << "[Services] All services cleared" << std::endl;
    }

    /**
     * @brief 获取已注册服务数量
     */
    static size_t serviceCount() {
        return services().size();
    }

private:
    /**
     * @brief 获取服务容器（单例）
     */
    static std::map<std::type_index, std::shared_ptr<void>>& services() {
        static std::map<std::type_index, std::shared_ptr<void>> instance;
        return instance;
    }

    // 禁止实例化
    Services() = default;
    ~Services() = default;
    Services(const Services&) = delete;
    Services& operator=(const Services&) = delete;
};

// ============================================================================
// 服务注册助手（RAII模式）
// ============================================================================

/**
 * @brief 服务注册助手
 *
 * 使用RAII模式自动管理服务生命周期。
 * 析构时自动注销服务。
 *
 * 使用示例：
 * ```cpp
 * {
 *     ServiceRegistration<LoggingModule> reg(logging);
 *     // 在此作用域内服务可用
 * }
 * // 离开作用域，服务自动注销
 * ```
 */
template<typename T>
class ServiceRegistration {
public:
    explicit ServiceRegistration(std::shared_ptr<T> service) {
        Services::registerService<T>(service);
    }

    ~ServiceRegistration() {
        Services::unregister<T>();
    }

    // 禁止拷贝和移动
    ServiceRegistration(const ServiceRegistration&) = delete;
    ServiceRegistration& operator=(const ServiceRegistration&) = delete;
    ServiceRegistration(ServiceRegistration&&) = delete;
    ServiceRegistration& operator=(ServiceRegistration&&) = delete;
};

} // namespace PaperCrawler
