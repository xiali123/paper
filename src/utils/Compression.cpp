#include "utils/Compression.hpp"
#include "core/Logger.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <map>
#include <unordered_map>
#include <sstream>
#include <iomanip>

// In production, include compression libraries:
// #include <zlib.h>
// #include <zstd.h>
// #include <lz4.h>
// #include <brotli/encode.h>
// #include <brotli/decode.h>

// For now, we'll use placeholder implementations
// The structure is ready for real compression libraries

namespace PaperCrawler {

// ============================================================================
// Compression Implementation
// ============================================================================

namespace {
    // Placeholder compression function
    std::vector<uint8_t> compressZlibPlaceholder(const std::string& data, int level) {
        // In production, use:
        // uLongf compressedSize = compressBound(data.size());
        // std::vector<uint8_t> compressed(compressedSize);
        // compress2(compressed.data(), &compressedSize,
        //            reinterpret_cast<const Bytef*>(data.c_str()), data.size(), level);
        // compressed.resize(compressedSize);
        // return compressed;

        // Placeholder: just copy data
        return std::vector<uint8_t>(data.begin(), data.end());
    }

    // Placeholder decompression function
    std::string decompressZlibPlaceholder(const std::vector<uint8_t>& compressed) {
        // In production, use:
        // uLongf decompressedSize = estimated_size;
        // std::vector<char> decompressed(decompressedSize);
        // uncompress(reinterpret_cast<Bytef*>(decompressed.data()), &decompressedSize,
        //           compressed.data(), compressed.size());
        // return std::string(decompressed.data(), decompressedSize);

        // Placeholder: just copy data
        return std::string(compressed.begin(), compressed.end());
    }

    const char* getCompressionPrefix(CompressionType type) {
        switch (type) {
            case CompressionType::Zlib:  return "zlib:";
            case CompressionType::Gzip:  return "gzip:";
            case CompressionType::LZ4:   return "lz4:";
            case CompressionType::Zstd:  return "zstd:";
            case CompressionType::Brotli: return "br:";
            default: return "raw:";
        }
    }

