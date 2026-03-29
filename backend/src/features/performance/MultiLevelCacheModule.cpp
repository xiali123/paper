#include "features/MultiLevelCacheModule.hpp"
#include <iostream>
#include <algorithm>
#include <list>

namespace PaperCrawler {

// ============================================================================
// MultiLevelCacheModule::Impl
// ============================================================================

class MultiLevelCacheModule::Impl {
public:
    // L1/L2 内存缓存
    std::map<std::string, CacheItem> l1Cache_;  // 热数据
    std::map<std::string, CacheItem> l2Cache_;  // 温数据
    size_t l1MaxSize_;
    size_t l2MaxSize_;

    // 统计信息
    uint64_t totalHits_{0};
    uint64_t totalMisses_{0};
    uint64_t totalL1Hits_{0};
    uint64_t totalL2Hits_{0};
    uint64_t totalL3Hits_{0};
    uint64_t totalL4Hits_{0};
    std::chrono::microseconds totalLatency_{0};

    mutable std::mutex mutex_;

    Impl(size_t l1MaxSize, size_t l2MaxSize)
        : l1MaxSize_(l1MaxSize), l2MaxSize_(l2MaxSize) {}

    /**
     * @brief 获取缓存（自动从最佳层级读取）
     */
    std::optional<std::any> get(const std::string& key) {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::lock_guard<std::mutex> lock(mutex_);

        // 1. 首先检查L1缓存（最快）
        auto l1It = l1Cache_.find(key);
        if (l1It != l1Cache_.end()) {
            if (!isExpired(l1It->second)) {
                l1It->second.accessCount++;
                l1It->second.lastAccess = std::chrono::system_clock::now();

                totalHits_++;
                totalL1Hits_++;

                auto endTime = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                totalLatency_ += latency;

                std::cout << "[MultiLevelCache] L1 HIT: " << key
                          << " (latency: " << latency.count() << "μs)" << std::endl;

                return l1It->second.value;
            } else {
                // 过期，删除
                l1Cache_.erase(l1It);
            }
        }

        // 2. 检查L2缓存
        auto l2It = l2Cache_.find(key);
        if (l2It != l2Cache_.end()) {
            if (!isExpired(l2It->second)) {
                l2It->second.accessCount++;
                l2It->second.lastAccess = std::chrono::system_clock::now();

                // 热数据提升到L1
                if (l1Cache_.size() < l1MaxSize_ && l2It->second.accessCount > 10) {
                    promoteToL1(key, l2It->second);
                    l2Cache_.erase(l2It);
                }

                totalHits_++;
                totalL2Hits_++;

                auto endTime = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                totalLatency_ += latency;

                std::cout << "[MultiLevelCache] L2 HIT: " << key
                          << " (latency: " << latency.count() << "μs)" << std::endl;

                return l2It->second.value;
            } else {
                l2Cache_.erase(l2It);
            }
        }

        // 3. 缓存未命中
        totalMisses_++;

        auto endTime = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        totalLatency_ += latency;

        std::cout << "[MultiLevelCache] MISS: " << key
                  << " (latency: " << latency.count() << "μs)" << std::endl;

        return std::nullopt;
    }

    /**
     * @brief 设置缓存（自动分配到最佳层级）
     */
    void set(const std::string& key, const std::any& value, std::chrono::seconds ttl) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 估算大小
        size_t size = sizeof(std::any); // 简化估算

        CacheItem item;
        item.key = key;
        item.value = value;
        item.level = CacheLevel::L2_WARM;  // 默认放到L2
        item.lastAccess = std::chrono::system_clock::now();
        item.accessCount = 1;
        item.ttl = ttl;
        item.size = size;

