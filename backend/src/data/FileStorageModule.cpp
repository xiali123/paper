#include <iostream>
#include "data/FileStorageModule.hpp"
#include "features/operations/ResponseHandlerModule.hpp"
#include <sstream>
#include <filesystem>
#include <random>
#include <iomanip>
#include <sstream>

namespace PaperCrawler {

// ============================================================================
// FileInfo JSON 序列化
// ============================================================================

std::string FileInfo::toJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"filename\": \"" << filename << "\",\n";
    json << "  \"path\": \"" << path << "\",\n";
    json << "  \"url\": \"" << url << "\",\n";
    json << "  \"size\": " << size << ",\n";
    json << "  \"content_type\": \"" << contentType << "\",\n";
    json << "  \"is_directory\": " << (isDirectory ? "true" : "false") << ",\n";
    json << "  \"access_count\": " << accessCount << "\n";
    json << "}";
    return json.str();
}

// ============================================================================
// FileStorageModule 实现
// ============================================================================

class FileStorageModule::Impl {
public:
    FileStorageConfig config_;
    std::map<std::string, FileInfo> fileIndex_;
    mutable std::mutex mutex_;

    /**
     * @brief 初始化文件存储
     */
    bool initialize(const FileStorageConfig& config) {
        config_ = config;

        std::cout << "[FileStorage] Initializing file storage..." << std::endl;
        std::cout << "  Base path: " << config.basePath << std::endl;
        std::cout << "  URL prefix: " << config.urlPrefix << std::endl;
        std::cout << "  Max file size: " << config.maxFileSize << " bytes" << std::endl;

        namespace fs = std::filesystem;

        // 创建基础目录
        try {
            if (!fs::exists(config.basePath)) {
                fs::create_directories(config.basePath);
                std::cout << "[FileStorage] Created base directory: " << config.basePath << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to create base directory: " << e.what() << std::endl;
            return false;
        }

        // 索引现有文件
        indexFiles(config.basePath);

        std::cout << "[FileStorage] Initialized with " << fileIndex_.size() << " files" << std::endl;
        return true;
    }

    /**
     * @brief 保存文件
     */
    UploadResult saveFile(const std::string& filename,
                         const std::vector<uint8_t>& data,
                         const std::string& contentType) {
        UploadResult result;

        // 验证文件扩展名
        if (!isValidExtension(filename)) {
            result.success = false;
            result.message = "Invalid file extension: " + filename;
            return result;
        }

        // 验证文件大小
        if (data.size() > config_.maxFileSize) {
            result.success = false;
            result.message = "File too large: " + std::to_string(data.size()) + " bytes (max: " +
                          std::to_string(config_.maxFileSize) + ")";
            return result;
        }

        // 生成文件路径
        std::string filePath = generateFilePath(filename);

        // 确保目录存在
        std::filesystem::path pathObj(filePath);
        if (pathObj.has_parent_path()) {
            createDirectories(pathObj.parent_path().string());
        }

        // 写入文件
        std::ofstream file(filePath, std::ios::binary);
        if (!file) {
            result.success = false;
            result.message = "Failed to create file: " + filePath;
            return result;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();

        // 计算校验和
        std::string checksum = calculateChecksum(data);

        // 创建文件信息
        FileInfo info;
        info.filename = std::filesystem::path(filePath).filename().string();
        info.path = filePath;
        info.url = getFileUrl(info.filename);
        info.size = data.size();
        info.contentType = contentType.empty() ? getMimeType(filename) : contentType;
        info.createdAt = std::chrono::system_clock::now();
        info.modifiedAt = info.createdAt;
        info.checksum = checksum;
        info.accessCount = 0;
        info.isDirectory = false;

        // 添加到索引
        {
            std::lock_guard<std::mutex> lock(mutex_);
            fileIndex_[filePath] = info;
        }

        result.success = true;
        result.message = "File saved successfully";
        result.filename = info.filename;
        result.path = filePath;
        result.url = info.url;
        result.size = data.size();

        std::cout << "[FileStorage] Saved: " << info.filename << " (" << data.size() << " bytes)" << std::endl;

        return result;
    }

    /**
     * @brief 加载文件
     */
    std::optional<std::vector<uint8_t>> loadFile(const std::string& path) {
        auto resolved = resolvePath(path);
        if (!resolved) return std::nullopt;
        std::string fullPath = *resolved;

        if (!std::filesystem::exists(fullPath)) {
            std::cout << "[FileStorage] File not found: " << fullPath << std::endl;
            return std::nullopt;
        }

        // 读取文件
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "[FileStorage] Failed to open file: " << fullPath << std::endl;
            return std::nullopt;
        }

        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(fileSize);
        file.read(reinterpret_cast<char*>(data.data()), fileSize);
        file.close();

        // 更新访问计数
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = fileIndex_.find(fullPath);
            if (it != fileIndex_.end()) {
                it->second.accessCount++;
            }
        }

        std::cout << "[FileStorage] Loaded: " << fullPath << " (" << data.size() << " bytes)" << std::endl;

        return data;
    }

