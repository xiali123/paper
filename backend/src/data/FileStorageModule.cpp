#include "data/FileStorageModule.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>

namespace PaperCrawler {

FileStorageModule::FileStorageModule() = default;
FileStorageModule::~FileStorageModule() = default;

bool FileStorageModule::initialize() {
    namespace fs = std::filesystem;

    // 创建基础目录
    try {
        if (!fs::exists(config_.basePath)) {
            fs::create_directories(config_.basePath);
            std::cout << "Created storage directory: " << config_.basePath << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create storage directory: " << e.what() << std::endl;
        return false;
    }

    std::cout << "FileStorageModule initialized: " << config_.basePath << std::endl;
    return true;
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
    stop();
}

MessageResponse FileStorageModule::save(const UnifiedMessage& message) {
    MessageResponse response;
    response.messageId = message.messageId;

    try {
        // 获取文件数据
        std::vector<uint8_t> fileData;
        try {
            fileData = std::any_cast<std::vector<uint8_t>>(message.payload);
        } catch (...) {
            response.success = false;
            response.errorMessage = "Invalid payload data";
            return response;
        }

        // 获取原始文件名
        std::string originalFilename = message.getParameter<std::string>("filename", "upload.dat");

        // 验证文件类型
        if (!isValidFileType(originalFilename)) {
            response.success = false;
            response.errorMessage = "Invalid file type: " + originalFilename;
            return response;
        }

        // 验证文件大小
        if (fileData.size() > config_.maxFileSize) {
            response.success = false;
            response.errorMessage = "File too large: " + std::to_string(fileData.size()) + " bytes";
            return response;
        }

        // 生成唯一文件名
        std::string uniqueFilename = generateUniqueFilename(originalFilename);
        std::string filepath = config_.basePath + "/" + uniqueFilename;

        // 保存文件
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            response.success = false;
            response.errorMessage = "Failed to create file: " + filepath;
            return response;
        }

        file.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
        file.close();

        // 生成URL
        std::string fileUrl = getFileUrl(uniqueFilename);

        response.success = true;
        response.data = fileUrl;

        std::cout << "File saved: " << uniqueFilename << " (" << fileData.size() << " bytes)" << std::endl;

    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = e.what();
    }

    return response;
}

MessageResponse FileStorageModule::load(const UnifiedMessage& message) {
    MessageResponse response;
    response.messageId = message.messageId;

    try {
        std::string filename = message.targetName;
        std::string filepath = config_.basePath + "/" + filename;

        if (!std::filesystem::exists(filepath)) {
            response.success = false;
            response.errorMessage = "File not found: " + filename;
            return response;
        }

        // 读取文件
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            response.success = false;
            response.errorMessage = "Failed to open file: " + filepath;
            return response;
        }

        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        file.close();

        response.success = true;
        response.data = fileData;

    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = e.what();
    }

    return response;
}

MessageResponse FileStorageModule::deleteFile(const UnifiedMessage& message) {
    MessageResponse response;
    response.messageId = message.messageId;

    try {
        std::string filename = message.targetName;
        std::string filepath = config_.basePath + "/" + filename;

        if (!std::filesystem::exists(filepath)) {
            response.success = false;
            response.errorMessage = "File not found: " + filename;
            return response;
        }

        std::filesystem::remove(filepath);
        response.success = true;

        std::cout << "File deleted: " << filename << std::endl;

    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = e.what();
    }

    return response;
}

MessageResponse FileStorageModule::exists(const UnifiedMessage& message) {
    MessageResponse response;
    response.messageId = message.messageId;

    std::string filename = message.targetName;
    std::string filepath = config_.basePath + "/" + filename;

    response.success = std::filesystem::exists(filepath);
    return response;
}

std::vector<std::string> FileStorageModule::listFiles(const std::string& directory) {
    std::vector<std::string> files;
    std::string dirPath = config_.basePath + "/" + directory;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing files: " << e.what() << std::endl;
    }

    return files;
}

bool FileStorageModule::deleteDirectory(const std::string& directory) {
    try {
        std::string dirPath = config_.basePath + "/" + directory;
        return std::filesystem::remove_all(dirPath);
    } catch (const std::exception& e) {
        std::cerr << "Error deleting directory: " << e.what() << std::endl;
        return false;
    }
}

std::string FileStorageModule::getFileUrl(const std::string& filePath) {
    return config_.urlPrefix + "/" + filePath;
}

bool FileStorageModule::isValidFileType(const std::string& filename) {
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

std::string FileStorageModule::generateUniqueFilename(const std::string& originalFilename) {
    // 提取扩展名
    size_t dotPos = originalFilename.find_last_of('.');
    std::string ext = (dotPos != std::string::npos) ? originalFilename.substr(dotPos) : "";

    // 生成随机文件名
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    std::string filename = std::to_string(timestamp) + "_" + std::to_string(dis(gen)) + ext;
    return filename;
}

} // namespace PaperCrawler
