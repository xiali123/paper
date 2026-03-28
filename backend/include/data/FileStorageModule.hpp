#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <fstream>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 文件存储配置
 */
struct FileStorageConfig {
    std::string basePath{"./storage"};      // 基础路径
    std::string urlPrefix{"/files"};        // URL前缀
    size_t maxFileSize{100 * 1024 * 1024}; // 最大文件大小（100MB）
    std::vector<std::string> allowedExtensions{".pdf", ".txt", ".doc", ".docx", ".jpg", ".png"};
    bool enableCompression{false};         // 启用压缩
    bool organizeByDate{true};            // 按日期组织文件
    bool overwriteExisting{false};        // 覆盖已存在文件
};

/**
 * @brief 文件信息
 */
struct FileInfo {
    std::string filename;
    std::string path;              // 完整路径
    std::string url;               // 访问URL
    size_t size{0};                // 文件大小（字节）
    std::string contentType;        // MIME类型
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
    std::string checksum;          // MD5/SHA256校验和
    uint64_t accessCount{0};       // 访问次数
    bool isDirectory{false};

    std::string toJSON() const;
};

/**
 * @brief 上传结果
 */
struct UploadResult {
    bool success{false};
    std::string message;
    std::string filename;
    std::string path;
    std::string url;
    size_t size{0};
};

/**
 * @brief 文件存储模块
 *
 * 功能：
 * 1. 文件上传和下载
 * 2. 文件列表和搜索
 * 3. 目录管理
 * 4. 文件元数据管理
 * 5. 访问控制
 * 6. 文件校验
 */
class FileStorageModule : public IModule {
public:
    FileStorageModule();
    ~FileStorageModule() override;

    std::string getName() const override { return "FileStorage"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "File storage abstraction layer";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return "/api/files"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 设置配置
     */
    void setConfig(const FileStorageConfig& config);

    /**
     * @brief 获取配置
     */
    FileStorageConfig getConfig() const;

    /**
     * @brief 保存文件
     */
    UploadResult saveFile(const std::string& filename,
                         const std::vector<uint8_t>& data,
                         const std::string& contentType = "");

    /**
     * @brief 加载文件
     */
    std::optional<std::vector<uint8_t>> loadFile(const std::string& path);

    /**
     * @brief 删除文件
     */
    bool deleteFile(const std::string& path);

    /**
     * @brief 文件是否存在
     */
    bool fileExists(const std::string& path);

    /**
     * @brief 获取文件信息
     */
    std::optional<FileInfo> getFileInfo(const std::string& path);

    /**
     * @brief 列出目录内容
     */
    std::vector<FileInfo> listDirectory(const std::string& path);

    /**
     * @brief 创建目录
     */
    bool createDirectory(const std::string& path);

    /**
     * @brief 删除目录
     */
    bool deleteDirectory(const std::string& path, bool recursive = false);

    /**
     * @brief 复制文件
     */
    bool copyFile(const std::string& source, const std::string& destination);

    /**
     * @brief 移动文件
     */
    bool moveFile(const std::string& source, const std::string& destination);

    /**
     * @brief 重命名文件
     */
    bool renameFile(const std::string& oldPath, const std::string& newPath);

    /**
     * @brief 获取文件URL
     */
    std::string getFileUrl(const std::string& path);

    /**
     * @brief 计算校验和
     */
    std::string calculateChecksum(const std::vector<uint8_t>& data);

    /**
     * @brief 验证文件扩展名
     */
    bool isValidExtension(const std::string& filename);

    /**
     * @brief 获取MIME类型
     */
    std::string getMimeType(const std::string& filename);

    /**
     * @brief 搜索文件
     */
    std::vector<FileInfo> searchFiles(const std::string& pattern,
                                      const std::string& directory = "");

    /**
     * @brief 获取存储统计
     */
    struct StorageStats {
        uint64_t totalFiles{0};
        uint64_t totalDirectories{0};
        uint64_t totalSize{0};
        uint64_t totalSizeBytes{0};
        std::map<std::string, uint64_t> filesByExtension;
        std::map<std::string, uint64_t> sizeByExtension;
    };
    StorageStats getStorageStats(const std::string& rootPath = "");

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleUpload(const std::string& body);
    std::string handleDownload(const std::map<std::string, std::string>& params);
    std::string handleDelete(const std::string& body);
    std::string handleList(const std::map<std::string, std::string>& params);
    std::string handleInfo(const std::map<std::string, std::string>& params);
    std::string handleStats();

    /**
     * @brief 生成文件路径
     */
    std::string generateFilePath(const std::string& filename);

    /**
     * @brief 创建目录（递归）
     */
    bool createDirectories(const std::string& path);
};

} // namespace PaperCrawler
