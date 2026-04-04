#pragma once

#include "ModuleMessage.hpp"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace PaperCrawler {

/**
 * @brief 跨DLL共享内存广播队列
 *
 * 解决Windows DLL边界问题：
 * - 主程序发送消息到共享队列
 * - DLL模块通过轮询从队列获取消息
 * - 使用共享内存实现跨进程/DLL通信
 */
class SharedBroadcastQueue {
public:
    // 共享内存名称
    static const char* SHARED_MEMORY_NAME;
    static const char* MUTEX_NAME;
    static const char* SEMAPHORE_NAME;

    // 队列容量
    static const size_t QUEUE_CAPACITY = 256;

    /**
     * @brief 初始化共享广播队列（主程序调用）
     */
    static bool initialize();

    /**
     * @brief 清理共享广播队列
     */
    static void cleanup();

    /**
     * @brief 发送消息到队列（主程序调用）
     */
    static bool broadcast(std::shared_ptr<ModuleMessage> message);

    /**
     * @brief 接收消息（DLL模块调用）
     * @param timeoutMs 超时时间（毫秒），0表示非阻塞
     * @return 消息或nullptr
     */
    static std::shared_ptr<ModuleMessage> receive(uint32_t timeoutMs = 0);

    /**
     * @brief 检查是否有消息（非阻塞）
     */
    static bool hasMessage();

private:
    // 共享内存数据结构
    struct SharedMemoryData {
        std::atomic<size_t> head{0};      // 队列头索引
        std::atomic<size_t> tail{0};      // 队列尾索引
        std::atomic<bool> initialized{false}; // 是否已初始化

        // 消息存储（简化版：只存储指针，实际需要序列化）
        struct MessageSlot {
            void* messagePtr{nullptr};  // 消息指针（在共享地址空间中有效）
            std::atomic<bool> occupied{false}; // 是否被占用
        };

        MessageSlot slots[QUEUE_CAPACITY];
    };

    // Windows共享内存封装
    class SharedMemory {
    public:
        SharedMemory(const char* name, size_t size);
        ~SharedMemory();  // 添加析构函数声明

        void* get() { return data_; }
        bool isValid() { return data_ != nullptr; }

        // 禁止拷贝和移动
        SharedMemory(const SharedMemory&) = delete;
        SharedMemory& operator=(const SharedMemory&) = delete;

    private:
        void* data_{nullptr};
        size_t size_{0};
#ifdef _WIN32
        HANDLE handle_{nullptr};
#endif
    };

    static std::unique_ptr<SharedMemory> sharedMemory_;
    static std::mutex mutex_;
};

} // namespace PaperCrawler