    /**
     * @brief 删除文件
     */
    bool deleteFile(const std::string& path) {
        auto resolved = resolvePath(path);
        if (!resolved) return false;
        std::string fullPath = *resolved;

        if (!std::filesystem::exists(fullPath)) {
            return false;
        }

        try {
            std::filesystem::remove(fullPath);

            // 从索引中移除
            std::lock_guard<std::mutex> lock(mutex_);
            fileIndex_.erase(fullPath);

            std::cout << "[FileStorage] Deleted: " << fullPath << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to delete file: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 文件是否存在
     */
    bool fileExists(const std::string& path) {
        auto resolved = resolvePath(path);
        if (!resolved) return false;
        std::string fullPath = *resolved;

        return std::filesystem::exists(fullPath);
    }

    /**
     * @brief 获取文件信息
     */
    std::optional<FileInfo> getFileInfo(const std::string& path) {
        auto resolved = resolvePath(path);
        if (!resolved) return std::nullopt;
        std::string fullPath = *resolved;

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = fileIndex_.find(fullPath);
        if (it != fileIndex_.end()) {
            return it->second;
        }

        // 如果不在索引中，尝试从文件系统获取
        if (!std::filesystem::exists(fullPath)) {
            return std::nullopt;
        }

        FileInfo info;
        auto fsPath = std::filesystem::path(fullPath);
        info.filename = fsPath.filename().string();
        info.path = fullPath;
        info.url = getFileUrl(info.filename);
        info.size = std::filesystem::file_size(fullPath);
        info.contentType = getMimeType(info.filename);

        // 转换 file_time_type 到 system_clock::time_point
        auto fileTime = std::filesystem::last_write_time(fullPath);
        auto sysTime = std::chrono::system_clock::now() + (fileTime - std::filesystem::file_time_type::clock::now());
        info.createdAt = sysTime;
        info.modifiedAt = sysTime;
        info.isDirectory = std::filesystem::is_directory(fullPath);
        info.accessCount = 0;

        return info;
    }

    /**
     * @brief 列出目录内容
     */
    std::vector<FileInfo> listDirectory(const std::string& path) {
        auto resolved = resolvePath(path);
        if (!resolved) return {};
        std::string fullPath = *resolved;

        std::vector<FileInfo> files;

        try {
            for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
                FileInfo info;
                info.filename = entry.path().filename().string();
                info.path = entry.path().string();
                info.url = getFileUrl(info.filename);
                info.size = std::filesystem::is_regular_file(entry) ?
                         std::filesystem::file_size(entry) : 0;
                info.contentType = getMimeType(info.filename);
                info.isDirectory = std::filesystem::is_directory(entry);

                auto ftime = std::filesystem::last_write_time(entry);
                auto sysTime = std::chrono::system_clock::now() + (ftime - std::filesystem::file_time_type::clock::now());
                info.modifiedAt = sysTime;
                info.createdAt = sysTime;

                files.push_back(info);
            }
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Error listing directory: " << e.what() << std::endl;
        }

        return files;
    }

    /**
     * @brief 创建目录
     */
    bool createDirectory(const std::string& path) {
        auto resolved = resolvePath(path);
        if (!resolved) return false;
        std::string fullPath = *resolved;

        try {
            std::filesystem::create_directories(fullPath);
            std::cout << "[FileStorage] Created directory: " << fullPath << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to create directory: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 删除目录
     */
    bool deleteDirectory(const std::string& path, bool recursive) {
        auto resolved = resolvePath(path);
        if (!resolved) return false;
        std::string fullPath = *resolved;

        try {
            if (recursive) {
                std::filesystem::remove_all(fullPath);
            } else {
                std::filesystem::remove(fullPath);
            }

            std::cout << "[FileStorage] Deleted directory: " << fullPath << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to delete directory: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 复制文件
     */
    bool copyFile(const std::string& source, const std::string& destination) {
        auto srcResolved = resolvePath(source);
        auto dstResolved = resolvePath(destination);
        if (!srcResolved || !dstResolved) return false;
        try {
            std::filesystem::copy_file(*srcResolved, *dstResolved);
            std::cout << "[FileStorage] Copied: " << *srcResolved << " -> " << *dstResolved << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to copy file: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 移动文件
     */
    bool moveFile(const std::string& source, const std::string& destination) {
        auto srcResolved = resolvePath(source);
        auto dstResolved = resolvePath(destination);
        if (!srcResolved || !dstResolved) return false;
        try {
            std::filesystem::rename(*srcResolved, *dstResolved);
            std::cout << "[FileStorage] Moved: " << *srcResolved << " -> " << *dstResolved << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to move file: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 重命名文件
     */
    bool renameFile(const std::string& oldPath, const std::string& newPath) {
        return moveFile(oldPath, newPath);
    }

    /**
     * @brief 获取文件URL
     */
    std::string getFileUrl(const std::string& filename) {
        return config_.urlPrefix + "/" + filename;
    }

    /**
     * @brief 计算校验和（简单的哈希）
     */
    std::string calculateChecksum(const std::vector<uint8_t>& data) {
        // 简单实现：使用std::hash
        // 生产环境应使用MD5或SHA256
        size_t hash = std::hash<std::string>{}(
            std::string(data.begin(), data.end())
        );

        std::ostringstream oss;
        oss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    }

    /**
     * @brief 验证文件扩展名
     */
    bool isValidExtension(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            return false;
        }

        std::string ext = filename.substr(dotPos);
        for (const auto& allowedExt : config_.allowedExtensions) {
            if (ext == allowedExt) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief 获取MIME类型
     */
    std::string getMimeType(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            return "application/octet-stream";
        }

        std::string ext = filename.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        static const std::map<std::string, std::string> mimeTypes = {
            {".pdf", "application/pdf"},
            {".txt", "text/plain"},
            {".html", "text/html"},
            {".htm", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".json", "application/json"},
            {".xml", "application/xml"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".png", "image/png"},
            {".gif", "image/gif"},
            {".svg", "image/svg+xml"},
            {".ico", "image/x-icon"},
            {".doc", "application/msword"},
            {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {".zip", "application/zip"},
            {".rar", "application/x-rar-compressed"},
            {".tar", "application/x-tar"},
            {".gz", "application/gzip"},
            {".mp3", "audio/mpeg"},
            {".mp4", "video/mp4"},
            {".avi", "video/x-msvideo"}
        };

        auto it = mimeTypes.find(ext);
        if (it != mimeTypes.end()) {
            return it->second;
        }

        return "application/octet-stream";
    }

    /**
     * @brief 搜索文件
     */
    std::vector<FileInfo> searchFiles(const std::string& pattern, const std::string& directory) {
        std::vector<FileInfo> results;

        std::string searchPath = directory.empty() ? config_.basePath : directory;

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();

                    // 简单的模式匹配（支持*通配符）
                    if (patternMatches(filename, pattern)) {
                        FileInfo info;
                        info.filename = filename;
                        info.path = entry.path().string();
                        info.url = getFileUrl(filename);
                        info.size = std::filesystem::file_size(entry);
                        info.contentType = getMimeType(filename);
                        info.isDirectory = false;

                        results.push_back(info);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Error searching files: " << e.what() << std::endl;
        }

        return results;
    }

    /**
     * @brief 获取存储统计
     */
    FileStorageModule::StorageStats getStorageStats(const std::string& rootPath) {
        FileStorageModule::StorageStats stats;
        std::string searchPath = rootPath.empty() ? config_.basePath : rootPath;

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
                if (entry.is_regular_file()) {
                    stats.totalFiles++;
                    stats.totalSizeBytes += std::filesystem::file_size(entry);

                    // 按扩展名统计
                    std::string filename = entry.path().filename().string();
                    size_t dotPos = filename.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        std::string ext = filename.substr(dotPos);
                        stats.filesByExtension[ext]++;
                        stats.sizeByExtension[ext] += std::filesystem::file_size(entry);
                    }
                } else if (entry.is_directory()) {
                    stats.totalDirectories++;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Error getting stats: " << e.what() << std::endl;
        }

        return stats;
    }

private:
    /**
     * @brief Sanitize and validate path to prevent path traversal
     * Resolves relative paths against basePath and ensures result stays within basePath.
     * Returns empty optional if path escapes base directory.
     */
    std::optional<std::string> resolvePath(const std::string& userPath) {
        namespace fs = std::filesystem;

        std::string fullPath;
        if (userPath.empty() || userPath == ".") {
            fullPath = config_.basePath;
        } else if (fs::path(userPath).is_absolute()) {
            // Reject absolute paths — they must be relative to basePath
            spdlog::warn("[FileStorage] Rejected absolute path: {}", userPath);
            return std::nullopt;
        } else {
            fullPath = config_.basePath + "/" + userPath;
        }

        // Canonicalize (resolve .., symlinks, etc.)
        std::error_code ec;
        fs::path canonicalBase = fs::canonical(config_.basePath, ec);
        if (ec) canonicalBase = fs::path(config_.basePath).lexically_normal();

        fs::path canonicalFull = fs::canonical(fullPath, ec);
        if (ec) {
            // File/dir may not exist yet — use lexically_normal
            canonicalFull = fs::path(fullPath).lexically_normal();
        }

        // Ensure canonicalFull starts with canonicalBase
        std::string baseStr = canonicalBase.string();
        std::string fullStr = canonicalFull.string();

        if (fullStr.size() < baseStr.size() ||
            fullStr.substr(0, baseStr.size()) != baseStr) {
            spdlog::warn("[FileStorage] Path traversal blocked: {} resolves to {}", userPath, fullStr);
            return std::nullopt;
        }

        return fullStr;
    }
    /**
     * @brief 索引现有文件
     */
    void indexFiles(const std::string& rootPath) {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath)) {
                if (entry.is_regular_file()) {
                    FileInfo info;
                    info.filename = entry.path().filename().string();
                    info.path = entry.path().string();
                    info.url = getFileUrl(info.filename);
                    info.size = std::filesystem::file_size(entry);
                    info.contentType = getMimeType(info.filename);
                    info.isDirectory = false;
                    auto ftime = std::filesystem::last_write_time(entry);
                    auto sysTime = std::chrono::system_clock::now() + (ftime - std::filesystem::file_time_type::clock::now());
                    info.createdAt = sysTime;
                    info.modifiedAt = sysTime;
                    info.accessCount = 0;

                    fileIndex_[entry.path().string()] = info;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Error indexing files: " << e.what() << std::endl;
        }
    }

    /**
     * @brief 生成文件路径
     */
    std::string generateFilePath(const std::string& filename) {
        if (config_.organizeByDate) {
            // 按日期组织：YYYY/MM/DD/
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time);
#else
            localtime_r(&time, &tm);
#endif

            char datePath[32];
            std::strftime(datePath, sizeof(datePath), "%Y/%m/%d", &tm);

            std::string fullPath = config_.basePath + "/" + datePath + "/" + filename;

            // 确保目录存在
            createDirectories(config_.basePath + "/" + datePath);

            return fullPath;
        } else {
            return config_.basePath + "/" + filename;
        }
    }

    /**
     * @brief 创建目录（递归）
     */
    bool createDirectories(const std::string& path) {
        try {
            std::filesystem::create_directories(path);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[FileStorage] Failed to create directories: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 模式匹配
     */
    bool patternMatches(const std::string& filename, const std::string& pattern) {
        if (pattern == "*" || pattern == "*.*") {
            return true;
        }

        if (pattern.find('*') == std::string::npos) {
            return filename == pattern;
        }

        // 简单的通配符匹配
        size_t wildcardPos = pattern.find('*');
        if (wildcardPos != std::string::npos) {
            std::string prefix = pattern.substr(0, wildcardPos);
            std::string suffix = pattern.substr(wildcardPos + 1);

            return filename.size() >= prefix.size() + suffix.size() &&
                   filename.substr(0, prefix.size()) == prefix &&
                   filename.substr(filename.size() - suffix.size()) == suffix;
        }

        return false;
    }
};

// ============================================================================

FileStorageModule::FileStorageModule()
    : impl_(std::make_unique<Impl>()) {
}

FileStorageModule::~FileStorageModule() = default;

bool FileStorageModule::initialize() {
    std::cout << "FileStorageModule::initialize" << std::endl;

    FileStorageConfig defaultConfig;
    return impl_->initialize(defaultConfig);
}

bool FileStorageModule::start() {
    std::cout << "FileStorageModule started" << std::endl;
    return true;
}

bool FileStorageModule::stop() {
    std::cout << "FileStorageModule stopped" << std::endl;
    return true;
}

void FileStorageModule::cleanup() {
    // 清理资源
}

void FileStorageModule::setConfig(const FileStorageConfig& config) {
    impl_->config_ = config;
}

FileStorageConfig FileStorageModule::getConfig() const {
    return impl_->config_;
}

UploadResult FileStorageModule::saveFile(const std::string& filename,
                                        const std::vector<uint8_t>& data,
                                        const std::string& contentType) {
    return impl_->saveFile(filename, data, contentType);
}

std::optional<std::vector<uint8_t>> FileStorageModule::loadFile(const std::string& path) {
    return impl_->loadFile(path);
}

bool FileStorageModule::deleteFile(const std::string& path) {
    return impl_->deleteFile(path);
}

bool FileStorageModule::fileExists(const std::string& path) {
    return impl_->fileExists(path);
}

std::optional<FileInfo> FileStorageModule::getFileInfo(const std::string& path) {
    return impl_->getFileInfo(path);
}

std::vector<FileInfo> FileStorageModule::listDirectory(const std::string& path) {
    return impl_->listDirectory(path);
}

bool FileStorageModule::createDirectory(const std::string& path) {
    return impl_->createDirectory(path);
}

bool FileStorageModule::deleteDirectory(const std::string& path, bool recursive) {
    return impl_->deleteDirectory(path, recursive);
}

bool FileStorageModule::copyFile(const std::string& source, const std::string& destination) {
    return impl_->copyFile(source, destination);
}

bool FileStorageModule::moveFile(const std::string& source, const std::string& destination) {
    return impl_->moveFile(source, destination);
}

bool FileStorageModule::renameFile(const std::string& oldPath, const std::string& newPath) {
    return impl_->renameFile(oldPath, newPath);
}

std::string FileStorageModule::getFileUrl(const std::string& path) {
    return impl_->getFileUrl(path);
}

std::string FileStorageModule::calculateChecksum(const std::vector<uint8_t>& data) {
    return impl_->calculateChecksum(data);
}

bool FileStorageModule::isValidExtension(const std::string& filename) {
    return impl_->isValidExtension(filename);
}

std::string FileStorageModule::getMimeType(const std::string& filename) {
    return impl_->getMimeType(filename);
}

std::vector<FileInfo> FileStorageModule::searchFiles(const std::string& pattern,
                                                     const std::string& directory) {
    return impl_->searchFiles(pattern, directory);
}

FileStorageModule::StorageStats FileStorageModule::getStorageStats(const std::string& rootPath) {
    return impl_->getStorageStats(rootPath);
}

// ============================================================================
// 路由处理
// ============================================================================

void FileStorageModule::registerRoutes() {
    // TODO: 注册路由到Router
}

std::string FileStorageModule::handleUpload(const std::string& body) {
    // TODO: 解析multipart/form-data
    std::string filename = "test.pdf";
    std::vector<uint8_t> data = {0x25, 0x50, 0x44, 0x46}; // Mock PDF数据

    auto result = saveFile(filename, data, "application/pdf");

    if (result.success) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", result.message},
            {"filename", result.filename},
            {"url", result.url},
            {"size", std::to_string(result.size)}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", result.message}
        }, 400);
    }
}

std::string FileStorageModule::handleDownload(const std::map<std::string, std::string>& params) {
    auto pathIt = params.find("path");
    if (pathIt == params.end()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Missing 'path' parameter"}
        }, 400);
    }

    auto data = loadFile(pathIt->second);
    if (data.has_value()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"size", std::to_string(data->size())}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "File not found"}
        }, 404);
    }
}

