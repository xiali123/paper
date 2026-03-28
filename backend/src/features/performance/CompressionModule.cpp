#include "performance/CompressionModule.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

// 简单的mock压缩实现
// TODO: 集成实际的压缩库（zlib, brotli, zstd）

namespace PaperCrawler {

// ============================================================================
// CompressionModule
// ============================================================================

CompressionModule::CompressionModule() {
    stats_ = CompressionStats{};
}

CompressionModule::~CompressionModule() = default;

bool CompressionModule::initialize() {
    std::cout << "CompressionModule::initialize" << std::endl;
    std::cout << "  Algorithm: ";

    switch (config_.algorithm) {
        case CompressionAlgorithm::GZIP:
            std::cout << "GZIP";
            break;
        case CompressionAlgorithm::BROTLI:
            std::cout << "BROTLI";
            break;
        case CompressionAlgorithm::ZSTD:
            std::cout << "ZSTD";
            break;
        case CompressionAlgorithm::LZ4:
            std::cout << "LZ4";
            break;
        case CompressionAlgorithm::SNAPPY:
            std::cout << "SNAPPY";
            break;
    }

    std::cout << std::endl;
    std::cout << "  Compression level: " << config_.compressionLevel << std::endl;
    std::cout << "  Threshold: " << config_.thresholdBytes << " bytes" << std::endl;

    return true;
}

bool CompressionModule::start() {
    std::cout << "CompressionModule started (Mock mode)" << std::endl;
    return true;
}

bool CompressionModule::stop() {
    std::cout << "CompressionModule stopped" << std::endl;
    return true;
}

void CompressionModule::cleanup() {
    // 清理资源
}

std::vector<uint8_t> CompressionModule::compress(
    const std::vector<uint8_t>& data,
    CompressionAlgorithm algorithm
) {
    if (data.size() < config_.thresholdBytes) {
        // 数据太小，不值得压缩
        return data;
    }

    // TODO: 实际压缩实现
    // 这里使用简单的mock实现
    std::vector<uint8_t> compressed = data;

    // 模拟压缩：添加压缩标记头
    compressed.insert(compressed.begin(), 0x1F);  // 压缩标记
    compressed.insert(compressed.begin() + 1, 0x8B); // Gzip magic number

    updateStats(data.size(), compressed.size());

    std::cout << "[Compression] Compressed: " << data.size()
              << " → " << compressed.size() << " bytes" << std::endl;

    return compressed;
}

std::vector<uint8_t> CompressionModule::decompress(
    const std::vector<uint8_t>& compressedData,
    CompressionAlgorithm algorithm
) {
    // TODO: 实际解压实现
    // 检查压缩标记
    if (compressedData.size() >= 2 &&
        compressedData[0] == 0x1F &&
        compressedData[1] == 0x8B) {

        // 移除压缩标记头
        std::vector<uint8_t> decompressed(compressedData.begin() + 2, compressedData.end());

        stats_.totalDecompressed++;

        std::cout << "[Compression] Decompressed: " << compressedData.size()
                  << " → " << decompressed.size() << " bytes" << std::endl;

        return decompressed;
    }

    // 未压缩数据，直接返回
    return compressedData;
}

std::string CompressionModule::compressString(const std::string& str) {
    if (str.size() < config_.thresholdBytes) {
        return str;
    }

    std::vector<uint8_t> data(str.begin(), str.end());
    auto compressed = compress(data, config_.algorithm);

    // Base64编码（简化版）
    static const char* encodeTable =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve((compressed.size() * 4 + 2) / 3);

    for (size_t i = 0; i < compressed.size(); i += 3) {
        uint32_t triple = (compressed[i] << 16) |
                         (i + 1 < compressed.size() ? compressed[i + 1] << 8 : 0) |
                         (i + 2 < compressed.size() ? compressed[i + 2] : 0);

        result.push_back(encodeTable[(triple >> 18) & 0x3F]);
        result.push_back(encodeTable[(triple >> 12) & 0x3F]);
        result.push_back(encodeTable[(triple >> 6) & 0x3F]);
        result.push_back(encodeTable[triple & 0x3F]);
    }

    // 添加padding
    while (result.size() % 4 != 0) {
        result.push_back('=');
    }

    std::cout << "[Compression] String compressed: " << str.size()
              << " → " << result.size() << " chars" << std::endl;

    return result;
}

std::string CompressionModule::decompressString(const std::string& compressedStr) {
    // TODO: 实现Base64解码和解压
    // 简化版：直接返回原字符串
    return compressedStr;
}

bool CompressionModule::shouldCompress(const std::string& contentType, size_t dataSize) {
    // 检查数据大小
    if (dataSize < config_.thresholdBytes) {
        return false;
    }

    // 检查内容类型
    if (!config_.enableForContentType) {
        return false;
    }

    // 可压缩的内容类型
    static const std::vector<std::string> compressibleTypes = {
        "application/json",
        "text/html",
        "text/plain",
        "text/css",
        "text/javascript",
        "application/javascript",
        "application/xml",
        "text/xml"
    };

    for (const auto& type : compressibleTypes) {
        if (contentType.find(type) != std::string::npos) {
            return true;
        }
    }

    // 图片、视频等已压缩格式不再次压缩
    static const std::vector<std::string> alreadyCompressed = {
        "image/",
        "video/",
        "audio/",
        "application/zip",
        "application/gzip"
    };

    for (const auto& type : alreadyCompressed) {
        if (contentType.find(type) != std::string::npos) {
            return false;
        }
    }

    return false;
}

CompressionModule::CompressionStats CompressionModule::getStats() const {
    return stats_;
}

void CompressionModule::updateStats(size_t originalSize, size_t compressedSize) {
    stats_.totalCompressed++;
    stats_.originalBytes += originalSize;
    stats_.compressedBytes += compressedSize;

    if (stats_.originalBytes > 0) {
        stats_.compressionRatio =
            1.0 - static_cast<double>(stats_.compressedBytes) / stats_.originalBytes;
    }
}

} // namespace PaperCrawler