    CompressionType parseCompressionPrefix(const std::string& prefix) {
        if (prefix == "zlib") return CompressionType::Zlib;
        if (prefix == "gzip") return CompressionType::Gzip;
        if (prefix == "lz4") return CompressionType::LZ4;
        if (prefix == "zstd") return CompressionType::Zstd;
        if (prefix == "br") return CompressionType::Brotli;
        return CompressionType::None;
    }
}

CompressionResult Compression::compress(const std::string& data,
                                        CompressionType type,
                                        int level) {
    CompressionResult result;
    result.originalSize = data.size();

    // Don't compress small data
    if (data.size() < 128) {
        result.data = std::vector<uint8_t>(data.begin(), data.end());
        result.compressedSize = data.size();
        result.compressionRatio = 1.0;
        result.algorithm = CompressionType::None;
        return result;
    }

    try {
        switch (type) {
            case CompressionType::Zlib:
                result.data = compressZlibPlaceholder(data, level);
                break;

            case CompressionType::Gzip:
                // Similar to zlib but with gzip wrapper
                result.data = compressZlibPlaceholder(data, level);
                break;

            case CompressionType::LZ4:
                // Use lz4 library
                result.data = compressZlibPlaceholder(data, level);
                break;

            case CompressionType::Zstd:
                // Use zstd library
                result.data = compressZlibPlaceholder(data, level);
                break;

            case CompressionType::Brotli:
                // Use brotli library
                result.data = compressZlibPlaceholder(data, level);
                break;

            default:
                result.data = std::vector<uint8_t>(data.begin(), data.end());
                break;
        }

        result.compressedSize = result.data.size();
        result.compressionRatio = static_cast<double>(result.compressedSize) /
                                  static_cast<double>(result.originalSize);
        result.algorithm = type;

    } catch (const std::exception& e) {
        LOG_ERROR("Compression failed: {}", e.what());
        // Fallback to uncompressed
        result.data = std::vector<uint8_t>(data.begin(), data.end());
        result.compressedSize = data.size();
        result.compressionRatio = 1.0;
        result.algorithm = CompressionType::None;
    }

    return result;
}

CompressionResult Compression::compressBytes(const std::vector<uint8_t>& data,
                                             CompressionType type,
                                             int level) {
    // Convert to string and use string compression
    std::string str(data.begin(), data.end());
    return compress(str, type, level);
}

std::string Compression::decompress(const std::vector<uint8_t>& compressed,
                                     CompressionType type) {
    try {
        switch (type) {
            case CompressionType::Zlib:
            case CompressionType::Gzip:
                return decompressZlibPlaceholder(compressed);

            case CompressionType::LZ4:
            case CompressionType::Zstd:
            case CompressionType::Brotli:
                return decompressZlibPlaceholder(compressed);

            default:
                return std::string(compressed.begin(), compressed.end());
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Decompression failed: {}", e.what());
        return "";
    }
}

std::vector<uint8_t> Compression::decompressBytes(const std::vector<uint8_t>& compressed,
                                                    CompressionType type) {
    auto str = decompress(compressed, type);
    return std::vector<uint8_t>(str.begin(), str.end());
}

double Compression::estimateCompressionRatio(const std::string& data) {
    if (data.size() < 128) return 1.0;

    // Simple heuristic: count unique characters
    std::unordered_map<char, int> charFreq;
    for (char c : data) {
        charFreq[c]++;
    }

    // Calculate entropy approximation
    double entropy = 0.0;
    for (const auto& [ch, count] : charFreq) {
        double p = static_cast<double>(count) / data.size();
        entropy -= p * std::log2(p);
    }

    // Higher entropy = less compressible
    // Max entropy for ASCII is ~7.5 bits, min is 0
    double maxEntropy = 7.5;
    double compressibility = 1.0 - (entropy / maxEntropy);

    // Expected compression ratio based on compressibility
    if (compressibility > 0.5) return 0.4;  // Highly compressible
    if (compressibility > 0.3) return 0.6;  // Moderately compressible
    if (compressibility > 0.1) return 0.8;  // Slightly compressible
    return 1.0;  // Not worth compressing
}

bool Compression::shouldCompress(const std::string& data, size_t threshold) {
    if (data.size() < threshold) {
        return false;
    }

    double ratio = estimateCompressionRatio(data);
    return ratio < 0.9;  // Compress if we can save > 10%
}

int Compression::getRecommendedLevel(size_t dataSize) {
    if (dataSize < 1024) return 1;           // Fast compression for small data
    if (dataSize < 10240) return 3;          // Balanced for medium data
    if (dataSize < 102400) return 6;         // Good compression for large data
    return 9;                                // Best compression for very large data
}

std::string Compression::encodeForStorage(const std::string& data,
                                          CompressionType type) {
    auto result = compress(data, type);

    // Prefix with compression type and encode as base64
    std::ostringstream encoded;
    encoded << getCompressionPrefix(result.algorithm);

    // Encode compressed data as base64
    std::string base64 = DataEncoder::base64Encode(result.data);
    encoded << base64;

    return encoded.str();
}

std::string Compression::decodeFromStorage(const std::string& encoded) {
    if (encoded.empty()) {
        return "";
    }

    // Extract prefix
    size_t colonPos = encoded.find(':');
    if (colonPos == std::string::npos) {
        // No compression prefix, return as-is
        return encoded;
    }

    std::string prefix = encoded.substr(0, colonPos);
    std::string base64 = encoded.substr(colonPos + 1);

    CompressionType type = parseCompressionPrefix(prefix);
    auto compressed = DataEncoder::base64Decode(base64);

    return decompress(compressed, type);
}

std::string Compression::compressJson(const std::string& json) {
    // JSON typically compresses well
    if (shouldCompress(json, 512)) {
        return encodeForStorage(json, CompressionType::Zlib);
    }
    return json;
}

std::string Compression::decompressJson(const std::string& compressed) {
    if (compressed.find(':') != std::string::npos) {
        return decodeFromStorage(compressed);
    }
    return compressed;
}

// ============================================================================
// DataEncoder Implementation
// ============================================================================

namespace {
    const char* base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<uint8_t> base64DecodeInternal(const std::string& encoded) {
        std::vector<uint8_t> result;
        int bits = 0;
        int current = 0;

        for (char c : encoded) {
            if (c == '=') break;

            // Find character in base64 alphabet
            const char* pos = std::strchr(base64Chars, c);
            if (!pos) continue;  // Skip non-base64 characters

            int value = pos - base64Chars;
            bits += 6;
            current = (current << 6) | value;

            if (bits >= 8) {
                bits -= 8;
                result.push_back(static_cast<uint8_t>((current >> bits) & 0xFF));
            }
        }

        return result;
    }
}

std::string DataEncoder::base64Encode(const std::vector<uint8_t>& data) {
    std::string encoded;
    encoded.reserve((data.size() * 4 + 2) / 3);

    int bits = 0;
    uint32_t buffer = 0;

    for (uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bits += 8;

        while (bits >= 6) {
            bits -= 6;
            encoded += base64Chars[(buffer >> bits) & 0x3F];
        }
    }

    if (bits > 0) {
        encoded += base64Chars[(buffer << (6 - bits)) & 0x3F];
    }

    // Add padding
    while (encoded.size() % 4 != 0) {
        encoded += '=';
    }

    return encoded;
}

std::vector<uint8_t> DataEncoder::base64Decode(const std::string& encoded) {
    return base64DecodeInternal(encoded);
}

std::string DataEncoder::hexEncode(const std::vector<uint8_t>& data) {
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');

    for (uint8_t byte : data) {
        hex << std::setw(2) << static_cast<int>(byte);
    }

    return hex.str();
}

std::vector<uint8_t> DataEncoder::hexDecode(const std::string& encoded) {
    std::vector<uint8_t> data;

    for (size_t i = 0; i < encoded.size(); i += 2) {
        std::string byteStr = encoded.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
        data.push_back(byte);
    }

    return data;
}

std::string DataEncoder::base64UrlEncode(const std::vector<uint8_t>& data) {
    std::string encoded = base64Encode(data);

    // Make URL-safe
    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');

    // Remove padding
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '='), encoded.end());

