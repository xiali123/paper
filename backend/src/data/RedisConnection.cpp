#include "data/RedisConnection.hpp"
#include <stdexcept>
#include <cstring>
#include <cstdarg>

#ifdef USE_REDIS_CACHE

namespace PaperCrawler {

RedisConnection::RedisConnection(const std::string& host, int port,
                               const std::string& password, int database)
    : context_(nullptr, redisFree)
    , host_(host)
    , port_(port)
    , password_(password)
    , database_(database)
    , connected_(false)
    , inPipeline_(false) {
}

RedisConnection::~RedisConnection() {
    disconnect();
}

bool RedisConnection::connect() {
    if (connected_) {
        return true;
    }

    // 连接到Redis服务器
    context_.reset(redisConnect(host_.c_str(), port_));
    if (!context_ || context_->err) {
        if (context_) {
            // 连接失败
            return false;
        }
        return false;
    }

    // 如果设置了密码，进行认证
    if (!password_.empty()) {
        redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                                "AUTH %s",
                                                                password_.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                freeReplyObject(reply);
            }
            context_.reset();
            return false;
        }
        freeReplyObject(reply);
    }

    // 选择数据库
    if (database_ > 0) {
        redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                                "SELECT %d",
                                                                database_));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                freeReplyObject(reply);
            }
            context_.reset();
            return false;
        }
        freeReplyObject(reply);
    }

    connected_ = true;
    return true;
}

void RedisConnection::disconnect() {
    connected_ = false;
    context_.reset();
}

bool RedisConnection::isConnected() const {
    return connected_ && context_ != nullptr;
}

// ========================================================================
// 基本Redis操作
// ========================================================================

bool RedisConnection::set(const std::string& key, const std::string& value, int ttl) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = nullptr;

    if (ttl > 0) {
        // 带TTL的SET
        reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                     "SETEX %s %d %s",
                                                     key.c_str(),
                                                     ttl,
                                                     value.c_str()));
    } else {
        // 普通SET
        reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                     "SET %s %s",
                                                     key.c_str(),
                                                     value.c_str()));
    }

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }

    freeReplyObject(reply);
    return true;
}

std::optional<std::string> RedisConnection::get(const std::string& key) {
    if (!isConnected()) {
        return std::nullopt;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "GET %s",
                                                             key.c_str()));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return std::nullopt;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return std::nullopt;
    }

    if (reply->type == REDIS_REPLY_STRING) {
        std::string value(reply->str, reply->len);
        freeReplyObject(reply);
        return value;
    }

    freeReplyObject(reply);
    return std::nullopt;
}

bool RedisConnection::del(const std::string& key) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "DEL %s",
                                                             key.c_str()));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }

    int result = reply->integer;
    freeReplyObject(reply);
    return result > 0;
}

bool RedisConnection::exists(const std::string& key) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "EXISTS %s",
                                                             key.c_str()));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }

    int result = reply->integer;
    freeReplyObject(reply);
    return result > 0;
}

bool RedisConnection::expire(const std::string& key, int ttl) {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "EXPIRE %s %d",
                                                             key.c_str(),
                                                             ttl));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }

    int result = reply->integer;
    freeReplyObject(reply);
    return result > 0;
}

int RedisConnection::ttl(const std::string& key) {
    if (!isConnected()) {
        return -2;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "TTL %s",
                                                             key.c_str()));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return -2;
    }

    int result = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    return result;
}

// ========================================================================
// 批量操作
// ========================================================================

bool RedisConnection::mset(const std::map<std::string, std::string>& kvs) {
    if (!isConnected() || kvs.empty()) {
        return false;
    }

    // 构建MSET命令
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;

    argv.push_back("MSET");
    argvlen.push_back(4);

    for (const auto& kv : kvs) {
        argv.push_back(kv.first.c_str());
        argvlen.push_back(kv.first.length());
        argv.push_back(kv.second.c_str());
        argvlen.push_back(kv.second.length());
    }

    redisReply* reply = static_cast<redisReply*>(redisCommandArgv(context_.get(),
                                                                  argv.size(),
                                                                  argv.data(),
                                                                  argvlen.data()));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }

    freeReplyObject(reply);
    return true;
}

