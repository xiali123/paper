#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <list>
#include <deque>
#include <array>
#include <tuple>
#include <memory>
#include <type_traits>
#include <functional>
#include <sstream>
#include <iomanip>
#include <cxxabi.h>
#include <typeinfo>

namespace PaperCrawler {
namespace Core {

/**
 * @brief TypeHelper - 通用类型辅助工具集
 *
 * 提供了完整的类型处理功能，包括：
 * - 类型信息查询
 * - 类型转换
 * - 类型检查
 * - 类型擦除
 * - 类型特征检测
 * - 模板元编程辅助
 *
 * @section features 核心特性
 * - @ref type_info "类型信息"
 * - @ref type_conversion "类型转换"
 * - @ref type_check "类型检查"
 * - @ref type_traits "类型特征"
 *
 * @section example_usage 示例用法
 * @code
 * // 获取类型名称
 * std::string typeName = TypeHelper::getTypeName<int>();
 * // "int"
 *
 * std::string typeName2 = TypeHelper::getTypeName(3.14);
 * // "double"
 *
 * // 检查类型
 * bool isInt = TypeHelper::isIntegral<int>::value;
 * // true
 *
 * bool isPointer = TypeHelper::isPointer<int*>::value;
 * // true
 *
 * // 转换类型
 * std::string str = TypeHelper::toString(123);
 * // "123"
 *
 * int value = TypeHelper::fromString<int>("456");
 * // 456
 *
 * // 类型列表操作
 * using MyTypes = TypeList<int, double, std::string>;
 * constexpr size_t count = MyTypes::size;
 * // 3
 *
 * bool hasString = MyTypes::contains<std::string>;
 * // true
 * @endcode
 *
 * @threadsafe 所有静态方法都是线程安全的（无共享状态）
 */
class TypeHelper {
public:
    // ========================================================================
    // 类型信息
    // ========================================================================

    /**
     * @brief 获取类型名称（可读格式）
     *
     * @tparam T 类型
     * @return 类型名称
     *
     * @section example 示例
     * @code
     * std::string name = TypeHelper::getTypeName<int>();
     * // "int"
     *
     * std::string name2 = TypeHelper::getTypeName<std::vector<int>>();
     * // "std::vector<int>"
     * @endcode
     */
    template<typename T>
    static std::string getTypeName() {
        return getTypeName<T>(typeid(T));
    }

    /**
     * @brief 从type_info获取类型名称
     *
     * @param info type_info对象
     * @return 类型名称
     */
    template<typename T>
    static std::string getTypeName(const std::type_info& info) {
        int status = 0;
        char* demangled = abi::__cxa_demangle(info.name(), nullptr, nullptr, &status);

        if (status == 0 && demangled) {
            std::string result(demangled);
            free(demangled);
            return result;
        }

        return info.name();
    }

    /**
     * @brief 从对象获取类型名称
     *
     * @param obj 对象
     * @return 类型名称
     *
     * @section example 示例
     * @code
     * std::vector<int> vec;
     * std::string name = TypeHelper::getTypeName(vec);
     * // "std::vector<int>"
     * @endcode
     */
    template<typename T>
    static std::string getTypeName(const T& obj) {
        return getTypeName<T>(typeid(obj));
    }

    // ========================================================================
    // 类型大小
    // ========================================================================

    /**
     * @brief 获取类型大小（字节）
     *
     * @tparam T 类型
     * @return 类型大小
     *
     * @section example 示例
     * @code
     * constexpr size_t intSize = TypeHelper::getSize<int>();
     * // 4
     *
     * constexpr size_t ptrSize = TypeHelper::getSize<int*>();
     * // 8 (64-bit系统)
     * @endcode
     */
    template<typename T>
    static constexpr size_t getSize() {
        return sizeof(T);
    }

    // ========================================================================
    // 类型对齐
    // ========================================================================

    /**
     * @brief 获取类型对齐要求（字节）
     *
     * @tparam T 类型
     * @return 对齐要求
     *
     * @section example 示例
     * @code
     * constexpr size_t intAlign = TypeHelper::getAlignment<int>();
     * // 4
     *
     * constexpr size_t doubleAlign = TypeHelper::getAlignment<double>();
     * // 8
     * @endcode
     */
    template<typename T>
    static constexpr size_t getAlignment() {
        return alignof(T);
    }

    // ========================================================================
    // 类型转换
    // ========================================================================