    return encoded;
}

std::vector<uint8_t> DataEncoder::base64UrlDecode(const std::string& encoded) {
    std::string modified = encoded;

    // Convert back to standard base64
    std::replace(modified.begin(), modified.end(), '-', '+');
    std::replace(modified.begin(), modified.end(), '_', '/');

    // Add padding
    while (modified.size() % 4 != 0) {
        modified += '=';
    }

    return base64DecodeInternal(modified);
}

// ============================================================================
// DictionaryCompressor Implementation
// ============================================================================

namespace {
    std::unordered_map<int, std::map<std::string, int>> dictionaries;
    std::unordered_map<int, std::vector<std::string>> reverseDictionaries;
    int nextDictionaryId = 1;
    std::mutex dictMutex;
}

int DictionaryCompressor::buildDictionary(const std::vector<std::string>& strings) {
    std::lock_guard<std::mutex> lock(dictMutex);

    int dictId = nextDictionaryId++;
    std::map<std::string, int>& dict = dictionaries[dictId];
    reverseDictionaries[dictId] = strings;

    // Assign IDs to unique strings
    std::vector<std::string> uniqueStrings = strings;
    std::sort(uniqueStrings.begin(), uniqueStrings.end());
    uniqueStrings.erase(
        std::unique(uniqueStrings.begin(), uniqueStrings.end()),
        uniqueStrings.end()
    );

    for (size_t i = 0; i < uniqueStrings.size(); ++i) {
        dict[uniqueStrings[i]] = static_cast<int>(i);
    }

    LOG_INFO("Built dictionary {} with {} entries", dictId, dict.size());
    return dictId;
}

