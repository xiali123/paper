/**
 * @file test_file_tools.cpp
 * @brief FileTools 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <fstream>
#include <cstdio>

using namespace PaperCrawler::Core;

/**
 * @brief 测试用临时文件管理器
 */
class TempFile {
public:
    TempFile(const std::string& path) : path_(path) {}

    ~TempFile() {
        // 删除文件
        std::remove(path_.c_str());
    }

    std::string getPath() const { return path_; }

private:
    std::string path_;
};

/**
 * @test 文件存在检查测试
 */
TEST(FileToolsTest, FileExists) {
    // 创建临时文件
    std::string tempPath = "/tmp/test_file_exists.txt";
    {
        std::ofstream file(tempPath);
        file << "test content";
    }

    EXPECT_TRUE(FileTools::exists(tempPath));
    EXPECT_TRUE(FileTools::isFile(tempPath));
    EXPECT_FALSE(FileTools::isDirectory(tempPath));

    // 清理
    std::remove(tempPath.c_str());
}

/**
 * @test 目录存在检查测试
 */
TEST(FileToolsTest, DirectoryExists) {
    // 创建临时目录
    std::string tempDir = "/tmp/test_dir_exists";
    FileTools::createDirectory(tempDir);

    EXPECT_TRUE(FileTools::exists(tempDir));
    EXPECT_TRUE(FileTools::isDirectory(tempDir));
    EXPECT_FALSE(FileTools::isFile(tempDir));

    // 清理
    FileTools::removeDirectory(tempDir, true);
}

/**
 * @test 文件读写测试
 */
TEST(FileToolsTest, FileReadAndWrite) {
    std::string tempPath = "/tmp/test_read_write.txt";

    // 写入文件
    std::string content = "Hello, World!";
    ASSERT_TRUE(FileTools::writeFile(tempPath, content));

    // 读取文件
    std::string readContent = FileTools::readFile(tempPath);
    EXPECT_EQ(readContent, content);

    // 清理
    std::remove(tempPath.c_str());
}

/**
 * @test 文件追加测试
 */
TEST(FileToolsTest, FileAppend) {
    std::string tempPath = "/tmp/test_append.txt";

    FileTools::writeFile(tempPath, "Line 1\n");
    FileTools::appendFile(tempPath, "Line 2\n");

    std::string content = FileTools::readFile(tempPath);
    EXPECT_EQ(content, "Line 1\nLine 2\n");

    // 清理
    std::remove(tempPath.c_str());
}

/**
 * @test 按行读取测试
 */
TEST(FileToolsTest, ReadFileLines) {
    std::string tempPath = "/tmp/test_read_lines.txt";

    // 创建多行文件
    {
        std::ofstream file(tempPath);
        file << "Line 1\nLine 2\nLine 3\n";
    }

    auto lines = FileTools::readFileLines(tempPath);

    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "Line 1");
    EXPECT_EQ(lines[1], "Line 2");
    EXPECT_EQ(lines[2], "Line 3");

    // 清理
    std::remove(tempPath.c_str());
}

/**
 * @test 文件删除测试
 */
TEST(FileToolsTest, RemoveFile) {
    std::string tempPath = "/tmp/test_remove.txt";

    // 创建文件
    {
        std::ofstream file(tempPath);
    }

    EXPECT_TRUE(FileTools::exists(tempPath));

    // 删除文件
    ASSERT_TRUE(FileTools::removeFile(tempPath));
    EXPECT_FALSE(FileTools::exists(tempPath));
}

/**
 * @test 目录创建测试
 */
TEST(FileToolsTest, CreateDirectory) {
    std::string tempDir = "/tmp/test_create_dir";

    ASSERT_FALSE(FileTools::exists(tempDir));

    // 创建单层目录
    EXPECT_TRUE(FileTools::createDirectory(tempDir, false));
    EXPECT_TRUE(FileTools::exists(tempDir));

    // 清理
    FileTools::removeDirectory(tempDir, true);
}

/**
 * @test 递归创建目录测试
 */
TEST(FileToolsTest, CreateNestedDirectory) {
    std::string tempDir = "/tmp/test/nested/dir";

    ASSERT_FALSE(FileTools::exists(tempDir));

    // 递归创建
    EXPECT_TRUE(FileTools::createDirectory(tempDir, true));
    EXPECT_TRUE(FileTools::exists(tempDir));

    // 清理
    FileTools::removeDirectory("/tmp/test", true);
}

/**
 * @test 目录删除测试
 */
