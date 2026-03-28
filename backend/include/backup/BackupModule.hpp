#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 备份类型
 */
enum class BackupType {
    DATABASE,    // 数据库备份
    FILES,       // 文件备份
    CONFIG,      // 配置备份
    LOGS,        // 日志备份
    FULL,        // 完整备份
    INCREMENTAL, // 增量备份
    DIFFERENTIAL // 差异备份
};

/**
 * @brief 备份状态
 */
enum class BackupStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED,
    EXPIRED
};

/**
 * @brief 备份任务
 */
struct BackupTask {
    std::string taskId;
    std::string name;
    std::string description;
    BackupType type;
    BackupStatus status{BackupStatus::PENDING};

    // 备份目标
    std::string database;        // 数据库名称
    std::string directory;       // 目录路径
    std::vector<std::string> files;  // 文件列表

    // 备份配置
    std::string backupPath;      // 备份文件路径
    std::string compressionType;  // 压缩类型（gzip, zip, none）
    bool enableEncryption{false};  // 启用加密
    std::string encryptionKey;     // 加密密钥

    // 调度
    std::chrono::system_clock::time_point scheduledAt;
    std::chrono::seconds interval{0};  // 重复间隔（0表示不重复）
    bool recurring{false};

    // 执行信息
    std::chrono::system_clock::time_point startedAt;
    std::chrono::system_clock::time_point completedAt;
    std::chrono::seconds duration{0};

    // 结果
    size_t size{0};              // 备份大小（字节）
    size_t originalSize{0};      // 原始大小
    double compressionRatio{0.0}; // 压缩率

    // 统计
    uint64_t filesProcessed{0};
    uint64_t totalFiles{0};
    uint64_t bytesProcessed{0};
    uint64_t totalBytes{0};

    // 错误信息
    std::string errorMessage;
    int retryCount{0};

    // 保留策略
    std::chrono::seconds retentionPeriod{0};  // 保留期限
    bool deleteAfterRestore{false};          // 恢复后删除

    /**
     * @brief 获取进度百分比
     */
    double getProgress() const {
        if (totalBytes == 0) return 0.0;
        return (static_cast<double>(bytesProcessed) / totalBytes) * 100.0;
    }
};

/**
 * @brief 恢复任务
 */
struct RestoreTask {
    std::string taskId;
    std::string backupId;        // 备份ID
    std::string backupPath;      // 备份文件路径
    std::string targetDatabase;  // 目标数据库
    std::string targetDirectory; // 目标目录

    // 恢复选项
    bool overwrite{false};       // 覆盖已存在的文件
    bool stopOnError{true};     // 遇到错误时停止
    bool verifyChecksum{true};  // 验证校验和

    // 执行信息
    std::chrono::system_clock::time_point startedAt;
    std::chrono::system_clock::time_point completedAt;

    // 结果
    bool success{false};
    std::string errorMessage;
    uint64_t filesRestored{0};
    uint64_t bytesRestored{0};
};

/**
 * @brief 备份统计
 */
struct BackupStats {
    uint64_t totalBackups{0};
    uint64_t successfulBackups{0};
    uint64_t failedBackups{0};
    uint64_t totalBackupSize{0};
    uint64_t totalOriginalSize{0};
    double averageCompressionRatio{0.0};
    std::map<BackupType, uint64_t> backupsByType;
    std::chrono::system_clock::time_point lastBackupTime;
    std::chrono::system_clock::time_point lastRestoreTime;
};

/**
 * @brief 备份模块配置
 */
struct BackupConfig {
    std::string backupDir{"./backups"};           // 备份目录
    std::chrono::hours backupInterval{24};        // 备份间隔
    std::chrono::seconds retentionDays{30};       // 保留天数
    bool enableCompression{true};                 // 启用压缩
    std::string compressionType{"gzip"};          // 压缩类型
    bool enableEncryption{false};                 // 启用加密
    int maxConcurrentBackups{3};                  // 最大并发备份数
    bool enableScheduling{true};                  // 启用调度
    bool enableCleanup{true};                     // 启用自动清理
    double minFreeSpaceGB{10.0};                  // 最小剩余空间（GB）
};