std::vector<uint8_t> DictionaryCompressor::compressWithDictionary(
    const std::string& data,
    int dictionaryId) {

    std::lock_guard<std::mutex> lock(dictMutex);

    auto it = dictionaries.find(dictionaryId);
    if (it == dictionaries.end()) {
        throw std::runtime_error("Dictionary not found: " + std::to_string(dictionaryId));
    }

    const auto& dict = it->second;

    // Simple encoding: replace dictionary words with IDs
    std::vector<uint8_t> result;
    std::istringstream stream(data);
    std::string word;

    while (stream >> word) {
        auto dictIt = dict.find(word);
        if (dictIt != dict.end()) {
            // Encode as variable-length integer
            int id = dictIt->second;
            while (id > 0x7F) {
                result.push_back(static_cast<uint8_t>((id & 0x7F) | 0x80));
                id >>= 7;
            }
            result.push_back(static_cast<uint8_t>(id));
        } else {
            // Not in dictionary, keep as-is (preceded by 0xFF)
            result.push_back(0xFF);
            result.insert(result.end(), word.begin(), word.end());
            result.push_back(0);
        }
    }

    return result;
}

std::string DictionaryCompressor::decompressWithDictionary(
    const std::vector<uint8_t>& compressed,
    int dictionaryId) {

    std::lock_guard<std::mutex> lock(dictMutex);

    auto it = reverseDictionaries.find(dictionaryId);
    if (it == reverseDictionaries.end()) {
        throw std::runtime_error("Dictionary not found: " + std::to_string(dictionaryId));
    }

    const auto& dict = it->second;
    std::ostringstream result;

    for (size_t i = 0; i < compressed.size(); ) {
        if (compressed[i] == 0xFF) {
            // Literal string
            i++;
            while (i < compressed.size() && compressed[i] != 0) {
                result << compressed[i];
                i++;
            }
            i++;
            result << " ";
        } else {
            // Dictionary ID
            int id = 0;
            int shift = 0;
            while (i < compressed.size() && (compressed[i] & 0x80)) {
                id |= (compressed[i] & 0x7F) << shift;
                shift += 7;
                i++;
            }
            if (i < compressed.size()) {
                id |= compressed[i] << shift;
                i++;
            }

            if (id >= 0 && id < static_cast<int>(dict.size())) {
                result << dict[id] << " ";
            }
        }
    }

    return result.str();
}

size_t DictionaryCompressor::getDictionarySize(int dictionaryId) {
    std::lock_guard<std::mutex> lock(dictMutex);

    auto it = dictionaries.find(dictionaryId);
    if (it != dictionaries.end()) {
        return it->second.size();
    }
    return 0;
}

void DictionaryCompressor::removeDictionary(int dictionaryId) {
    std::lock_guard<std::mutex> lock(dictMutex);

    dictionaries.erase(dictionaryId);
    reverseDictionaries.erase(dictionaryId);

    LOG_DEBUG("Removed dictionary {}", dictionaryId);
}

// ============================================================================
// DeltaEncoder Implementation
// ============================================================================

std::vector<uint8_t> DeltaEncoder::encode(const std::vector<int64_t>& values) {
    if (values.empty()) return {};

    std::vector<uint8_t> result;

    // Store first value as-is
    int64_t prev = 0;
    for (int64_t val : values) {
        int64_t delta = val - prev;

        // Encode delta as variable-length integer (zigzag encoding)
        uint64_t zigzag = (static_cast<uint64_t>(delta) << 1) ^ (delta >> 63);

        while (zigzag > 0x7F) {
            result.push_back(static_cast<uint8_t>((zigzag & 0x7F) | 0x80));
            zigzag >>= 7;
        }
        result.push_back(static_cast<uint8_t>(zigzag));

        prev = val;
    }

    return result;
}

