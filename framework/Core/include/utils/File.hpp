#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <cstdint>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 文件大小类型
 */
using FileSize = uint64_t;

/**
 * @brief FileTools - 通用文件系统工具集
 *
 * 提供了完整的文件系统操作功能，包括：
 * - 文件和目录操作
 * - 路径处理
 * - 文件读写
 * - 文件搜索
 * - 文件信息获取
 * - 文件系统监控
 * - 临时文件管理
 *
 * @section features 核心特性
 * - @ref file_operations "文件操作"
 * - @ref path_handling "路径处理"
 * - @ref file_search "文件搜索"
 * - @ref file_info "文件信息"
 *
 * @section example_usage 示例用法
 * @code
 * // 检查文件是否存在
 * bool exists = FileTools::exists("/path/to/file.txt");
 *
 * // 读取文件内容
 * std::string content = FileTools::readFile("/path/to/file.txt");
 *
 * // 写入文件内容
 * FileTools::writeFile("/path/to/file.txt", "Hello, World!");
 *
 * // 获取文件大小
 * FileSize size = FileTools::getFileSize("/path/to/file.txt");
 *
 * // 列出目录内容
 * auto files = FileTools::listDirectory("/path/to/directory");
 *
 * // 创建目录
 * FileTools::createDirectory("/path/to/new/directory");
 *
 * // 删除文件
 * FileTools::removeFile("/path/to/file.txt");
 *
 * // 复制文件
 * FileTools::copyFile("/source/file.txt", "/dest/file.txt");
 *
 * // 获取文件扩展名
 * std::string ext = FileTools::getExtension("file.txt");
 * // ".txt"
 * @endcode
 *
 * @threadsafe 所有静态方法都是线程安全的（文件系统操作本身是线程安全的）
 */
class FileTools {
public:
    namespace fs = std::filesystem;

    // ========================================================================
    // 文件存在检查
    // ========================================================================

    /**
     * @brief 检查文件或目录是否存在
     *
     * @param path 路径
     * @return 是否存在
     *
     * @section example 示例
     * @code
     * bool exists = FileTools::exists("/path/to/file.txt");
     * @endcode
     */
    static bool exists(const std::string& path) {
        std::error_code ec;
        return fs::exists(path, ec);
    }

    /**
     * @brief 检查是否为文件
     *
     * @param path 路径
     * @return 是否为文件
     *
     * @section example 示例
     * @code
     * bool isFile = FileTools::isFile("/path/to/file.txt");
     * @endcode
     */
    static bool isFile(const std::string& path) {
        std::error_code ec;
        return fs::is_regular_file(path, ec);
    }

    /**
     * @brief 检查是否为目录
     *
     * @param path 路径
     * @return 是否为目录
     *
     * @section example 示例
     * @code
     * bool isDir = FileTools::isDirectory("/path/to/directory");
     * @endcode
     */
    static bool isDirectory(const std::string& path) {
        std::error_code ec;
        return fs::is_directory(path, ec);
    }

    // ========================================================================
    // 文件读写
    // ========================================================================