/**
 * @brief 备份模块
 *
 * 功能：
 * 1. 数据库备份
 * 2. 文件备份
 * 3. 自动备份调度
 * 4. 备份压缩
 * 5. 备份恢复
 * 6. 备份加密
 * 7. 增量备份
 * 8. 备份清理
 *
 * 特性：
 * - 可靠：校验和验证
 * - 灵活：支持多种备份类型
 * - 自动：定时备份自动执行
 * - 安全：支持加密
 * - 高效：压缩和增量备份
 */
class BackupModule : public IModule {
public:
    BackupModule();
    ~BackupModule() override;

    std::string getName() const override { return "Backup"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Automated backup and restore for databases and files";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 创建数据库备份
     * @param database 数据库名称
     * @return 备份任务ID
     */
    std::string backupDatabase(const std::string& database);

    /**
     * @brief 创建文件备份
     * @param directory 目录路径
     * @return 备份任务ID
     */
    std::string backupFiles(const std::string& directory);

    /**
     * @brief 创建配置备份
     */
    std::string backupConfig();

    /**
     * @brief 创建完整备份
     */
    std::string backupFull();

    /**
     * @brief 创建增量备份
     */
    std::string backupIncremental(const std::string& baseBackupId);

    /**
     * @brief 恢复备份
     * @param backupPath 备份文件路径
     * @param options 恢复选项
     * @return 恢复任务ID
     */
    std::string restoreBackup(const std::string& backupPath, const RestoreTask& options = RestoreTask());

    /**
     * @brief 调度定期备份
     * @param type 备份类型
     * @param interval 间隔时间
     * @param target 备份目标
     * @return 任务ID
     */
    std::string scheduleBackup(BackupType type, std::chrono::seconds interval, const std::string& target);

    /**
     * @brief 取消备份任务
     */
    bool cancelBackup(const std::string& taskId);

    /**
     * @brief 获取备份任务
     */
    std::optional<BackupTask> getBackupTask(const std::string& taskId) const;

    /**
     * @brief 获取所有备份
     */
    std::vector<BackupTask> listBackups() const;

    /**
     * @brief 列出指定类型的备份
     */
    std::vector<BackupTask> listBackupsByType(BackupType type) const;

    /**
     * @brief 删除备份
     */
    bool deleteBackup(const std::string& backupId);

    /**
     * @brief 验证备份完整性
     */
    bool verifyBackup(const std::string& backupPath);

    /**
     * @brief 清理过期备份
     */
    size_t cleanupExpired();

    /**
     * @brief 获取备份统计
     */
    BackupStats getStats() const;

    /**
     * @brief 设置配置
     */
    void setConfig(const BackupConfig& config);

    /**
     * @brief 导出备份
     */
    bool exportBackup(const std::string& backupId, const std::string& destination);

    /**
     * @brief 导入备份
     */
    bool importBackup(const std::string& source);

    /**
     * @brief 获取备份大小
     */
    size_t getBackupSize(const std::string& backupPath) const;

    /**
     * @brief 压缩备份
     */
    bool compressBackup(const std::string& backupPath, const std::string& compressionType = "gzip");

    /**
     * @brief 解压备份
     */
    bool decompressBackup(const std::string& backupPath);

    /**
     * @brief 加密备份
     */
    bool encryptBackup(const std::string& backupPath, const std::string& encryptionKey);

    /**
     * @brief 解密备份
     */
    bool decryptBackup(const std::string& backupPath, const std::string& encryptionKey);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::string generateTaskId();
    std::string generateBackupPath(BackupType type, const std::string& name);
    void cleanupLoop();

    BackupConfig config_;
    std::map<std::string, BackupTask> backupTasks_;
    std::vector<std::string> scheduledBackups_;
    BackupStats stats_;
};

} // namespace PaperCrawler
