#include "security/SessionModule.hpp"
#include <iostream>
#include <random>
#include <algorithm>

namespace PaperCrawler {

class SessionModule::Impl {
public:
    std::map<std::string, Session> sessions_;
    std::map<std::string, std::vector<std::string>> userSessions_;  // userId -> sessionIds
    SessionStats stats_;
    std::chrono::seconds defaultTTL_{3600};
    mutable std::mutex mutex_;

    std::string createSession(const std::string& userId, const SessionOptions& options) {
        std::lock_guard<std::mutex> lock(mutex_);

        Session session;
        session.sessionId = generateSessionId();
        session.userId = userId;
        session.createdAt = std::chrono::system_clock::now();
        session.lastAccessedAt = session.createdAt;
        session.expiresAt = session.createdAt + options.ttl;
        session.active = true;

        sessions_[session.sessionId] = session;
        userSessions_[userId].push_back(session.sessionId);

        stats_.totalSessions++;
        stats_.activeSessions++;
        stats_.sessionsByUser[userId]++;

        std::cout << "[Session] Created: " << session.sessionId
                  << " (user: " << userId << ", TTL: " << options.ttl.count() << "s)" << std::endl;

        return session.sessionId;
    }

    std::optional<Session> getSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return std::nullopt;
        }

        if (it->second.isExpired()) {
            // 清理过期会话
            sessions_.erase(it);
            stats_.activeSessions--;
            stats_.expiredSessions++;
            return std::nullopt;
        }

        // 更新最后访问时间
        it->second.lastAccessedAt = std::chrono::system_clock::now();

        return it->second;
    }

    bool updateSession(const std::string& sessionId, const Session& session) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        it->second = session;
        it->second.lastAccessedAt = std::chrono::system_clock::now();

        return true;
    }

    bool deleteSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        // 从用户会话列表中移除
        auto userId = it->second.userId;
        auto& userSess = userSessions_[userId];
        userSess.erase(std::remove(userSess.begin(), userSess.end(), sessionId), userSess.end());

        sessions_.erase(it);
        stats_.activeSessions--;

        std::cout << "[Session] Deleted: " << sessionId << std::endl;

        return true;
    }

    bool refreshSession(const std::string& sessionId, std::chrono::seconds additionalTTL) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        if (it->second.isExpired()) {
            return false;
        }

        it->second.expiresAt = std::chrono::system_clock::now() +
                               it->second.getTimeRemaining() + additionalTTL;
        it->second.lastAccessedAt = std::chrono::system_clock::now();

        std::cout << "[Session] Refreshed: " << sessionId << std::endl;

        return true;
    }

    bool isValidSession(const std::string& sessionId) {
        auto session = getSession(sessionId);
        return session.has_value() && session->active;
    }

    bool setSessionData(const std::string& sessionId, const std::string& key, const std::any& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        it->second.data[key] = value;
        return true;
    }

    bool removeSessionData(const std::string& sessionId, const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        it->second.data.erase(key);
        return true;
    }

    std::vector<Session> getUserSessions(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<Session> result;
        auto it = userSessions_.find(userId);

        if (it != userSessions_.end()) {
            for (const auto& sessionId : it->second) {
                auto sessIt = sessions_.find(sessionId);
                if (sessIt != sessions_.end() && !sessIt->second.isExpired()) {
                    result.push_back(sessIt->second);
                }
            }
        }

        return result;
    }

    size_t deleteUserSessions(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = userSessions_.find(userId);
        if (it == userSessions_.end()) {
            return 0;
        }

        size_t deleted = 0;
        for (const auto& sessionId : it->second) {
            if (sessions_.erase(sessionId)) {
                deleted++;
                stats_.activeSessions--;
            }
        }

        userSessions_.erase(it);

        std::cout << "[Session] Deleted " << deleted << " sessions for user: " << userId << std::endl;

        return deleted;
    }

    size_t cleanupExpired() {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t cleaned = 0;
        std::vector<std::string> toDelete;

        for (const auto& [sessionId, session] : sessions_) {
            if (session.isExpired()) {
                toDelete.push_back(sessionId);
            }
        }

        for (const auto& sessionId : toDelete) {
            deleteSession(sessionId);
            cleaned++;
        }

        if (cleaned > 0) {
            stats_.lastCleanup = std::chrono::system_clock::now();
            std::cout << "[Session] Cleaned up " << cleaned << " expired sessions" << std::endl;
        }

        return cleaned;
    }

    void clearAll() {
        std::lock_guard<std::mutex> lock(mutex_);

        sessions_.clear();
        userSessions_.clear();
        stats_.activeSessions = 0;

        std::cout << "[Session] All sessions cleared" << std::endl;
    }

    SessionStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

private:
    std::string generateSessionId() {
        static std::atomic<uint64_t> counter{0};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1000, 9999);

        std::ostringstream oss;
        oss << "sess_" << std::time(nullptr) << "_" << counter.fetch_add(1) << "_" << dis(gen);
        return oss.str();
    }
};

SessionModule::SessionModule()
    : impl_(std::make_unique<Impl>()) {}

SessionModule::~SessionModule() = default;

bool SessionModule::initialize() {
    std::cout << "SessionModule::initialize" << std::endl;
    std::cout << "  Storage: " << storageType_ << std::endl;
    std::cout << "  Default TTL: " << defaultTTL_.count() << "s" << std::endl;
    return true;
}

bool SessionModule::start() {
    std::cout << "SessionModule started" << std::endl;
    return true;
}

bool SessionModule::stop() {
    std::cout << "SessionModule stopped" << std::endl;
    return true;
}

void SessionModule::cleanup() {
    impl_->clearAll();
}

std::string SessionModule::createSession(const std::string& userId, const SessionOptions& options) {
    return impl_->createSession(userId, options);
}

std::optional<Session> SessionModule::getSession(const std::string& sessionId) {
    return impl_->getSession(sessionId);
}

bool SessionModule::updateSession(const std::string& sessionId, const Session& session) {
    return impl_->updateSession(sessionId, session);
}

bool SessionModule::deleteSession(const std::string& sessionId) {
    return impl_->deleteSession(sessionId);
}

bool SessionModule::refreshSession(const std::string& sessionId, std::chrono::seconds additionalTTL) {
    return impl_->refreshSession(sessionId, additionalTTL);
}

bool SessionModule::isValidSession(const std::string& sessionId) {
    return impl_->isValidSession(sessionId);
}

bool SessionModule::setSessionData(const std::string& sessionId, const std::string& key, const std::any& value) {
    return impl_->setSessionData(sessionId, key, value);
}

bool SessionModule::removeSessionData(const std::string& sessionId, const std::string& key) {
    return impl_->removeSessionData(sessionId, key);
}

std::vector<Session> SessionModule::getUserSessions(const std::string& userId) {
    return impl_->getUserSessions(userId);
}

size_t SessionModule::deleteUserSessions(const std::string& userId) {
    return impl_->deleteUserSessions(userId);
}

size_t SessionModule::cleanupExpired() {
    return impl_->cleanupExpired();
}

void SessionModule::clearAll() {
    impl_->clearAll();
}

SessionModule::SessionStats SessionModule::getStats() const {
    return impl_->getStats();
}

void SessionModule::setDefaultTTL(std::chrono::seconds ttl) {
    defaultTTL_ = ttl;
}

void SessionModule::setStorageType(const std::string& type) {
    storageType_ = type;
}

std::string SessionModule::generateSessionId() {
    return impl_->generateSessionId();
}

} // namespace PaperCrawler