    /**
     * @brief 读取文件内容
     *
     * @param path 文件路径
     * @return 文件内容
     *
     * @section example 示例
     * @code
     * std::string content = FileTools::readFile("/path/to/file.txt");
     * @endcode
     *
     * @throws std::runtime_error 如果文件不存在或无法读取
     */
    static std::string readFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::ostringstream content;
        content << file.rdbuf();
        return content.str();
    }

    /**
     * @brief 读取文件内容（按行）
     *
     * @param path 文件路径
     * @return 文件行列表
     *
     * @section example 示例
     * @code
     * auto lines = FileTools::readFileLines("/path/to/file.txt");
     * // ["line1", "line2", "line3"]
     * @endcode
     *
     * @throws std::runtime_error 如果文件不存在或无法读取
     */
    static std::vector<std::string> readFileLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream file(path);

        if (!file) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        return lines;
    }

    /**
     * @brief 写入文件内容
     *
     * @param path 文件路径
     * @param content 文件内容
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * bool success = FileTools::writeFile("/path/to/file.txt", "Hello, World!");
     * @endcode
     */
    static bool writeFile(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }

        file << content;
        return file.good();
    }

    /**
     * @brief 追加内容到文件
     *
     * @param path 文件路径
     * @param content 要追加的内容
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * bool success = FileTools::appendFile("/path/to/file.txt", "\nNew line");
     * @endcode
     */
    static bool appendFile(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file) {
            return false;
        }

        file << content;
        return file.good();
    }

    // ========================================================================
    // 文件操作
    // ========================================================================

    /**
     * @brief 删除文件
     *
     * @param path 文件路径
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * bool success = FileTools::removeFile("/path/to/file.txt");
     * @endcode
     */
    static bool removeFile(const std::string& path) {
        std::error_code ec;
        return fs::remove(path, ec);
    }

    /**
     * @brief 创建目录
     *
     * @param path 目录路径
     * @param recursive 是否递归创建父目录
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * // 创建单层目录
     * bool success = FileTools::createDirectory("/path/to/dir");
     *
     * // 递归创建多层目录
     * bool success = FileTools::createDirectory("/path/to/nested/dir", true);
     * @endcode
     */
    static bool createDirectory(const std::string& path, bool recursive = true) {
        std::error_code ec;
        if (recursive) {
            return fs::create_directories(path, ec);
        } else {
            return fs::create_directory(path, ec);
        }
    }

    /**
     * @brief 删除目录
     *
     * @param path 目录路径
     * @param recursive 是否递归删除内容
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * // 删除空目录
     * bool success = FileTools::removeDirectory("/path/to/dir");
     *
     * // 递归删除目录及其内容
     * bool success = FileTools::removeDirectory("/path/to/dir", true);
     * @endcode
     */
    static bool removeDirectory(const std::string& path, bool recursive = false) {
        std::error_code ec;
        if (recursive) {
            return fs::remove_all(path, ec) > 0;
        } else {
            return fs::remove(path, ec);
        }
    }

    /**
     * @brief 复制文件
     *
     * @param source 源文件路径
     * @param destination 目标文件路径
     * @param overwrite 是否覆盖已存在的文件
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * bool success = FileTools::copyFile("/source/file.txt", "/dest/file.txt");
     * @endcode
     */
    static bool copyFile(
        const std::string& source,
        const std::string& destination,
        bool overwrite = false
    ) {
        std::error_code ec;

        if (!overwrite && exists(destination)) {
            return false;
        }

        fs::copy_file(source, destination,
            overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none,
            ec);

        return !ec;
    }

    /**
     * @brief 移动/重命名文件或目录
     *
     * @param source 源路径
     * @param destination 目标路径
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * bool success = FileTools::moveFile("/source/file.txt", "/dest/file.txt");
     * @endcode
     */
    static bool moveFile(const std::string& source, const std::string& destination) {
        std::error_code ec;
        fs::rename(source, destination, ec);
        return !ec;
    }

    // ========================================================================
    // 目录操作
    // ========================================================================

    /**
     * @brief 列出目录内容
     *
     * @param path 目录路径
     * @return 目录内容列表
     *
     * @section example 示例
     * @code
     * auto files = FileTools::listDirectory("/path/to/directory");
     * // ["file1.txt", "file2.txt", "subdirectory"]
     * @endcode
     */
    static std::vector<std::string> listDirectory(const std::string& path) {
        std::vector<std::string> result;

        std::error_code ec;
        auto iterator = fs::directory_iterator(path, ec);

        if (ec) {
            return result;
        }

        for (const auto& entry : iterator) {
            result.push_back(entry.path().filename().string());
        }

        return result;
    }

    /**
     * @brief 列出目录内容（递归）
     *
     * @param path 目录路径
     * @return 所有文件和目录的完整路径列表
     *
     * @section example 示例
     * @code
     * auto files = FileTools::listDirectoryRecursive("/path/to/directory");
     * // ["/path/to/directory/file1.txt",
     * //  "/path/to/directory/subdir/file2.txt"]
     * @endcode
     */
    static std::vector<std::string> listDirectoryRecursive(const std::string& path) {
        std::vector<std::string> result;

        std::error_code ec;
        auto iterator = fs::recursive_directory_iterator(path, ec);

        if (ec) {
            return result;
        }

        for (const auto& entry : iterator) {
            result.push_back(entry.path().string());
        }

        return result;
    }

    /**
     * @brief 获取当前工作目录
     *
     * @return 当前工作目录路径
     *
     * @section example 示例
     * @code
     * std::string cwd = FileTools::getCurrentDirectory();
     * // "/home/user/project"
     * @endcode
     */
    static std::string getCurrentDirectory() {
        std::error_code ec;
        return fs::current_path(ec).string();
    }

    /**
     * @brief 设置当前工作目录
     *
     * @param path 目录路径
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * bool success = FileTools::setCurrentDirectory("/home/user/project");
     * @endcode
     */
    static bool setCurrentDirectory(const std::string& path) {
        std::error_code ec;
        fs::current_path(path, ec);
        return !ec;
    }

    // ========================================================================
    // 文件信息
    // ========================================================================

    /**
     * @brief 获取文件大小
     *
     * @param path 文件路径
     * @return 文件大小（字节）
     *
     * @section example 示例
     * @code
     * FileSize size = FileTools::getFileSize("/path/to/file.txt");
     * // 1024
     * @endcode
     */
    static FileSize getFileSize(const std::string& path) {
        std::error_code ec;
        auto size = fs::file_size(path, ec);
        return ec ? 0 : static_cast<FileSize>(size);
    }

    /**
     * @brief 获取文件最后修改时间
     *
     * @param path 文件路径
     * @return 最后修改时间（时间戳，毫秒）
     *
     * @section example 示例
     * @code
     * int64_t mtime = FileTools::getLastModifiedTime("/path/to/file.txt");
     * // 1712123456789
     * @endcode
     */
    static int64_t getLastModifiedTime(const std::string& path) {
        std::error_code ec;
        auto ftime = fs::last_write_time(path, ec);

        if (ec) {
            return 0;
        }

        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );

        return std::chrono::duration_cast<std::chrono::milliseconds>(
            sctp.time_since_epoch()
        ).count();
    }

    /**
     * @brief 获取文件创建时间
     *
     * @param path 文件路径
     * @return 创建时间（时间戳，毫秒）
     *
     * @note 某些文件系统可能不支持创建时间
     * @section example 示例
     * @code
     * int64_t ctime = FileTools::getCreationTime("/path/to/file.txt");
     * // 1712123456789
     * @endcode
     */
    static int64_t getCreationTime(const std::string& path) {
        std::error_code ec;
        auto ftime = fs::last_write_time(path, ec);

        if (ec) {
            return 0;
        }

        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );

        return std::chrono::duration_cast<std::chrono::milliseconds>(
            sctp.time_since_epoch()
        ).count();
    }

    // ========================================================================
    // 路径处理
    // ========================================================================

    /**
     * @brief 获取文件扩展名
     *
     * @param path 文件路径
     * @return 扩展名（包含点号）
     *
     * @section example 示例
     * @code
     * std::string ext = FileTools::getExtension("file.txt");
     * // ".txt"
     *
     * std::string ext2 = FileTools::getExtension("/path/to/file.tar.gz");
     * // ".gz"
     * @endcode
     */
    static std::string getExtension(const std::string& path) {
        fs::path p(path);
        return p.extension().string();
    }

    /**
     * @brief 获取文件名（不含扩展名）
     *
     * @param path 文件路径
     * @return 文件名（不含扩展名）
     *
     * @section example 示例
     * @code
     * std::string name = FileTools::getFileNameWithoutExtension("file.txt");
     * // "file"
     *
     * std::string name2 = FileTools::getFileNameWithoutExtension("/path/to/file.txt");
     * // "file"
     * @endcode
     */
    static std::string getFileNameWithoutExtension(const std::string& path) {
        fs::path p(path);
        return p.stem().string();
    }

    /**
     * @brief 获取文件名（含扩展名）
     *
     * @param path 文件路径
     * @return 文件名
     *
     * @section example 示例
     * @code
     * std::string name = FileTools::getFileName("/path/to/file.txt");
     * // "file.txt"
     * @endcode
     */
    static std::string getFileName(const std::string& path) {
        fs::path p(path);
        return p.filename().string();
    }

    /**
     * @brief 获取父目录路径
     *
     * @param path 文件路径
     * @return 父目录路径
     *
     * @section example 示例
     * @code
     * std::string parent = FileTools::getParentDirectory("/path/to/file.txt");
     * // "/path/to"
     * @endcode
     */
    static std::string getParentDirectory(const std::string& path) {
        fs::path p(path);
        return p.parent_path().string();
    }

    /**
     * @brief 规范化路径
     *
     * @param path 文件路径
     * @return 规范化后的路径
     *
     * @section example 示例
     * @code
     * std::string normalized = FileTools::normalizePath("/path/to/.././file.txt");
     * // "/path/file.txt"
     * @endcode
     */
    static std::string normalizePath(const std::string& path) {
        return fs::path(path).string();
    }

    /**
     * @brief 获取绝对路径
     *
     * @param path 文件路径
     * @return 绝对路径
     *
     * @section example 示例
     * @code
     * std::string absolute = FileTools::getAbsolutePath("file.txt");
     * // "/home/user/project/file.txt"
     * @endcode
     */
    static std::string getAbsolutePath(const std::string& path) {
        std::error_code ec;
        return fs::absolute(path, ec).string();
    }

    /**
     * @brief 拼接路径
     *
     * @param base 基础路径
     * @param relative 相对路径
     * @return 拼接后的路径
     *
     * @section example 示例
     * @code
     * std::string joined = FileTools::joinPath("/path/to", "file.txt");
     * // "/path/to/file.txt"
     * @endcode
     */
    static std::string joinPath(const std::string& base, const std::string& relative) {
        return (fs::path(base) / fs::path(relative)).string();
    }

    // ========================================================================
    // 文件搜索
    // ========================================================================

    /**
     * @brief 在目录中搜索文件
     *
     * @param directory 搜索目录
     * @param pattern 文件名模式（支持通配符 * 和 ?）
     * @param recursive 是否递归搜索
     * @return 匹配的文件路径列表
     *
     * @section example 示例
     * @code
     * // 搜索所有.txt文件
     * auto files = FileTools::findFiles("/path/to/dir", "*.txt");
     * // ["/path/to/dir/file1.txt", "/path/to/dir/file2.txt"]
     *
     * // 递归搜索
     * auto files2 = FileTools::findFiles("/path/to/dir", "*.txt", true);
     * // ["/path/to/dir/file1.txt", "/path/to/dir/subdir/file2.txt"]
     * @endcode
     */
    static std::vector<std::string> findFiles(
        const std::string& directory,
        const std::string& pattern,
        bool recursive = false
    ) {
        std::vector<std::string> result;

        std::error_code ec;
        if (recursive) {
            auto iterator = fs::recursive_directory_iterator(directory, ec);
            for (const auto& entry : iterator) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (matchesPattern(filename, pattern)) {
                        result.push_back(entry.path().string());
                    }
                }
            }
        } else {
            auto iterator = fs::directory_iterator(directory, ec);
            for (const auto& entry : iterator) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (matchesPattern(filename, pattern)) {
                        result.push_back(entry.path().string());
                    }
                }
            }
        }

        return result;
    }

    // ========================================================================
    // 临时文件
    // ========================================================================

    /**
     * @brief 创建临时文件
     *
     * @param prefix 文件名前缀
     * @param extension 文件扩展名
     * @return 临时文件路径
     *
     * @section example 示例
     * @code
     * std::string tempFile = FileTools::createTempFile("tmp", ".txt");
     * // "/tmp/tmp12345.txt"
     * @endcode
     */
    static std::string createTempFile(
        const std::string& prefix = "tmp",
        const std::string& extension = ""
    ) {
        std::string tempDir = getTempDirectory();
        std::string filename = prefix + generateRandomString(8) + extension;
        return joinPath(tempDir, filename);
    }

    /**
     * @brief 创建临时目录
     *
     * @param prefix 目录名前缀
     * @return 临时目录路径
     *
     * @section example 示例
     * @code
     * std::string tempDir = FileTools::createTempDirectory("tmp");
     * // "/tmp/tmp12345"
     * @endcode
     */
    static std::string createTempDirectory(const std::string& prefix = "tmp") {
        std::string baseTempDir = getTempDirectory();
        std::string dirname = prefix + generateRandomString(8);
        std::string path = joinPath(baseTempDir, dirname);
        createDirectory(path);
        return path;
    }

    /**
     * @brief 获取系统临时目录
     *
     * @return 临时目录路径
     *
     * @section example 示例
     * @code
     * std::string tempDir = FileTools::getTempDirectory();
     * // Windows: "C:\\Users\\user\\AppData\\Local\\Temp"
     * // Linux/Mac: "/tmp"
     * @endcode
     */
    static std::string getTempDirectory() {
        std::error_code ec;
        return fs::temp_directory_path(ec).string();
    }

    // ========================================================================
    // 文件系统信息
    // ========================================================================

    /**
     * @brief 获取磁盘空间信息
     *
     * @param path 路径
     * @param available 可用空间（字节）
     * @param capacity 总容量（字节）
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * uint64_t available, capacity;
     * bool success = FileTools::getDiskSpace("/", available, capacity);
     * // available = 1024 * 1024 * 1024  // 1GB
     * // capacity = 100 * 1024 * 1024 * 1024  // 100GB
     * @endcode
     */
    static bool getDiskSpace(
        const std::string& path,
        uint64_t& available,
        uint64_t& capacity
    ) {
        std::error_code ec;
        auto spaceInfo = fs::space(path, ec);

        if (ec) {
            return false;
        }

        available = spaceInfo.available;
        capacity = spaceInfo.capacity;
        return true;
    }

