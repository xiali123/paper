#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace PaperCrawler {

/**
 * @brief Compression algorithm type
 */
enum class CompressionType {
    None,       // No compression
    Zlib,       // Zlib compression (RFC 1950)
    Gzip,       // Gzip compression (RFC 1952)
    LZ4,        // LZ4 fast compression
    Zstd,       // Zstandard compression
    Brotli      // Brotli compression
};

/**
 * @brief Compression result
 */
struct CompressionResult {
    std::vector<uint8_t> data;
    size_t originalSize{0};
    size_t compressedSize{0};
    double compressionRatio{0.0};  // compressed / original
    CompressionType algorithm{CompressionType::None};
};

/**
 * @brief Data compression utility
 *
 * Provides compression/decompression for large text data:
 * - Paper abstracts
 * - Author lists
 * - JSON payloads
 * - Database exports
 */
class Compression {
public:
    /**
     * @brief Compress data using specified algorithm
     * @param data Input data
     * @param type Compression algorithm
     * @param level Compression level (1-9, higher = better compression but slower)
     * @return Compression result
     */
    static CompressionResult compress(const std::string& data,
                                      CompressionType type = CompressionType::Zlib,
                                      int level = 6);

    /**
     * @brief Compress raw bytes
     */
    static CompressionResult compressBytes(const std::vector<uint8_t>& data,
                                           CompressionType type = CompressionType::Zlib,
                                           int level = 6);

    /**
     * @brief Decompress data
     * @param compressed Compressed data
     * @param type Compression algorithm used
     * @return Decompressed string
     */
    static std::string decompress(const std::vector<uint8_t>& compressed,
                                   CompressionType type = CompressionType::Zlib);

    /**
     * @brief Decompress to raw bytes
     */
    static std::vector<uint8_t> decompressBytes(const std::vector<uint8_t>& compressed,
                                                  CompressionType type = CompressionType::Zlib);

    /**
     * @brief Estimate compression ratio without actually compressing
     * @param data Input data
     * @return Estimated ratio (0.0-1.0, lower is better)
     */
    static double estimateCompressionRatio(const std::string& data);

    /**
     * @brief Check if data is worth compressing
     * @param data Input data
     * @return true if compression would be beneficial
     */
    static bool shouldCompress(const std::string& data,
                               size_t threshold = 1024);  // 1KB default

    /**
     * @brief Get recommended compression level for data size
     * @param dataSize Size of data in bytes
     * @return Recommended level (1-9)
     */
    static int getRecommendedLevel(size_t dataSize);

    /**
     * @brief Compress string for database storage (with metadata)
     * @param data Input string
     * @return Base64-encoded compressed data with type prefix
     */
    static std::string encodeForStorage(const std::string& data,
                                        CompressionType type = CompressionType::Zlib);

    /**
     * @brief Decompress string from database storage
     * @param encoded Base64-encoded compressed data with type prefix
     * @return Original string
     */
    static std::string decodeFromStorage(const std::string& encoded);

    /**
     * @brief Compress JSON payload for API transfer
     * @param json JSON string
     * @return Compressed and encoded string
     */
    static std::string compressJson(const std::string& json);

    /**
     * @brief Decompress JSON payload from API transfer
     * @param compressed Compressed JSON string
     * @return Original JSON string
     */
    static std::string decompressJson(const std::string& compressed);
};

/**
 * @brief Binary data encoder/decoder
 */
class DataEncoder {
public:
    /**
     * @brief Encode binary data to Base64
     */
    static std::string base64Encode(const std::vector<uint8_t>& data);

    /**
     * @brief Decode Base64 to binary data
     */
    static std::vector<uint8_t> base64Decode(const std::string& encoded);

    /**
     * @brief Encode binary data to Hex
     */
    static std::string hexEncode(const std::vector<uint8_t>& data);

    /**
     * @brief Decode Hex to binary data
     */
    static std::vector<uint8_t> hexDecode(const std::string& encoded);

    /**
     * @brief Encode string to URL-safe Base64
     */
    static std::string base64UrlEncode(const std::vector<uint8_t>& data);

    /**
     * @brief Decode URL-safe Base64 to binary data
     */
    static std::vector<uint8_t> base64UrlDecode(const std::string& encoded);
};

/**
 * @brief String compression using dictionary encoding
 *
 * For repetitive text data like journal names, author names, etc.
 */
class DictionaryCompressor {
public:
    /**
     * @brief Build dictionary from list of strings
     * @param strings List of strings to build dictionary from
     * @return Dictionary ID for later use
     */
    static int buildDictionary(const std::vector<std::string>& strings);

    /**
     * @brief Compress string using dictionary
     * @param data Input string
     * @param dictionaryId Dictionary to use
     * @return Compressed data
     */
    static std::vector<uint8_t> compressWithDictionary(
        const std::string& data,
        int dictionaryId
    );

