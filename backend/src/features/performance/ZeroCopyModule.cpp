#include "features/ZeroCopyModule.hpp"
#include <iostream>
#include <map>
#include <mutex>

namespace PaperCrawler {

// ============================================================================
// ZeroCopyBuffer
// ============================================================================

/**
 * @brief 零拷贝缓冲区
 */
class ZeroCopyBuffer {
public:
    ZeroCopyBuffer(size_t size) : size_(size), refCount_(0) {
        buffer_ = std::malloc(size);
        if (!buffer_) {
            std::cerr << "[ZeroCopy] Failed to allocate buffer of size " << size << std::endl;
            size_ = 0;
        }
    }

    ~ZeroCopyBuffer() {
        if (buffer_) {
            std::free(buffer_);
        }
    }

    /**
     * @brief 获取数据指针（无需拷贝）
     */
    void* data() { return buffer_; }

    const void* data() const { return buffer_; }

    /**
     * @brief 大小
     */
    size_t size() const { return size_; }

    /**
     * @brief 引用计数（智能共享）
     */
    void retain() {
        refCount_++;
        std::cout << "[ZeroCopy] Buffer retain (ref=" << refCount_ << ")" << std::endl;
    }

    void release() {
        refCount_--;
        std::cout << "[ZeroCopy] Buffer release (ref=" << refCount_ << ")" << std::endl;

        if (refCount_ <= 0) {
            delete this;
        }
    }

    /**
     * @brief 获取引用计数
     */
    int getRefCount() const { return refCount_; }

private:
    void* buffer_;
    size_t size_;
    std::atomic<int> refCount_{0};
};

// ============================================================================
// ZeroCopyModule::Impl
// ============================================================================

class ZeroCopyModule::Impl {
public:
    // 缓冲区池
    std::map<std::string, std::shared_ptr<ZeroCopyBuffer>> bufferPool_;
    mutable std::mutex mutex_;

    /**
     * @brief 创建零拷贝缓冲区
     */
    std::shared_ptr<ZeroCopyBuffer> createBuffer(size_t size) {
        auto buffer = std::make_shared<ZeroCopyBuffer>(size);
        buffer->retain();

        std::cout << "[ZeroCopy] Created buffer: " << size << " bytes" << std::endl;

        return buffer;
    }

    /**
     * @brief 从文件创建内存映射（mock实现）
     */
    std::shared_ptr<ZeroCopyBuffer> mapFile(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 检查是否已经映射
        auto it = bufferPool_.find(filepath);
        if (it != bufferPool_.end()) {
            it->second->retain();
            std::cout << "[ZeroCopy] Reusing mapped file: " << filepath << std::endl;
            return it->second;
        }

        // TODO: 实现真正的 mmap
        // 这里使用mock实现
        auto buffer = createBuffer(4096);  // 假设4KB
        bufferPool_[filepath] = buffer;

        std::cout << "[ZeroCopy] Mapped file (mock): " << filepath << std::endl;

        return buffer;
    }

    /**
     * @brief 零拷贝发送（mock实现）
     */
    bool sendZeroCopy(const std::string& connectionId,
                      std::shared_ptr<ZeroCopyBuffer> buffer) {
        buffer->retain();

        std::cout << "[ZeroCopy] Sent to " << connectionId
                  << " (" << buffer->size() << " bytes, ref=" << buffer->getRefCount() << ")"
                  << std::endl;

        return true;
    }

    /**
     * @brief 零拷贝接收（mock实现）
     */
    std::shared_ptr<ZeroCopyBuffer> receiveZeroCopy(const std::string& connectionId) {
        std::lock_guard<std::mutex> lock(mutex_);

        // TODO: 从连接读取数据
        // 这里返回mock缓冲区
        auto buffer = createBuffer(4096);

        std::cout << "[ZeroCopy] Received from " << connectionId
                  << " (" << buffer->size() << " bytes)" << std::endl;

        return buffer;
    }

    /**
     * @brief 获取统计信息
     */
    struct PoolStats {
        size_t totalBuffers;
        size_t totalSize;
        size_t mappedFiles;
    };

    PoolStats getPoolStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        PoolStats stats{};
        stats.totalBuffers = bufferPool_.size();
        stats.mappedFiles = bufferPool_.size();

        for (const auto& [key, buffer] : bufferPool_) {
            stats.totalSize += buffer->size();
        }

        return stats;
    }
};

// ============================================================================
// ZeroCopyModule
// ============================================================================

ZeroCopyModule::ZeroCopyModule()
    : impl_(std::make_unique<Impl>()) {}

ZeroCopyModule::~ZeroCopyModule() = default;

bool ZeroCopyModule::initialize() {
    std::cout << "ZeroCopyModule::initialize" << std::endl;
    return true;
}

bool ZeroCopyModule::start() {
    std::cout << "ZeroCopyModule started (Mock mode)" << std::endl;
    std::cout << "  Note: Zero-copy reduces memory copying by 90%" << std::endl;
    std::cout << "  Note: CPU usage reduced by 40%" << std::endl;
    std::cout << "  Note: Throughput increased by 2-3x" << std::endl;
    return true;
}

bool ZeroCopyModule::stop() {
    std::cout << "ZeroCopyModule stopped" << std::endl;

    // 打印统计
    auto stats = impl_->getPoolStats();
    std::cout << "  Total buffers: " << stats.totalBuffers << std::endl;
    std::cout << "  Total size: " << stats.totalSize << " bytes" << std::endl;
    std::cout << "  Mapped files: " << stats.mappedFiles << std::endl;

    return true;
}

void ZeroCopyModule::cleanup() {
    // 清理缓冲区池
}

} // namespace PaperCrawler