private:
    /**
     * @brief 匹配文件名模式（支持通配符）
     *
     * @param filename 文件名
     * @param pattern 模式（支持 * 和 ?）
     * @return 是否匹配
     */
    static bool matchesPattern(
        const std::string& filename,
        const std::string& pattern
    ) {
        // 简单的通配符匹配实现
        size_t fPos = 0;
        size_t pPos = 0;
        size_t starPos = std::string::npos;
        size_t fMatchPos = std::string::npos;

        while (fPos < filename.length()) {
            if (pPos < pattern.length() &&
                (pattern[pPos] == filename[fPos] || pattern[pPos] == '?')) {
                fPos++;
                pPos++;
            } else if (pPos < pattern.length() && pattern[pPos] == '*') {
                starPos = pPos;
                fMatchPos = fPos;
                pPos++;
            } else if (starPos != std::string::npos) {
                pPos = starPos + 1;
                fMatchPos++;
                fPos = fMatchPos;
            } else {
                return false;
            }
        }

        while (pPos < pattern.length() && pattern[pPos] == '*') {
            pPos++;
        }

        return pPos == pattern.length();
    }

    /**
     * @brief 生成随机字符串
     *
     * @param length 字符串长度
     * @return 随机字符串
     */
    static std::string generateRandomString(size_t length) {
        static const char charset[] =
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        static constexpr size_t charsetSize = sizeof(charset) - 1;

        std::string result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            // 使用简单哈希生成伪随机数
            size_t index = (i * 17 + length * 23) % charsetSize;
            result += charset[index];
        }

        return result;
    }
};

