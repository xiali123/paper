#pragma once

#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include "auth/AuthManager.hpp"

namespace PaperCrawler {

/**
 * @brief JWT utility functions
 *
 * Uses OpenSSL for signing and verification
 * HS256 algorithm (HMAC-SHA256)
 *
 * JWT Format: header.payload.signature
 * Example: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0.dozjgNryP4J3jVmNHl0w5N_XgL0n3I9PlFUP0THsR8U
 *
 * Usage:
 * @code
 * JwtPayload payload;
 * payload.userId = 123;
 * payload.username = "john_doe";
 * payload.email = "john@example.com";
 * payload.role = "user";
 * payload.issuedAt = getCurrentTimestamp();
 * payload.expiresAt = payload.issuedAt + 900; // 15 minutes
 *
 * std::string token = JwtUtils::generateToken(payload, "your-secret-key");
 * auto decoded = JwtUtils::verifyToken(token, "your-secret-key");
 * if (decoded.has_value()) {
 *     std::cout << "User ID: " << decoded->userId << std::endl;
 * }
 * @endcode
 */
class JwtUtils {
public:
    // ========================================================================
    // Token Generation & Verification
    // ========================================================================

    /**
     * @brief Generate JWT token
     * @param payload Token payload
     * @param secret Secret key for signing
     * @return JWT token string
     *
     * Throws: std::runtime_error if encoding fails
     */
    static std::string generateToken(const JwtPayload& payload,
                                    const std::string& secret);

    /**
     * @brief Verify and decode JWT token
     * @param token JWT token string
     * @param secret Secret key for verification
     * @return JwtPayload if valid, std::nullopt otherwise
     *
     * This function:
     * 1. Verifies the signature using the secret
     * 2. Decodes the payload
     * 3. Checks token expiration
     * 4. Returns the payload if all checks pass
     */
    static std::optional<JwtPayload> verifyToken(const std::string& token,
                                                 const std::string& secret);

    /**
     * @brief Decode JWT without verification (for debugging)
     * @param token JWT token string
     * @return JwtPayload if well-formed, std::nullopt otherwise
     *
     * WARNING: This does NOT verify the signature!
     * Use only for debugging purposes, never for authentication
     */
    static std::optional<JwtPayload> decodeToken(const std::string& token);

    // ========================================================================
    // Token Validation
    // ========================================================================

    /**
     * @brief Check if token is expired
     * @param payload JWT payload
     * @return true if token is expired
     */
    static bool isExpired(const JwtPayload& payload);

    /**
     * @brief Check if token is about to expire (within threshold)
     * @param payload JWT payload
     * @param thresholdSeconds Threshold in seconds (default: 300 = 5 minutes)
     * @return true if token will expire within threshold
     */
    static bool isExpiringSoon(const JwtPayload& payload, int thresholdSeconds = 300);

    /**
     * @brief Get token expiration time
     * @param token JWT token
     * @return Expiration timestamp, or 0 if invalid
     */
    static int64_t getExpiration(const std::string& token);

    /**
     * @brief Get time until token expires
     * @param payload JWT payload
     * @return Seconds until expiration, 0 if already expired
     */
    static int64_t getTimeUntilExpiration(const JwtPayload& payload);

    // ========================================================================
    // Token Parts
    // ========================================================================

    /**
     * @brief Extract header from token
     * @param token JWT token
     * @return Header JSON string, or empty string if invalid
     */
    static std::string getHeader(const std::string& token);

    /**
     * @brief Extract payload from token
     * @param token JWT token
     * @return Payload JSON string, or empty string if invalid
     */
    static std::string getPayload(const std::string& token);

    /**
     * @brief Extract signature from token
     * @param token JWT token
     * @return Signature string, or empty string if invalid
     */
    static std::string getSignature(const std::string& token);

    // ========================================================================
    // Token Information
    // ========================================================================

    /**
     * @brief Check if string is valid JWT format
     * @param token String to check
     * @return true if valid JWT format (header.payload.signature)
     */
    static bool isValidFormat(const std::string& token);

    /**
     * @brief Get algorithm from token header
     * @param token JWT token
     * @return Algorithm (e.g., "HS256"), or empty string if not found
     */
    static std::string getAlgorithm(const std::string& token);