    /**
     * @brief Decompress string using dictionary
     */
    static std::string decompressWithDictionary(
        const std::vector<uint8_t>& compressed,
        int dictionaryId
    );

    /**
     * @brief Get dictionary size in bytes
     */
    static size_t getDictionarySize(int dictionaryId);

    /**
     * @brief Remove dictionary from memory
     */
    static void removeDictionary(int dictionaryId);
};

/**
 * @brief Delta encoding for numeric sequences
 *
 * Efficient storage of sorted data like:
 * - Paper IDs
 * - Years
 * - Citation counts
 */
class DeltaEncoder {
public:
    /**
     * @brief Encode sorted integer array using delta encoding
     * @param values Sorted integer array
     * @return Encoded bytes
     */
    static std::vector<uint8_t> encode(const std::vector<int64_t>& values);

    /**
     * @brief Decode delta-encoded integer array
     */
    static std::vector<int64_t> decode(const std::vector<uint8_t>& encoded);

    /**
     * @brief Encode sorted integer array with variable-byte encoding
     * More compact for small deltas
     */
    static std::vector<uint8_t> encodeVarint(const std::vector<int64_t>& values);

    /**
     * @brief Decode variable-byte delta-encoded array
     */
    static std::vector<int64_t> decodeVarint(const std::vector<uint8_t>& encoded);
};

/**
 * @brief Bitmap for boolean flags
 *
 * Compact storage for boolean arrays like:
 * - Read/unread status
 * - Bookmark flags
 * - Presence flags
 */
class Bitmap {
public:
    /**
     * @brief Create bitmap from boolean array
     */
    static std::vector<uint8_t> fromBools(const std::vector<bool>& flags);

    /**
     * @brief Convert bitmap to boolean array
     */
    static std::vector<bool> toBools(const std::vector<uint8_t>& bitmap, size_t count);

    /**
     * @brief Set bit in bitmap
     */
    static void setBit(std::vector<uint8_t>& bitmap, size_t index, bool value);

    /**
     * @brief Get bit from bitmap
     */
    static bool getBit(const std::vector<uint8_t>& bitmap, size_t index);

    /**
     * @brief Count set bits (population count)
     */
    static size_t countBits(const std::vector<uint8_t>& bitmap);

    /**
     * @brief Compress bitmap using RLE (run-length encoding)
     * Effective for consecutive identical bits
     */
    static std::vector<uint8_t> compressRle(const std::vector<uint8_t>& bitmap);

    /**
     * @brief Decompress RLE-compressed bitmap
     */
    static std::vector<uint8_t> decompressRle(const std::vector<uint8_t>& compressed);
};

/**
 * @brief String deduplication using intern pool
 *
 * For repeated strings like:
 * - Journal names
 * - Author names
 * - Keywords
 */
class StringPool {
public:
    /**
     * @brief Intern string and get unique ID
     * @param str String to intern
     * @return Unique ID for this string
     */
    static int intern(const std::string& str);

    /**
     * @brief Get string by ID
     */
    static std::string getString(int id);

    /**
     * @brief Check if string is already interned
     */
    static std::optional<int> findId(const std::string& str);

    /**
     * @brief Get pool statistics
     */
    struct PoolStats {
        int stringCount{0};
        size_t totalBytes{0};
        size_t savedBytes{0};  // Bytes saved vs storing duplicates
        double deduplicationRatio{0.0};  // saved / total
    };

    static PoolStats getStats();

    /**
     * @brief Clear pool
     */
    static void clear();

    /**
     * @brief Export pool to bytes
     */
    static std::vector<uint8_t> exportPool();

    /**
     * @brief Import pool from bytes
     */
    static bool importPool(const std::vector<uint8_t>& data);
};

/**
 * @brief Compact storage for paper metadata
 */
class PaperMetadataCompressor {
public:
    /**
     * @brief Compress paper metadata to compact binary format
     * @param paperId Paper ID
     * @param title Paper title
     * @param authors Author list
     * @param year Publication year
     * @param level CCF level
     * @return Compressed bytes
     */
    static std::vector<uint8_t> compressMetadata(
        int paperId,
        const std::string& title,
        const std::string& authors,
        int year,
        const std::string& level
    );

    /**
     * @brief Decompress metadata
     */
    static bool decompressMetadata(
        const std::vector<uint8_t>& compressed,
        int& outPaperId,
        std::string& outTitle,
        std::string& outAuthors,
        int& outYear,
        std::string& outLevel
    );

    /**
     * @brief Batch compress multiple papers
     */
    static std::vector<uint8_t> compressBatch(
        const std::vector<std::tuple<int, std::string, std::string, int, std::string>>& papers
    );

    /**
     * @brief Batch decompress papers
     */
    static std::vector<std::tuple<int, std::string, std::string, int, std::string>>
    decompressBatch(const std::vector<uint8_t>& compressed);
};

} // namespace PaperCrawler
