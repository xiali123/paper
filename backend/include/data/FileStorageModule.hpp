#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include "communication/UnifiedMessage.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace PaperCrawler {

/**
 * @brief 文件存储模块 - 文件存储抽象层
 *
 * 功能：
 * 1. 文件上传/下载
 * 2. 文件列表管理
 * 3. 文件类型验证
 * 4. URL生成
 * 5. 统一消息协议接口
 */
class FileStorageModule : public IModule {
public:
    FileStorageModule();
    ~FileStorageModule() override;

    // IModule接口实现
    std::string getName() const override { return "FileStorage"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "File storage abstraction layer";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 文件操作（使用统一消息协议）
     */
    MessageResponse save(const UnifiedMessage& message);      // 保存文件
    MessageResponse load(const UnifiedMessage& message);      // 加载文件
    MessageResponse deleteFile(const UnifiedMessage& message); // 删除文件
    MessageResponse exists(const UnifiedMessage& message);    // 检查存在

    /**
     * @brief 批量操作
     */
    std::vector<std::string> listFiles(const std::string& directory);
    bool deleteDirectory(const std::string& directory);

    /**
     * @brief 获取文件URL
     */
    std::string getFileUrl(const std::string& filePath);

    /**
     * @brief 验证文件类型
     */
    bool isValidFileType(const std::string& filename);

private:
    struct StorageConfig {
        std::string basePath{"./storage"};
        std::string urlPrefix{"http://localhost:8080/files"};
        size_t maxFileSize{100 * 1024 * 1024};  // 100MB
        std::vector<std::string> allowedExtensions{
            ".pdf", ".txt", ".doc", ".docx", ".png", ".jpg", ".jpeg"
        };
    } config_;

    std::string generateUniqueFilename(const std::string& originalFilename);
};

} // namespace PaperCrawler