std::string FileStorageModule::handleDelete(const std::string& body) {
    // TODO: 解析JSON body
    std::string path = "test.pdf";

    if (deleteFile(path)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "File deleted successfully"}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "File not found"}
        }, 404);
    }
}

std::string FileStorageModule::handleList(const std::map<std::string, std::string>& params) {
    std::string path = params.count("path") ? params.at("path") : "";

    auto files = listDirectory(path);

    std::ostringstream json;
    json << "[\n";
    bool first = true;
    for (const auto& file : files) {
        if (!first) json << ",\n";
        first = false;
        json << file.toJSON();
    }
    json << "\n]";

    return ResponseHandlerModule::buildJsonResponse({
        {"files", json.str()},
        {"count", std::to_string(files.size())}
    });
}

std::string FileStorageModule::handleInfo(const std::map<std::string, std::string>& params) {
    auto pathIt = params.find("path");
    if (pathIt == params.end()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Missing 'path' parameter"}
        }, 400);
    }

    auto info = getFileInfo(pathIt->second);
    if (info.has_value()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"info", info->toJSON()}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "File not found"}
        }, 404);
    }
}

std::string FileStorageModule::handleStats() {
    auto stats = getStorageStats();

    std::map<std::string, std::string> statsMap;
    statsMap["total_files"] = std::to_string(stats.totalFiles);
    statsMap["total_directories"] = std::to_string(stats.totalDirectories);
    statsMap["total_size_bytes"] = std::to_string(stats.totalSizeBytes);

    return ResponseHandlerModule::buildJsonResponse(statsMap);
}

} // namespace PaperCrawler