    /**
     * @brief 将任意类型转换为字符串
     *
     * @tparam T 类型
     * @param value 值
     * @return 字符串表示
     *
     * @section example 示例
     * @code
     * std::string str1 = TypeHelper::toString(123);
     * // "123"
     *
     * std::string str2 = TypeHelper::toString(3.14);
     * // "3.140000"
     *
     * std::string str3 = TypeHelper::toString(true);
     * // "true"
     * @endcode
     */
    template<typename T>
    static std::string toString(const T& value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    /**
     * @brief 从字符串转换为任意类型
     *
     * @tparam T 目标类型
     * @param str 字符串
     * @return 转换后的值
     *
     * @section example 示例
     * @code
     * int i = TypeHelper::fromString<int>("123");
     * // 123
     *
     * double d = TypeHelper::fromString<double>("3.14");
     * // 3.14
     *
     * bool b = TypeHelper::fromString<bool>("true");
     * // true
     * @endcode
     */
    template<typename T>
    static T fromString(const std::string& str) {
        std::istringstream iss(str);
        T value;
        iss >> value;
        return value;
    }

    // ========================================================================
    // 类型特征检测
    // ========================================================================

    /// @brief 检查是否为整数类型
    template<typename T>
    using isIntegral = std::is_integral<T>;

    /// @brief 检查是否为浮点类型
    template<typename T>
    using isFloatingPoint = std::is_floating_point<T>;

    /// @brief 检查是否为数值类型
    template<typename T>
    using isArithmetic = std::is_arithmetic<T>;

    /// @brief 检查是否为指针类型
    template<typename T>
    using isPointer = std::is_pointer<T>;

    /// @brief 检查是否为引用类型
    template<typename T>
    using isReference = std::is_reference<T>;

    /// @brief 检查是否为const类型
    template<typename T>
    using isConst = std::is_const<T>;

    /// @brief 检查是否为数组类型
    template<typename T>
    using isArray = std::is_array<T>;

    /// @brief 检查是否为类类型
    template<typename T>
    using isClass = std::is_class<T>;

    /// @brief 检查是否为枚举类型
    template<typename T>
    using isEnum = std::is_enum<T>;

    /// @brief 检查是否为函数类型
    template<typename T>
    using isFunction = std::is_function<typename std::remove_pointer<T>::type>;

    /// @brief 检查是否为成员函数指针类型
    template<typename T>
    using isMemberFunctionPointer = std::is_member_function_pointer<T>;

    /// @brief 检查是否为成员对象指针类型
    template<typename T>
    using isMemberObjectPointer = std::is_member_object_pointer<T>;

    // ========================================================================
    // 类型修改
    // ========================================================================

    /// @brief 移除const
    template<typename T>
    using removeConst = typename std::remove_const<T>::type;

    /// @brief 移除volatile
    template<typename T>
    using removeVolatile = typename std::remove_volatile<T>::type;

    /// @brief 移除const和volatile
    template<typename T>
    using removeCV = typename std::remove_cv<T>::type;

    /// @brief 移除引用
    template<typename T>
    using removeReference = typename std::remove_reference<T>::type;

    /// @brief 移除指针
    template<typename T>
    using removePointer = typename std::remove_pointer<T>::type;

    /// @brief 添加const
    template<typename T>
    using addConst = typename std::add_const<T>::type;

    /// @brief 添加指针
    template<typename T>
    using addPointer = typename std::add_pointer<T>::type;

    /// @brief 添加左值引用
    template<typename T>
    using addLValueReference = typename std::add_lvalue_reference<T>::type;

    /// @brief 添加右值引用
    template<typename T>
    using addRValueReference = typename std::add_rvalue_reference<T>::type;

    // ========================================================================
    // 类型比较
    // ========================================================================

    /// @brief 检查两个类型是否相同
    template<typename T, typename U>
    using isSame = std::is_same<T, U>;

    /// @brief 检查T是否可以转换为U
    template<typename T, typename U>
    using isConvertible = std::is_convertible<T, U>;

    /// @brief 检查T是否是U的基类
    template<typename T, typename U>
    using isBaseOf = std::is_base_of<T, U>;

    // ========================================================================
    // 容器类型检测
    // ========================================================================

    /// @brief 检查是否为vector
    template<typename T>
    struct isVector : std::false_type {};

    template<typename T, typename A>
    struct isVector<std::vector<T, A>> : std::true_type {};

    /// @brief 检查是否为map
    template<typename T>
    struct isMap : std::false_type {};

    template<typename K, typename V, typename C, typename A>
    struct isMap<std::map<K, V, C, A>> : std::true_type {};

    /// @brief 检查是否为unordered_map
    template<typename T>
    struct isUnorderedMap : std::false_type {};

    template<typename K, typename V, typename H, typename E, typename A>
    struct isUnorderedMap<std::unordered_map<K, V, H, E, A>> : std::true_type {};

    /// @brief 检查是否为set
    template<typename T>
    struct isSet : std::false_type {};

    template<typename K, typename C, typename A>
    struct isSet<std::set<K, C, A>> : std::true_type {};

    /// @brief 检查是否为unordered_set
    template<typename T>
    struct isUnorderedSet : std::false_type {};

    template<typename K, typename H, typename E, typename A>
    struct isUnorderedSet<std::unordered_set<K, H, E, A>> : std::true_type {};

    /// @brief 检查是否为pair
    template<typename T>
    struct isPair : std::false_type {};

    template<typename T1, typename T2>
    struct isPair<std::pair<T1, T2>> : std::true_type {};

    /// @brief 检查是否为tuple
    template<typename T>
    struct isTuple : std::false_type {};

    template<typename... Ts>
    struct isTuple<std::tuple<Ts...>> : std::true_type {};

    /// @brief 检查是否为shared_ptr
    template<typename T>
    struct isSharedPtr : std::false_type {};

    template<typename T>
    struct isSharedPtr<std::shared_ptr<T>> : std::true_type {};

    /// @brief 检查是否为unique_ptr
    template<typename T>
    struct isUniquePtr : std::false_type {};

    template<typename T, typename D>
    struct isUniquePtr<std::unique_ptr<T, D>> : std::true_type {};

    /// @brief 检查是否为weak_ptr
    template<typename T>
    struct isWeakPtr : std::false_type {};

    template<typename T>
    struct isWeakPtr<std::weak_ptr<T>> : std::true_type {};

    // ========================================================================
    // 函数特征
    // ========================================================================

    /**
     * @brief 获取函数返回类型
     *
     * @tparam F 函数类型
     *
     * @section example 示例
     * @code
     * using RetType = TypeHelper::functionReturnType<decltype(std::stoi)>;
     * // int
     * @endcode
     */
    template<typename F>
    using functionReturnType = typename std::result_of<F()>::type;

    /**
     * @brief 获取函数参数个数
     *
     * @tparam F 函数类型
     *
     * @section example 示例
     * @code
     * constexpr size_t argCount = TypeHelper::functionArgumentCount<void(int, double)>;
     * // 2
     * @endcode
     */
    template<typename F>
    struct functionArgumentCount;

    template<typename R, typename... Args>
    struct functionArgumentCount<R(Args...)> {
        static constexpr size_t value = sizeof...(Args);
    };

    // ========================================================================
    // 智能指针辅助
    // ========================================================================

    /**
     * @brief 创建shared_ptr
     *
     * @tparam T 类型
     * @tparam Args 参数类型
     * @param args 构造参数
     * @return shared_ptr
     *
     * @section example 示例
     * @code
     * auto ptr = TypeHelper::makeShared<std::vector<int>>(5, 10);
     * // shared_ptr<vector<int>> with 5 elements, all equal to 10
     * @endcode
     */
    template<typename T, typename... Args>
    static std::shared_ptr<T> makeShared(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief 创建unique_ptr
     *
     * @tparam T 类型
     * @tparam Args 参数类型
     * @param args 构造参数
     * @return unique_ptr
     *
     * @section example 示例
     * @code
     * auto ptr = TypeHelper::makeUnique<std::vector<int>>(5, 10);
     * // unique_ptr<vector<int>> with 5 elements, all equal to 10
     * @endcode
     */
    template<typename T, typename... Args>
    static std::unique_ptr<T> makeUnique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // ========================================================================
    // 类型列表（TypeList）
    // ========================================================================

    /**
     * @brief 类型列表
     *
     * 编译期类型列表，用于类型操作
     *
     * @section example 示例
     * @code
     * using MyTypes = TypeList<int, double, std::string>;
     *
     * constexpr size_t count = MyTypes::size;
     * // 3
     *
     * using FirstType = MyTypes::get<0>;
     * // int
     *
     * bool hasString = MyTypes::contains<std::string>;
     * // true
     *
     * using AppendTypes = MyTypes::append<float, char>;
     * // TypeList<int, double, std::string, float, char>
     * @endcode
     */
    template<typename... Ts>
    struct TypeList {
        /// @brief 类型数量
        static constexpr size_t size = sizeof...(Ts);

        /// @brief 获取第N个类型
        template<size_t N>
        using get = typename std::tuple_element<N, std::tuple<Ts...>>::type;

        /// @brief 检查是否包含某个类型
        template<typename T>
        struct contains {
            static constexpr bool value = (std::is_same<T, Ts>::value || ...);
        };

        template<typename T>
        static constexpr bool contains_v = contains<T>::value;

        /// @brief 追加类型
        template<typename... Us>
        using append = TypeList<Ts..., Us...>;

        /// @brief 前置类型
        template<typename... Us>
        using prepend = TypeList<Us..., Ts...>;

        /// @brief 应用到模板
        template<template<typename> class Template>
        using apply = TypeList<Template<Ts>...>;

        /// @brief 转换为tuple
        using asTuple = std::tuple<Ts...>;

        /// @brief 过滤类型
        template<template<typename> class Predicate>
        using filter = typename filter_impl<Predicate, Ts...>::type;
    };

private:
    template<template<typename> class Predicate, typename... Ts>
    struct filter_impl;

    template<template<typename> class Predicate>
    struct filter_impl<Predicate> {
        using type = TypeList<>;
    };

    template<template<typename> class Predicate, typename T, typename... Ts>
    struct filter_impl<Predicate, T, Ts...> {
        using type = typename std::conditional<
            Predicate<T>::value,
            typename TypeList<T>::template append<typename filter_impl<Predicate, Ts...>::type::template get<Ts>...>,
            typename filter_impl<Predicate, Ts...>::type
        >::type;
    };

public:
    // ========================================================================
    // 运行时类型信息（RTTI）辅助
    // ========================================================================

    /**
     * @brief 类型擦除包装器
     *
     * 可以存储任意类型的对象
     *
     * @section example 示例
     * @code
     * TypeHelper::Any any1 = 42;
     * TypeHelper::Any any2 = std::string("hello");
     *
     * int value = any1.cast<int>();
     * std::string str = any2.cast<std::string>();
     * @endcode
     */
    class Any {
    public:
        Any() = default;

        template<typename T>
        Any(T value) : content_(std::make_shared<Model<T>>(std::move(value))) {}

        bool isEmpty() const {
            return content_ == nullptr;
        }

        template<typename T>
        bool isType() const {
            return content_ && content_->type() == typeid(T);
        }

        template<typename T>
        T cast() const {
            if (!content_) {
                throw std::bad_cast();
            }
            return static_cast<Model<T>*>(content_.get())->value_;
        }

    private:
        struct Concept {
            virtual ~Concept() = default;
            virtual const std::type_info& type() const = 0;
        };

        template<typename T>
        struct Model : Concept {
            explicit Model(T value) : value_(std::move(value)) {}

            const std::type_info& type() const override {
                return typeid(T);
            }

            T value_;
        };

        std::shared_ptr<Concept> content_;
    };

    // ========================================================================
    // 常量检查
    // ========================================================================

    /**
     * @brief 编译期常量检查
     *
     * @tparam condition 条件
     * @tparam T 类型
     *
     * @section example 示例
     * @code
     * static_assert(TypeHelper::checkCondition<true, int>::value, "must be true");
     * @endcode
     */
    template<bool condition, typename T = void>
    using enableIf = typename std::enable_if<condition, T>::type;

    /**
     * @brief 条件类型选择
     *
     * @tparam condition 条件
     * @tparam T 如果为true的类型
     * @tparam F 如果为false的类型
     *
     * @section example 示例
     * @code
     * using IntOrFloat = TypeHelper::conditional<true, int, float>;
     * // int
     *
     * using IntOrFloat2 = TypeHelper::conditional<false, int, float>;
     * // float
     * @endcode
     */
    template<bool condition, typename T, typename F>
    using conditional = typename std::conditional<condition, T, F>::type;
};

// ============================================================================
// 便捷别名
// ============================================================================

/**
 * @brief 类型辅助的便捷别名
 */
namespace Type {
    /// @brief 类型名称
    template<typename T>
    inline std::string getName() {
        return TypeHelper::getTypeName<T>();
    }

    template<typename T>
    inline std::string getName(const T& obj) {
        return TypeHelper::getTypeName(obj);
    }

    /// @brief 类型转换
    template<typename T>
    inline std::string toString(const T& value) {
        return TypeHelper::toString(value);
    }

    template<typename T>
    inline T fromString(const std::string& str) {
        return TypeHelper::fromString<T>(str);
    }

    /// @brief 智能指针创建
    template<typename T, typename... Args>
    inline auto makeShared(Args&&... args) {
        return TypeHelper::makeShared<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline auto makeUnique(Args&&... args) {
        return TypeHelper::makeUnique<T>(std::forward<Args>(args)...);
    }

    /// @brief 类型列表
    template<typename... Ts>
    using List = TypeHelper::TypeList<Ts...>;

    /// @brief 类型擦除
    using Any = TypeHelper::Any;
}

} // namespace Core
} // namespace PaperCrawler

// ============================================================================
// 特化：bool类型转换
// ============================================================================

namespace PaperCrawler {
namespace Core {

template<>
inline bool TypeHelper::fromString<bool>(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        return true;
    } else if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        return false;
    }

    std::istringstream iss(str);
    bool value;
    iss >> value;
    return value;
}

template<>
inline std::string TypeHelper::toString<bool>(const bool& value) {
    return value ? "true" : "false";
}

} // namespace Core
} // namespace PaperCrawler
