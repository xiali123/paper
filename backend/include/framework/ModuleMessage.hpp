#pragma once

#include <string>
#include <memory>
#include <map>
#include <functional>
#include <cstdint>
#include <future>
#include <any>

namespace PaperCrawler {

/**
 * @brief 消息类型
 */
enum class MessageType {
    // 数据操作
    DATA_GET = 0x0100,
    DATA_SET = 0x0101,
    DATA_DELETE = 0x0102,
    DATA_UPDATE = 0x0103,
    DATA_QUERY = 0x0104,

    // 系统控制
    SYSTEM_START = 0x0200,
    SYSTEM_STOP = 0x0201,
    SYSTEM_RESTART = 0x0202,
    SYSTEM_STATUS = 0x0203,

    // 模块管理
    MODULE_LOAD = 0x0300,
    MODULE_UNLOAD = 0x0301,
    MODULE_REGISTER = 0x0302,
    MODULE_UNREGISTER = 0x0303,

    // 网络请求
    HTTP_REQUEST = 0x0400,
    HTTP_RESPONSE = 0x0401,

    // 通用消息
    CUSTOM = 0xFFFF
};

/**
 * @brief 模块消息基类
 */
class ModuleMessage {
public:
    ModuleMessage(MessageType type, const std::string& source = "", const std::string& target = "")
        : type_(type), source_(source), target_(target) {}

    virtual ~ModuleMessage() = default;

    MessageType getType() const { return type_; }
    std::string getSource() const { return source_; }
    std::string getTarget() const { return target_; }

    void setType(MessageType type) { type_ = type; }
    void setSource(const std::string& source) { source_ = source; }
    void setTarget(const std::string& target) { target_ = target; }

    // 数据访问
    template<typename T>
    T getData(const std::string& key, const T& defaultValue = T{}) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    template<typename T>
    void setData(const std::string& key, const T& value) {
        data_[key] = value;
    }

private:
    MessageType type_;
    std::string source_;
    std::string target_;
    std::map<std::string, std::any> data_;
};

} // namespace PaperCrawler