    /**
     * @brief Get type from token header
     * @param token JWT token
     * @return Type (e.g., "JWT"), or empty string if not found
     */
    static std::string getType(const std::string& token);

private:
    // ========================================================================
    // Encoding/Decoding
    // ========================================================================

    /**
     * @brief Base64URL encode
     * @param data Data to encode
     * @return Base64URL encoded string
     *
     * Base64URL is a variant of Base64 that:
     * - Uses '-' instead of '+'
     * - Uses '_' instead of '/'
     * - Removes padding '='
     */
    static std::string base64UrlEncode(const std::vector<uint8_t>& data);

    /**
     * @brief Base64URL decode
     * @param data Base64URL encoded string
     * @return Decoded data
     *
     * Throws: std::runtime_error if decoding fails
     */
    static std::vector<uint8_t> base64UrlDecode(const std::string& data);

    /**
     * @brief String to bytes
     * @param str String to convert
     * @return Byte vector
     */
    static std::vector<uint8_t> stringToBytes(const std::string& str);

    /**
     * @brief Bytes to string
     * @param bytes Byte vector
     * @return String
     */
    static std::string bytesToString(const std::vector<uint8_t>& bytes);

    // ========================================================================
    // Cryptography
    // ========================================================================

    /**
     * @brief HMAC-SHA256 signature
     * @param data Data to sign
     * @param secret Secret key
     * @return Signature bytes
     *
     * Uses OpenSSL's HMAC implementation
     */
    static std::vector<uint8_t> hmacSha256(const std::string& data,
                                          const std::string& secret);

    /**
     * @brief SHA256 hash
     * @param data Data to hash
     * @return Hash bytes
     *
     * Uses OpenSSL's SHA256 implementation
     */
    static std::vector<uint8_t> sha256(const std::string& data);

    // ========================================================================
    // JSON Operations
    // ========================================================================

    /**
     * @brief Escape JSON string
     * @param str String to escape
     * @return Escaped string
     */
    static std::string escapeJsonString(const std::string& str);

    /**
     * @brief Unescape JSON string
     * @param str String to unescape
     * @return Unescaped string
     */
    static std::string unescapeJsonString(const std::string& str);

    /**
     * @brief Build JSON object from key-value pairs
     * @param pairs Key-value pairs
     * @return JSON string
     */
    static std::string buildJson(const std::map<std::string, std::string>& pairs);

    /**
     * @brief Parse JSON object to key-value pairs
     * @param json JSON string
     * @return Key-value pairs
     *
     * Note: Simple JSON parser, doesn't support nested objects
     */
    static std::map<std::string, std::string> parseJson(const std::string& json);

    // ========================================================================
    // Timestamp Utilities
    // ========================================================================

    /**
     * @brief Get current Unix timestamp
     * @return Current timestamp in seconds
     */
    static int64_t getCurrentTimestamp();

    /**
     * @brief Format timestamp as ISO 8601 string
     * @param timestamp Unix timestamp
     * @return ISO 8601 formatted string
     */
    static std::string formatTimestamp(int64_t timestamp);

    /**
     * @brief Parse ISO 8601 string to timestamp
     * @param isoString ISO 8601 formatted string
     * @return Unix timestamp
     */
    static int64_t parseTimestamp(const std::string& isoString);

    // ========================================================================
    // Constants
    // ========================================================================

    static const char* const HEADER_DEFAULT;
    static const char* const ALGORITHM_HS256;
    static const size_t SIGNATURE_SIZE = 32; // HMAC-SHA256 output size
};

// ============================================================================
// Inline Functions
// ============================================================================

inline bool JwtUtils::isExpired(const JwtPayload& payload) {
    return payload.expiresAt < getCurrentTimestamp();
}

inline bool JwtUtils::isExpiringSoon(const JwtPayload& payload, int thresholdSeconds) {
    int64_t timeLeft = payload.expiresAt - getCurrentTimestamp();
    return timeLeft > 0 && timeLeft <= thresholdSeconds;
}

inline int64_t JwtUtils::getTimeUntilExpiration(const JwtPayload& payload) {
    int64_t timeLeft = payload.expiresAt - getCurrentTimestamp();
    return timeLeft > 0 ? timeLeft : 0;
}

inline int64_t JwtUtils::getCurrentTimestamp() {
    return std::chrono::system_clock::now().time_since_epoch() /
           std::chrono::seconds(1);
}

} // namespace PaperCrawler