        // 根据可用空间选择层级
        if (l1Cache_.size() < l1MaxSize_ * 0.8) {
            // L1有空间，直接放L1
            item.level = CacheLevel::L1_HOT;
            l1Cache_[key] = item;
            std::cout << "[MultiLevelCache] SET " << key << " → L1" << std::endl;
        } else if (l2Cache_.size() < l2MaxSize_) {
            // L2有空间
            item.level = CacheLevel::L2_WARM;
            l2Cache_[key] = item;
            std::cout << "[MultiLevelCache] SET " << key << " → L2" << std::endl;
        } else {
            // 都满了，需要淘汰
            evictLRUInL2();
            l2Cache_[key] = item;
            std::cout << "[MultiLevelCache] SET " << key << " → L2 (evicted)" << std::endl;
        }
    }

    /**
     * @brief 批量获取
     */
    std::map<std::string, std::any> getBatch(const std::vector<std::string>& keys) {
        std::map<std::string, std::any> results;

        for (const auto& key : keys) {
            auto value = get(key);
            if (value.has_value()) {
                results[key] = *value;
            }
        }

        return results;
    }

    /**
     * @brief 批量设置
     */
    void setBatch(const std::map<std::string, std::any>& items) {
        for (const auto& [key, value] : items) {
            set(key, value, std::chrono::seconds(3600));
        }
    }

    /**
     * @brief 缓存预热
     */
    void warmUp(const std::vector<std::string>& keys) {
        std::cout << "[MultiLevelCache] Warming up " << keys.size() << " keys..." << std::endl;

        // 将热数据预加载到L1
        for (const auto& key : keys) {
            // TODO: 从数据源加载
            // 这里只是占位符
            std::cout << "  Preloading: " << key << std::endl;
        }

        std::cout << "[MultiLevelCache] Warmup complete" << std::endl;
    }

    /**
     * @brief 获取统计信息
     */
    MultiLevelCacheModule::CacheStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        MultiLevelCacheModule::CacheStats stats;
        stats.totalItems = l1Cache_.size() + l2Cache_.size();
        stats.l1Count = l1Cache_.size();
        stats.l2Count = l2Cache_.size();
        stats.l3Count = 0;  // TODO: 实现L3后添加
        stats.totalHits = totalHits_;
        stats.totalMisses = totalMisses_;
        stats.hitRate = (totalHits_ + totalMisses_ > 0) ?
            static_cast<double>(totalHits_) / (totalHits_ + totalMisses_) : 0.0;

        stats.avgLatencyUs = (totalHits_ > 0) ?
            static_cast<double>(totalLatency_.count()) / totalHits_ : 0.0;

        stats.hitsByLevel[CacheLevel::L1_HOT] = totalL1Hits_;
        stats.hitsByLevel[CacheLevel::L2_WARM] = totalL2Hits_;
        stats.hitsByLevel[CacheLevel::L3_COLD] = totalL3Hits_;
        stats.hitsByLevel[CacheLevel::L4_PERSISTENT] = totalL4Hits_;

        return stats;
    }

    /**
     * @brief 清理过期缓存
     */
    void cleanupExpired() {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t cleanedL1 = 0;
        size_t cleanedL2 = 0;

        auto l1It = l1Cache_.begin();
        while (l1It != l1Cache_.end()) {
            if (isExpired(l1It->second)) {
                l1It = l1Cache_.erase(l1It);
                cleanedL1++;
            } else {
                ++l1It;
            }
        }

        auto l2It = l2Cache_.begin();
        while (l2It != l2Cache_.end()) {
            if (isExpired(l2It->second)) {
                l2It = l2Cache_.erase(l2It);
                cleanedL2++;
            } else {
                ++l2It;
            }
        }

        if (cleanedL1 > 0 || cleanedL2 > 0) {
            std::cout << "[MultiLevelCache] Cleaned up " << cleanedL1
                      << " L1 items, " << cleanedL2 << " L2 items" << std::endl;
        }
    }

    /**
     * @brief 淘汰LRU缓存
     */
    void evictLRU(size_t targetSize) {
        while (l2Cache_.size() > targetSize) {
            evictLRUInL2();
        }
    }