std::vector<int64_t> DeltaEncoder::decode(const std::vector<uint8_t>& encoded) {
    std::vector<int64_t> values;
    int64_t current = 0;

    for (size_t i = 0; i < encoded.size(); ) {
        // Decode variable-length integer
        uint64_t zigzag = 0;
        int shift = 0;

        while (i < encoded.size() && (encoded[i] & 0x80)) {
            zigzag |= (encoded[i] & 0x7F) << shift;
            shift += 7;
            i++;
        }

        if (i < encoded.size()) {
            zigzag |= encoded[i] << shift;
            i++;

            // Decode zigzag
            int64_t delta = (zigzag >> 1) ^ -(zigzag & 1);
            current += delta;
            values.push_back(current);
        }
    }

    return values;
}

std::vector<uint8_t> DeltaEncoder::encodeVarint(const std::vector<int64_t>& values) {
    // Similar to encode but with optimized varint encoding
    return encode(values);
}

std::vector<int64_t> DeltaEncoder::decodeVarint(const std::vector<uint8_t>& encoded) {
    return decode(encoded);
}

// ============================================================================
// Bitmap Implementation
// ============================================================================

std::vector<uint8_t> Bitmap::fromBools(const std::vector<bool>& flags) {
    std::vector<uint8_t> bitmap((flags.size() + 7) / 8, 0);

    for (size_t i = 0; i < flags.size(); ++i) {
        if (flags[i]) {
            bitmap[i / 8] |= (1 << (i % 8));
        }
    }

    return bitmap;
}

std::vector<bool> Bitmap::toBools(const std::vector<uint8_t>& bitmap, size_t count) {
    std::vector<bool> flags(count);

    for (size_t i = 0; i < count; ++i) {
        flags[i] = getBit(bitmap, i);
    }

    return flags;
}

void Bitmap::setBit(std::vector<uint8_t>& bitmap, size_t index, bool value) {
    if (index >= bitmap.size() * 8) {
        bitmap.resize((index / 8) + 1, 0);
    }

    if (value) {
        bitmap[index / 8] |= (1 << (index % 8));
    } else {
        bitmap[index / 8] &= ~(1 << (index % 8));
    }
}