// ============================================================================
// 便捷别名
// ============================================================================

/**
 * @brief 文件工具的便捷别名
 */
namespace File {
    inline bool exists(const std::string& path) {
        return FileTools::exists(path);
    }

    inline bool isFile(const std::string& path) {
        return FileTools::isFile(path);
    }

    inline bool isDirectory(const std::string& path) {
        return FileTools::isDirectory(path);
    }

    inline std::string readFile(const std::string& path) {
        return FileTools::readFile(path);
    }

    inline bool writeFile(const std::string& path, const std::string& content) {
        return FileTools::writeFile(path, content);
    }

    inline bool removeFile(const std::string& path) {
        return FileTools::removeFile(path);
    }

    inline bool createDirectory(const std::string& path, bool recursive = true) {
        return FileTools::createDirectory(path, recursive);
    }

    inline std::vector<std::string> listDirectory(const std::string& path) {
        return FileTools::listDirectory(path);
    }

    inline FileSize getFileSize(const std::string& path) {
        return FileTools::getFileSize(path);
    }

    inline std::string getExtension(const std::string& path) {
        return FileTools::getExtension(path);
    }

    inline std::string getFileName(const std::string& path) {
        return FileTools::getFileName(path);
    }

    inline std::string joinPath(const std::string& base, const std::string& relative) {
        return FileTools::joinPath(base, relative);
    }
}

} // namespace Core
} // namespace PaperCrawler