TEST(FileToolsTest, RemoveDirectory) {
    std::string tempDir = "/tmp/test_remove_dir";

    FileTools::createDirectory(tempDir);

    EXPECT_TRUE(FileTools::exists(tempDir));

    // 删除空目录
    EXPECT_TRUE(FileTools::removeDirectory(tempDir, false));
    EXPECT_FALSE(FileTools::exists(tempDir));
}

/**
 * @test 递归删除目录测试
 */
TEST(FileToolsTest, RemoveDirectoryRecursive) {
    std::string tempDir = "/tmp/test_remove_nested";
    std::string subFile = tempDir + "/subdir/file.txt";

    FileTools::createDirectory(tempDir);
    FileTools::createDirectory(tempDir + "/subdir");
    FileTools::writeFile(subFile, "content");

    // 递归删除
    EXPECT_TRUE(FileTools::removeDirectory(tempDir, true));
    EXPECT_FALSE(FileTools::exists(tempDir));
}

/**
 * @test 文件复制测试
 */
TEST(FileToolsTest, CopyFile) {
    std::string srcPath = "/tmp/test_copy_src.txt";
    std::string dstPath = "/tmp/test_copy_dst.txt";

    // 创建源文件
    FileTools::writeFile(srcPath, "test content");

    // 复制文件
    EXPECT_TRUE(FileTools::copyFile(srcPath, dstPath));
    EXPECT_TRUE(FileTools::exists(dstPath));

    // 验证内容
    std::string content = FileTools::readFile(dstPath);
    EXPECT_EQ(content, "test content");

    // 清理
    std::remove(srcPath.c_str());
    std::remove(dstPath.c_str());
}

/**
 * @test 文件移动测试
 */
TEST(FileToolsTest, MoveFile) {
    std::string srcPath = "/tmp/test_move_src.txt";
    std::string dstPath = "/tmp/test_move_dst.txt";

    FileTools::writeFile(srcPath, "test content");

    EXPECT_TRUE(FileTools::exists(srcPath));
    EXPECT_FALSE(FileTools::exists(dstPath));

    // 移动文件
    ASSERT_TRUE(FileTools::moveFile(srcPath, dstPath));

    EXPECT_FALSE(FileTools::exists(srcPath));
    EXPECT_TRUE(FileTools::exists(dstPath));

    // 清理
    std::remove(dstPath.c_str());
}

/**
 * @test 列出目录内容测试
 */
TEST(FileToolsTest, ListDirectory) {
    std::string tempDir = "/tmp/test_list_dir";

    FileTools::createDirectory(tempDir);
    FileTools::writeFile(tempDir + "/file1.txt", "content1");
    FileTools::writeFile(tempDir + "/file2.txt", "content2");

    auto files = FileTools::listDirectory(tempDir);

    EXPECT_EQ(files.size(), 2);

    // 清理
    FileTools::removeDirectory(tempDir, true);
}

/**
 * @test 路径处理测试
 */
TEST(FileToolsTest, PathOperations) {
    // 扩展名
    EXPECT_EQ(FileTools::getExtension("file.txt"), ".txt");
    EXPECT_EQ(FileTools::getExtension("file.tar.gz"), ".gz");
    EXPECT_EQ(FileTools::getExtension("file"), "");

    // 文件名
    EXPECT_EQ(FileTools::getFileName("/path/to/file.txt"), "file.txt");
    EXPECT_EQ(FileTools::getFileName("file.txt"), "file.txt");

    // 文件名（无扩展名）
    EXPECT_EQ(FileTools::getFileNameWithoutExtension("file.txt"), "file");
    EXPECT_EQ(FileTools::getFileNameWithoutExtension("file.tar.gz"), "file.tar");

    // 父目录
    EXPECT_EQ(FileTools::getParentDirectory("/path/to/file.txt"), "/path/to");
    EXPECT_EQ(FileTools::getParentDirectory("/path/to/"), "/path");
}

/**
 * @test 路径拼接测试
 */
TEST(FileToolsTest, JoinPath) {
    EXPECT_EQ(FileTools::joinPath("/path", "file.txt"), "/path/file.txt");
    EXPECT_EQ(FileTools::joinPath("/path/", "file.txt"), "/path/file.txt");
    EXPECT_EQ(FileTools::joinPath("/path", "to", "file.txt"), "/path/to/file.txt");
}

/**
 * @test 规范化路径测试
 */
TEST(FileToolsTest, NormalizePath) {
    std::string normalized = FileTools::normalizePath("/path/to/.././file.txt");
    EXPECT_TRUE(normalized.find("..") == std::string::npos);
}