bool Bitmap::getBit(const std::vector<uint8_t>& bitmap, size_t index) {
    if (index >= bitmap.size() * 8) {
        return false;
    }

    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

size_t Bitmap::countBits(const std::vector<uint8_t>& bitmap) {
    size_t count = 0;

    for (uint8_t byte : bitmap) {
        // Population count algorithm
        while (byte) {
            count += byte & 1;
            byte >>= 1;
        }
    }

    return count;
}

std::vector<uint8_t> Bitmap::compressRle(const std::vector<uint8_t>& bitmap) {
    std::vector<uint8_t> compressed;

    if (bitmap.empty()) return compressed;

    uint8_t current = bitmap[0];
    int runLength = 1;

    for (size_t i = 1; i < bitmap.size(); ++i) {
        if (bitmap[i] == current && runLength < 127) {
            runLength++;
        } else {
            // Write run
            compressed.push_back(static_cast<uint8_t>(runLength));
            compressed.push_back(current);
            current = bitmap[i];
            runLength = 1;
        }
    }

    // Write final run
    compressed.push_back(static_cast<uint8_t>(runLength));
    compressed.push_back(current);

    return compressed;
}

std::vector<uint8_t> Bitmap::decompressRle(const std::vector<uint8_t>& compressed) {
    std::vector<uint8_t> bitmap;

    for (size_t i = 0; i < compressed.size(); i += 2) {
        int runLength = compressed[i];
        uint8_t value = compressed[i + 1];

        for (int j = 0; j < runLength; ++j) {
            bitmap.push_back(value);
        }
    }

    return bitmap;
}

// ============================================================================
// StringPool Implementation
// ============================================================================

namespace {
    std::unordered_map<std::string, int> stringToId;
    std::vector<std::string> idToString;
    std::mutex poolMutex;
}

int StringPool::intern(const std::string& str) {
    std::lock_guard<std::mutex> lock(poolMutex);

    auto it = stringToId.find(str);
    if (it != stringToId.end()) {
        return it->second;
    }

    int id = static_cast<int>(idToString.size());
    stringToId[str] = id;
    idToString.push_back(str);

    return id;
}

std::string StringPool::getString(int id) {
    std::lock_guard<std::mutex> lock(poolMutex);

    if (id >= 0 && id < static_cast<int>(idToString.size())) {
        return idToString[id];
    }

    return "";
}

std::optional<int> StringPool::findId(const std::string& str) {
    std::lock_guard<std::mutex> lock(poolMutex);

    auto it = stringToId.find(str);
    if (it != stringToId.end()) {
        return it->second;
    }

    return std::nullopt;
}

StringPool::PoolStats StringPool::getStats() {
    std::lock_guard<std::mutex> lock(poolMutex);

    PoolStats stats;
    stats.stringCount = static_cast<int>(idToString.size());

    size_t totalBytes = 0;
    for (const auto& str : idToString) {
        totalBytes += str.size();
    }
    stats.totalBytes = totalBytes;

    // Estimate saved bytes (assuming average 3 copies per string)
    stats.savedBytes = totalBytes * 2;
    stats.deduplicationRatio = static_cast<double>(stats.savedBytes) /
                               static_cast<double>(stats.totalBytes + stats.savedBytes);

    return stats;
}

void StringPool::clear() {
    std::lock_guard<std::mutex> lock(poolMutex);

    stringToId.clear();
    idToString.clear();
}

std::vector<uint8_t> StringPool::exportPool() {
    std::lock_guard<std::mutex> lock(poolMutex);

    nlohmann::json json;
    json["strings"] = idToString;

    std::string str = json.dump();
    return std::vector<uint8_t>(str.begin(), str.end());
}

bool StringPool::importPool(const std::vector<uint8_t>& data) {
    try {
        std::string str(data.begin(), data.end());
        auto json = nlohmann::json::parse(str);

        if (json.contains("strings")) {
            std::lock_guard<std::mutex> lock(poolMutex);

            idToString = json["strings"].get<std::vector<std::string>>();
            stringToId.clear();

            for (size_t i = 0; i < idToString.size(); ++i) {
                stringToId[idToString[i]] = static_cast<int>(i);
            }

            return true;
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to import string pool: {}", e.what());
    }

    return false;
}

// ============================================================================
// PaperMetadataCompressor Implementation
// ============================================================================

std::vector<uint8_t> PaperMetadataCompressor::compressMetadata(
    int paperId,
    const std::string& title,
    const std::string& authors,
    int year,
    const std::string& level) {

    // Use string pool for title and authors
    int titleId = StringPool::intern(title);
    int authorsId = StringPool::intern(authors);
    int levelId = StringPool::intern(level);

    // Compact binary format:
    // [paper_id:4 bytes][title_id:varint][authors_id:varint][year:2 bytes][level_id:varint]

    std::vector<uint8_t> compressed;

    // Paper ID (4 bytes)
    compressed.push_back(static_cast<uint8_t>(paperId >> 24));
    compressed.push_back(static_cast<uint8_t>(paperId >> 16));
    compressed.push_back(static_cast<uint8_t>(paperId >> 8));
    compressed.push_back(static_cast<uint8_t>(paperId));

    // Title ID (varint)
    uint32_t val = titleId;
    while (val > 0x7F) {
        compressed.push_back(static_cast<uint8_t>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    compressed.push_back(static_cast<uint8_t>(val));

    // Authors ID (varint)
    val = authorsId;
    while (val > 0x7F) {
        compressed.push_back(static_cast<uint8_t>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    compressed.push_back(static_cast<uint8_t>(val));

    // Year (2 bytes)
    compressed.push_back(static_cast<uint8_t>(year >> 8));
    compressed.push_back(static_cast<uint8_t>(year));

    // Level ID (varint)
    val = levelId;
    while (val > 0x7F) {
        compressed.push_back(static_cast<uint8_t>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    compressed.push_back(static_cast<uint8_t>(val));

    return compressed;
}

bool PaperMetadataCompressor::decompressMetadata(
    const std::vector<uint8_t>& compressed,
    int& outPaperId,
    std::string& outTitle,
    std::string& outAuthors,
    int& outYear,
    std::string& outLevel) {

    if (compressed.size() < 8) {
        return false;
    }

    size_t pos = 0;

    // Paper ID
    outPaperId = (compressed[pos++] << 24) |
                 (compressed[pos++] << 16) |
                 (compressed[pos++] << 8) |
                 compressed[pos++];

    // Title ID (varint)
    uint32_t titleId = 0;
    int shift = 0;
    while (pos < compressed.size() && (compressed[pos] & 0x80)) {
        titleId |= (compressed[pos] & 0x7F) << shift;
        shift += 7;
        pos++;
    }
    if (pos < compressed.size()) {
        titleId |= compressed[pos++] << shift;
    }
    outTitle = StringPool::getString(titleId);

    // Authors ID (varint)
    uint32_t authorsId = 0;
    shift = 0;
    while (pos < compressed.size() && (compressed[pos] & 0x80)) {
        authorsId |= (compressed[pos] & 0x7F) << shift;
        shift += 7;
        pos++;
    }
    if (pos < compressed.size()) {
        authorsId |= compressed[pos++] << shift;
    }
    outAuthors = StringPool::getString(authorsId);

    // Year
    if (pos + 1 >= compressed.size()) {
        return false;
    }
    outYear = (compressed[pos++] << 8) | compressed[pos++];

    // Level ID (varint)
    uint32_t levelId = 0;
    shift = 0;
    while (pos < compressed.size() && (compressed[pos] & 0x80)) {
        levelId |= (compressed[pos] & 0x7F) << shift;
        shift += 7;
        pos++;
    }
    if (pos < compressed.size()) {
        levelId |= compressed[pos++] << shift;
    }
    outLevel = StringPool::getString(levelId);

    return true;
}

std::vector<uint8_t> PaperMetadataCompressor::compressBatch(
    const std::vector<std::tuple<int, std::string, std::string, int, std::string>>& papers) {

    std::vector<uint8_t> batch;

    for (const auto& [id, title, authors, year, level] : papers) {
        auto compressed = compressMetadata(id, title, authors, year, level);
        batch.insert(batch.end(), compressed.begin(), compressed.end());
    }

    return batch;
}

std::vector<std::tuple<int, std::string, std::string, int, std::string>>
PaperMetadataCompressor::decompressBatch(const std::vector<uint8_t>& compressed) {

    std::vector<std::tuple<int, std::string, std::string, int, std::string>> papers;

    size_t pos = 0;
    while (pos < compressed.size()) {
        int id, year;
        std::string title, authors, level;

        // Find size of this entry
        size_t startPos = pos;

        // Skip fixed fields (6 bytes)
        pos += 6;

        // Skip varint fields
        for (int i = 0; i < 3 && pos < compressed.size(); ++i) {
            while (pos < compressed.size() && (compressed[pos] & 0x80)) {
                pos++;
            }
            if (pos < compressed.size()) pos++;
        }

        if (pos > compressed.size()) break;

        // Extract this entry
        std::vector<uint8_t> entry(compressed.begin() + startPos,
                                   compressed.begin() + pos);

        if (decompressMetadata(entry, id, title, authors, year, level)) {
            papers.emplace_back(id, title, authors, year, level);
        }
    }

    return papers;
}

} // namespace PaperCrawler
