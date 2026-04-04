#include "core/SharedBroadcastQueue.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace PaperCrawler {

const char* SharedBroadcastQueue::SHARED_MEMORY_NAME = "PaperCrawler_BroadcastQueue_SM";
const char* SharedBroadcastQueue::MUTEX_NAME = "PaperCrawler_BroadcastQueue_Mutex";
const char* SharedBroadcastQueue::SEMAPHORE_NAME = "PaperCrawler_BroadcastQueue_Sem";

std::unique_ptr<SharedBroadcastQueue::SharedMemory> SharedBroadcastQueue::sharedMemory_;
std::mutex SharedBroadcastQueue::mutex_;

// ============================================================================
// SharedMemory implementation
// ============================================================================

SharedBroadcastQueue::SharedMemory::SharedMemory(const char* name, size_t size)
    : size_(size) {
#ifdef _WIN32
    // 创建或打开共享内存
    handle_ = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        size,
        name
    );

    if (!handle_) {
        spdlog::error("[SharedQueue] Failed to create shared memory: {}", name);
        return;
    }

    data_ = MapViewOfFile(
        handle_,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        size
    );

    if (!data_) {
        spdlog::error("[SharedQueue] Failed to map shared memory");
        CloseHandle(handle_);
        handle_ = nullptr;
        return;
    }

    spdlog::info("[SharedQueue] ✅ Created/shared memory: {} ({} bytes)", name, size);
#endif
}

SharedBroadcastQueue::SharedMemory::~SharedMemory() {
#ifdef _WIN32
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (handle_) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
#endif
}

SharedBroadcastQueue::~SharedMemory() {
#ifdef _WIN32
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (handle_) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
#endif
}

// ============================================================================
// SharedBroadcastQueue implementation
// ============================================================================

bool SharedBroadcastQueue::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (sharedMemory_ && sharedMemory_->isValid()) {
        spdlog::info("[SharedQueue] Already initialized");
        return true;
    }

    // 创建共享内存（足够容纳SharedMemoryData）
    sharedMemory_ = std::make_unique<SharedMemory>(
        SHARED_MEMORY_NAME,
        sizeof(SharedMemoryData)
    );

    if (!sharedMemory_->isValid()) {
        spdlog::error("[SharedQueue] Failed to initialize shared memory");
        return false;
    }

    // 初始化共享内存数据
    auto* data = static_cast<SharedMemoryData*>(sharedMemory_->get());

    // 只在首次创建时初始化
    if (!data->initialized.load()) {
        new (data) SharedMemoryData(); // placement new
        data->initialized.store(true);
        spdlog::info("[SharedQueue] ✅ Shared memory initialized");
    }

    return true;
}

void SharedBroadcastQueue::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (sharedMemory_) {
        auto* data = static_cast<SharedMemoryData*>(sharedMemory_->get());
        if (data) {
            data->initialized.store(false);
        }
        sharedMemory_.reset();
    }

#ifdef _WIN32
    // Windows清理：关闭所有句柄
    // 实际清理由析构函数处理
#endif
}

bool SharedBroadcastQueue::broadcast(std::shared_ptr<ModuleMessage> message) {
    if (!sharedMemory_ || !sharedMemory_->isValid()) {
        spdlog::error("[SharedQueue] Not initialized");
        return false;
    }

    auto* data = static_cast<SharedMemoryData*>(sharedMemory_->get());

    // 计算下一个位置
    size_t currentTail = data->tail.load();
    size_t nextTail = (currentTail + 1) % QUEUE_CAPACITY;

    // 检查队列是否已满
    size_t currentHead = data->head.load();
    if (nextTail == currentHead) {
        spdlog::warn("[SharedQueue] ⚠️ Queue is full, message dropped");
        return false;
    }

    // 存储消息（简化版：存储指针）
    // 注意：这在跨DLL情况下不安全，实际需要序列化
    auto& slot = data->slots[currentTail];

    // ⚠️ 临时方案：只在同一进程内有效
    // 如果消息来自主程序，DLL可以直接访问
    slot.messagePtr = message.get();
    slot.occupied.store(true);

    // 更新tail指针
    data->tail.store(nextTail);

    spdlog::info("[SharedQueue] 📤 Broadcast message: type={}, head={}, tail={}",
                 static_cast<int>(message->getType()),
                 currentHead,
                 nextTail);

    return true;
}

std::shared_ptr<ModuleMessage> SharedBroadcastQueue::receive(uint32_t timeoutMs) {
    if (!sharedMemory_ || !sharedMemory_->isValid()) {
        return nullptr;
    }

    auto* data = static_cast<SharedMemoryData*>(sharedMemory_->get());

    // 检查是否有消息
    size_t currentHead = data->head.load();
    size_t currentTail = data->tail.load();

    if (currentHead == currentTail) {
        // 队列为空
        return nullptr;
    }

    // 获取消息
    auto& slot = data->slots[currentHead];

    if (!slot.occupied.load()) {
        return nullptr;
    }

    // ⚠️ 临时方案：恢复指针
    auto* rawPtr = static_cast<ModuleMessage*>(slot.messagePtr);

    // 尝试转换为正确的消息类型
    std::shared_ptr<ModuleMessage> message;

    // 尝试转换为DatabaseConnectionMessage
    if (rawPtr->getType() == MessageType::CUSTOM) {
        // 使用shared_from_this技巧获取shared_ptr
        // 这需要消息对象本身支持shared_from_this
        // 临时方案：不增加引用计数，直接使用原始指针
        message = std::shared_ptr<ModuleMessage>(rawPtr, [](ModuleMessage*) {
            // 不删除，因为所有权在发送者
        });
    }

    // 清空slot
    slot.messagePtr = nullptr;
    slot.occupied.store(false);

    // 更新head指针
    size_t nextHead = (currentHead + 1) % QUEUE_CAPACITY;
    data->head.store(nextHead);

    spdlog::info("[SharedQueue] 📨 Received message: type={}, head={}, tail={}",
                 static_cast<int>(message->getType()),
                 currentHead,
                 nextHead);

    return message;
}

bool SharedBroadcastQueue::hasMessage() {
    if (!sharedMemory_ || !sharedMemory_->isValid()) {
        return false;
    }

    auto* data = static_cast<SharedMemoryData*>(sharedMemory_->get());
    return data->head.load() != data->tail.load();
}

} // namespace PaperCrawler