/**
 * @test 获取绝对路径测试
 */
TEST(FileToolsTest, GetAbsolutePath) {
    std::string absPath = FileTools::getAbsolutePath(".");
    EXPECT_FALSE(absPath.empty());
    EXPECT_TRUE(absPath[0] == '/' || absPath[1] == ':');  // Unix或Windows
}

/**
 * @test 文件大小获取测试
 */
TEST(FileToolsTest, GetFileSize) {
    std::string tempPath = "/tmp/test_file_size.txt";

    std::string content = "Hello, World!";  // 13字节
    FileTools::writeFile(tempPath, content);

    FileSize size = FileTools::getFileSize(tempPath);
    EXPECT_EQ(size, 13);

    // 清理
    std::remove(tempPath.c_str());
}

/**
 * @test 最后修改时间测试
 */
TEST(FileToolsTest, GetLastModifiedTime) {
    std::string tempPath = "/tmp/test_mtime.txt";

    FileTools::writeFile(tempPath, "content");

    int64_t mtime = FileTools::getLastModifiedTime(tempPath);
    EXPECT_GT(mtime, 0);

    // 清理
    std::remove(tempPath.c_str());
}

/**
 * @test 临时文件创建测试
 */
TEST(FileToolsTest, CreateTempFile) {
    std::string tempFile = FileTools::createTempFile("test", ".tmp");

    EXPECT_FALSE(tempFile.empty());
    EXPECT_TRUE(tempFile.find("test") != std::string::npos);
    EXPECT_TRUE(tempFile.find(".tmp") != std::string::npos);

    // 清理
    std::remove(tempFile.c_str());
}

/**
 * @test 临时目录创建测试
 */
TEST(FileToolsTest, CreateTempDirectory) {
    std::string tempDir = FileTools::createTempDirectory("test");

    EXPECT_FALSE(tempDir.empty());
    EXPECT_TRUE(FileTools::exists(tempDir));
    EXPECT_TRUE(FileTools::isDirectory(tempDir));

    // 清理
    FileTools::removeDirectory(tempDir, true);
}

/**
 * @test 文件搜索测试
 */
TEST(FileToolsTest, FindFiles) {
    std::string tempDir = "/tmp/test_find";

    FileTools::createDirectory(tempDir);
    FileTools::writeFile(tempDir + "/file1.txt", "content1");
    FileTools::writeFile(tempDir + "/file2.txt", "content2");
    FileTools::writeFile(tempDir + "/file3.log", "content3");

    // 搜索.txt文件
    auto txtFiles = FileTools::findFiles(tempDir, "*.txt");
    EXPECT_EQ(txtFiles.size(), 2);

    // 搜索所有文件
    auto allFiles = FileTools::findFiles(tempDir, "*");
    EXPECT_EQ(allFiles.size(), 3);

    // 清理
    FileTools::removeDirectory(tempDir, true);
}

/**
 * @test 递归文件搜索测试
 */
TEST(FileToolsTest, FindFilesRecursive) {
    std::string tempDir = "/tmp/test_find_recursive";

    FileTools::createDirectory(tempDir);
    FileTools::createDirectory(tempDir + "/subdir");
    FileTools::writeFile(tempDir + "/file1.txt", "content1");
    FileTools::writeFile(tempDir + "/subdir/file2.txt", "content2");

    // 递归搜索
    auto files = FileTools::findFiles(tempDir, "*.txt", true);
    EXPECT_EQ(files.size(), 2);

    // 清理
    FileTools::removeDirectory(tempDir, true);
}

/**
 * @test 获取临时目录测试
 */
TEST(FileToolsTest, GetTempDirectory) {
    std::string tempDir = FileTools::getTempDirectory();

    EXPECT_FALSE(tempDir.empty());
    EXPECT_TRUE(FileTools::exists(tempDir));
}

/**
 * @test 获取当前目录测试
 */
TEST(FileToolsTest, GetCurrentDirectory) {
    std::string cwd = FileTools::getCurrentDirectory();

    EXPECT_FALSE(cwd.empty());
}

/**
 * @test 磁盘空间查询测试
 */
TEST(FileToolsTest, GetDiskSpace) {
    uint64_t available, capacity;

    bool success = FileTools::getDiskSpace("/", available, capacity);

    // 应该能获取磁盘信息（至少在Unix系统上）
    if (success) {
        EXPECT_GT(capacity, 0);
        EXPECT_GT(available, 0);
        EXPECT_LE(available, capacity);
    } else {
        // Windows可能失败
        SUCCEED();
    }
}
