/**
 * OT Engine (Operational Transformation)
 * 操作转换引擎 - 头文件
 *
 * 文件位置: backend/include/collaboration/OTEngine.hpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

namespace PaperCrawler {
namespace Collaboration {

// ============================================================================
// 操作类型枚举
// ============================================================================

/**
 * @brief 操作类型
 */
enum class OpType {
    Insert,  // 插入操作
    Delete,  // 删除操作
    Retain   // 保留操作（用于占位）
};

// ============================================================================
// 操作（Operation）结构
// ============================================================================

/**
 * @brief 操作结构
 *
 * 表示单个编辑操作（插入、删除、保留）
 */
struct Operation {
    OpType type;              // 操作类型
    int position;             // 操作位置
    std::string content;      // 插入的内容（仅Insert操作）
    int length;               // 删除的长度（仅Delete操作）
    std::string clientId;     // 客户端ID
    uint64_t clientTimestamp; // 客户端时间戳
    uint64_t serverTimestamp; // 服务器时间戳
    int version;              // 文档版本号
    bool transformed;         // 是否已被转换

    // 构造函数
    Operation(OpType type, int position, const std::string& content = "", int length = 0);
    Operation();

    // 工具方法
    std::string toString() const;
    nlohmann::json toJson() const;
    static Operation fromJson(const nlohmann::json& j);
};

// ============================================================================
// 文档状态（DocumentState）类
// ============================================================================

/**
 * @brief 文档状态类
 *
 * 维护文档的当前内容和版本号
 */
class DocumentState {
public:
    DocumentState(int documentId, const std::string& initialContent = "");

    // 获取文档内容
    std::string getContent() const;

    // 获取文档版本
    int getVersion() const;

    // 应用操作
    void applyOperation(const Operation& op);

    // 获取文档长度
    int length() const;

private:
    int documentId;
    std::string content;
    int version;
    mutable std::mutex mutex_;
};

// ============================================================================
// OT引擎（OTEngine）类
// ============================================================================

/**
 * @brief 操作转换引擎
 *
 * 核心功能：
 * 1. 创建和管理文档状态
 * 2. 操作转换（Transform）
 * 3. 应用操作
 * 4. 冲突检测和解决
 * 5. 批量操作处理
 */
class OTEngine {
public:
    OTEngine();
    ~OTEngine();

    /**
     * @brief 创建新文档
     * @param documentId 文档ID
     * @param initialContent 初始内容
     */
    void createDocument(int documentId, const std::string& initialContent = "");

    /**
     * @brief 删除文档
     * @param documentId 文档ID
     */
    void deleteDocument(int documentId);

    /**
     * @brief 获取文档内容
     * @param documentId 文档ID
     * @return 文档内容
     */
    std::string getDocumentContent(int documentId);

    /**
     * @brief 获取文档版本
     * @param documentId 文档ID
     * @return 文档版本号
     */
    int getDocumentVersion(int documentId);

    /**
     * @brief 转换操作（核心OT算法）
     * @param op1 要转换的操作
     * @param op2 参考操作
     * @return 转换后的操作
     */
    Operation transform(Operation op1, Operation op2);

    /**
     * @brief 应用操作到文档
     * @param documentId 文档ID
     * @param operation 要应用的操作（引用，会被修改）
     * @param clientId 客户端ID
     * @return 是否成功
     */
    bool applyOperation(int documentId, Operation& operation, const std::string& clientId);

    /**
     * @brief 获取待处理操作队列
     * @param documentId 文档ID
     * @param clientId 客户端ID
     * @return 待处理操作列表（排除当前客户端的操作）
     */
    std::vector<Operation> getPendingOperations(int documentId, const std::string& clientId);

    /**
     * @brief 清空操作队列
     * @param documentId 文档ID
     */
    void clearOperationQueue(int documentId);

    /**
     * @brief 批量转换操作
     * @param operations 要转换的操作列表
     * @param againstOp 参考操作
     * @return 转换后的操作列表
     */
    std::vector<Operation> transformBatch(
        const std::vector<Operation>& operations,
        const Operation& againstOp);

    /**
     * @brief 批量应用操作
     * @param documentId 文档ID
     * @param operations 要应用的操作列表（引用，会被修改）
     * @param clientId 客户端ID
     * @return 是否全部成功
     */
    bool applyOperationsBatch(
        int documentId,
        std::vector<Operation>& operations,
        const std::string& clientId);

    /**
     * @brief 检测两个操作是否冲突
     * @param op1 操作1
     * @param op2 操作2
     * @return 是否冲突
     */
    bool hasConflict(const Operation& op1, const Operation& op2);

    /**
     * @brief 解决操作冲突
     * @param op1 操作1
     * @param op2 操作2
     * @return 解决后的操作
     */
    Operation resolveConflict(const Operation& op1, const Operation& op2);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Collaboration
} // namespace PaperCrawler