std::map<std::string, std::string> RedisConnection::mget(const std::vector<std::string>& keys) {
    std::map<std::string, std::string> result;

    if (!isConnected() || keys.empty()) {
        return result;
    }

    // 构建MGET命令
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;

    argv.push_back("MGET");
    argvlen.push_back(4);

    for (const auto& key : keys) {
        argv.push_back(key.c_str());
        argvlen.push_back(key.length());
    }

    redisReply* reply = static_cast<redisReply*>(redisCommandArgv(context_.get(),
                                                                  argv.size(),
                                                                  argv.data(),
                                                                  argvlen.data()));

    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) {
            freeReplyObject(reply);
        }
        return result;
    }

    // 解析结果
    for (size_t i = 0; i < reply->elements && i < keys.size(); ++i) {
        redisReply* element = reply->element[i];
        if (element->type == REDIS_REPLY_STRING) {
            result[keys[i]] = std::string(element->str, element->len);
        }
    }

    freeReplyObject(reply);
    return result;
}

// ========================================================================
// 高级操作
// ========================================================================

int64_t RedisConnection::incr(const std::string& key, int64_t delta) {
    if (!isConnected()) {
        return 0;
    }

    redisReply* reply = nullptr;

    if (delta == 1) {
        reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                     "INCR %s",
                                                     key.c_str()));
    } else {
        reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                     "INCRBY %s %lld",
                                                     key.c_str(),
                                                     static_cast<long long>(delta)));
    }

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return 0;
    }

    int64_t result = static_cast<int64_t>(reply->integer);
    freeReplyObject(reply);
    return result;
}

std::vector<std::string> RedisConnection::keys(const std::string& pattern) {
    std::vector<std::string> result;

    if (!isConnected()) {
        return result;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "KEYS %s",
                                                             pattern.c_str()));

    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        if (reply) {
            freeReplyObject(reply);
        }
        return result;
    }

    for (size_t i = 0; i < reply->elements; ++i) {
        redisReply* element = reply->element[i];
        if (element->type == REDIS_REPLY_STRING) {
            result.emplace_back(element->str, element->len);
        }
    }

    freeReplyObject(reply);
    return result;
}

bool RedisConnection::flushAll() {
    if (!isConnected()) {
        return false;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "FLUSHALL"));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return false;
    }

    freeReplyObject(reply);
    return true;
}

std::string RedisConnection::ping() {
    if (!isConnected()) {
        return "";
    }

    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "PING"));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        return "";
    }

    std::string result;
    if (reply->type == REDIS_REPLY_STATUS) {
        result = std::string(reply->str, reply->len);
    } else if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }

    freeReplyObject(reply);
    return result;
}

// ========================================================================
// Pipeline支持
// ========================================================================

void RedisConnection::pipelineBegin() {
    inPipeline_ = true;
}

bool RedisConnection::pipelineExecute() {
    if (!inPipeline_ || !isConnected()) {
        return false;
    }

    // 退出Pipeline模式
    redisReply* reply = static_cast<redisReply*>(redisCommand(context_.get(),
                                                             "EXEC"));

    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            freeReplyObject(reply);
        }
        inPipeline_ = false;
        return false;
    }

    freeReplyObject(reply);
    inPipeline_ = false;
    return true;
}

} // namespace PaperCrawler

#else // !USE_REDIS_CACHE

// 当没有Redis支持时，提供空实现
namespace PaperCrawler {

RedisConnection::RedisConnection(const std::string&, int,
                               const std::string&, int)
    : host_()
    , port_(0)
    , password_()
    , database_(0)
    , connected_(false)
    , inPipeline_(false) {
}

RedisConnection::~RedisConnection() {
}

bool RedisConnection::connect() { return false; }
void RedisConnection::disconnect() {}
bool RedisConnection::isConnected() const { return false; }
bool RedisConnection::set(const std::string&, const std::string&, int) { return false; }
std::optional<std::string> RedisConnection::get(const std::string&) { return std::nullopt; }
bool RedisConnection::del(const std::string&) { return false; }
bool RedisConnection::exists(const std::string&) { return false; }
bool RedisConnection::expire(const std::string&, int) { return false; }
int RedisConnection::ttl(const std::string&) { return -2; }
bool RedisConnection::mset(const std::map<std::string, std::string>&) { return false; }
std::map<std::string, std::string> RedisConnection::mget(const std::vector<std::string>&) { return {}; }
int64_t RedisConnection::incr(const std::string&, int64_t) { return 0; }
std::vector<std::string> RedisConnection::keys(const std::string&) { return {}; }
bool RedisConnection::flushAll() { return false; }
std::string RedisConnection::ping() { return ""; }
void RedisConnection::pipelineBegin() {}
bool RedisConnection::pipelineExecute() { return false; }

} // namespace PaperCrawler

#endif // USE_REDIS_CACHE