private:
    bool isExpired(const CacheItem& item) const {
        if (item.ttl.count() <= 0) {
            return false;  // 永不过期
        }

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - item.lastAccess);
        return elapsed >= item.ttl;
    }

    void promoteToL1(const std::string& key, const CacheItem& item) {
        // 确保L1有空间
        if (l1Cache_.size() >= l1MaxSize_) {
            // 淘汰最久未使用的L1项
            auto lruIt = std::min_element(l1Cache_.begin(), l1Cache_.end(),
                [](const auto& a, const auto& b) {
                    return a.second.lastAccess < b.second.lastAccess;
                });

            if (lruIt != l1Cache_.end()) {
                std::cout << "[MultiLevelCache] Evicting from L1: " << lruIt->first << std::endl;
                l1Cache_.erase(lruIt);
            }
        }

        // 提升到L1
        CacheItem l1Item = item;
        l1Item.level = CacheLevel::L1_HOT;
        l1Cache_[key] = l1Item;

        std::cout << "[MultiLevelCache] Promoted to L1: " << key << std::endl;
    }

    void evictLRUInL2() {
        if (l2Cache_.empty()) {
            return;
        }

        auto lruIt = std::min_element(l2Cache_.begin(), l2Cache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.lastAccess < b.second.lastAccess;
            });

        if (lruIt != l2Cache_.end()) {
            std::cout << "[MultiLevelCache] Evicting from L2: " << lruIt->first << std::endl;
            l2Cache_.erase(lruIt);
        }
    }
};

// ============================================================================
// MultiLevelCacheModule
// ============================================================================

MultiLevelCacheModule::MultiLevelCacheModule()
    : impl_(std::make_unique<Impl>(1000, 10000)) {}

MultiLevelCacheModule::~MultiLevelCacheModule() = default;

bool MultiLevelCacheModule::initialize() {
    std::cout << "MultiLevelCacheModule::initialize" << std::endl;
    std::cout << "  L1 max size: " << impl_->l1MaxSize_ << std::endl;
    std::cout << "  L2 max size: " << impl_->l2MaxSize_ << std::endl;
    return true;
}

bool MultiLevelCacheModule::start() {
    std::cout << "MultiLevelCacheModule started" << std::endl;
    return true;
}

bool MultiLevelCacheModule::stop() {
    std::cout << "MultiLevelCacheModule stopped" << std::endl;
    return true;
}

void MultiLevelCacheModule::cleanup() {
    impl_->cleanupExpired();
}

std::optional<std::any> MultiLevelCacheModule::get(const std::string& key) {
    return impl_->get(key);
}

void MultiLevelCacheModule::set(const std::string& key, const std::any& value,
                                std::chrono::seconds ttl) {
    impl_->set(key, value, ttl);
}

std::map<std::string, std::any> MultiLevelCacheModule::getBatch(const std::vector<std::string>& keys) {
    return impl_->getBatch(keys);
}

void MultiLevelCacheModule::setBatch(const std::map<std::string, std::any>& items) {
    impl_->setBatch(items);
}

void MultiLevelCacheModule::warmUp(const std::vector<std::string>& keys) {
    impl_->warmUp(keys);
}

MultiLevelCacheModule::CacheStats MultiLevelCacheModule::getStats() const {
    return impl_->getStats();
}

void MultiLevelCacheModule::cleanupExpired() {
    impl_->cleanupExpired();
}

void MultiLevelCacheModule::evictLRU(size_t targetSize) {
    impl_->evictLRU(targetSize);
}

void MultiLevelCacheModule::promoteToL1(const std::string& key) {
    // 由Impl内部处理
}

void MultiLevelCacheModule::demoteToL2(const std::string& key) {
    // 由Impl内部处理
}

CacheLevel MultiLevelCacheModule::selectOptimalLevel(const std::string& key, uint64_t accessCount) {
    // 简单策略：高频访问→L1，低频→L2
    if (accessCount > 10) {
        return CacheLevel::L1_HOT;
    } else {
        return CacheLevel::L2_WARM;
    }
}

} // namespace PaperCrawler
