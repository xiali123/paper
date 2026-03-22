#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace PaperCrawler {

/**
 * @brief Password hashing utilities
 *
 * Uses PBKDF2 (Password-Based Key Derivation Function 2) with HMAC-SHA256
 * Implements salt generation and verification for secure password storage
 *
 * Security Features:
 * - PBKDF2 with 100,000 iterations (configurable)
 * - Unique salt per user
 * - HMAC-SHA256 as the PRF (Pseudo-Random Function)
 * - 32-byte (256-bit) derived keys
 *
 * Usage:
 * @code
 * std::string password = "MySecurePass123!";
 *
 * // Generate salt and hash
 * std::string salt;
 * std::string hash = PasswordHasher::hashPassword(password, salt);
 *
 * // Store salt and hash in database
 *
 * // Verify password
 * bool valid = PasswordHasher::verifyPassword(password, hash, salt);
 * @endcode
 *
 * Password Strength Requirements:
 * - Minimum 8 characters
 * - At least one uppercase letter
 * - At least one lowercase letter
 * - At least one digit
 * - At least one special character
 */
class PasswordHasher {
public:
    // ========================================================================
    // Password Hashing & Verification
    // ========================================================================

    /**
     * @brief Hash password with generated salt
     * @param password Plain text password
     * @param salt Generated salt (output parameter)
     * @return Password hash (hex-encoded)
     *
     * This method:
     * 1. Generates a cryptographically secure random salt
     * 2. Uses PBKDF2-HMAC-SHA256 to derive a key from password + salt
     * 3. Returns both the hash and the salt for storage
     *
     * @note Store BOTH the hash AND the salt in the database
     */
    static std::string hashPassword(const std::string& password, std::string& salt);

    /**
     * @brief Hash password with existing salt
     * @param password Plain text password
     * @param salt Existing salt (hex-encoded)
     * @return Password hash (hex-encoded)
     *
     * Use this when verifying a password against an existing hash
     */
    static std::string hashPassword(const std::string& password,
                                   const std::string& salt);

    /**
     * @brief Verify password against hash
     * @param password Plain text password
     * @param hash Stored hash (hex-encoded)
     * @param salt Salt used for hashing (hex-encoded)
     * @return true if password matches
     *
     * This method is timing-attack resistant
     */
    static bool verifyPassword(const std::string& password,
                              const std::string& hash,
                              const std::string& salt);

    // ========================================================================
    // Salt Generation
    // ========================================================================

    /**
     * @brief Generate cryptographically secure random salt
     * @param saltLength Length of salt in bytes (default: 32)
     * @return Hex-encoded salt string
     *
     * Uses cryptographically secure random number generator
     * Recommended: 32 bytes (256 bits) for good security
     */
    static std::string generateSalt(int saltLength = 32);

    // ========================================================================
    // Password Strength Validation
    // ========================================================================

    /**
     * @brief Validate password strength
     * @param password Password to validate
     * @return true if password meets requirements
     *
     * Requirements:
     * - Minimum 8 characters
     * - At least one uppercase letter (A-Z)
     * - At least one lowercase letter (a-z)
     * - At least one digit (0-9)
     * - At least one special character (!@#$%^&*...)
     *
     * @note You can customize requirements using the detailed version
     */
    static bool validateStrength(const std::string& password);

    /**
     * @brief Validate password with custom requirements
     * @param password Password to validate
     * @param minLength Minimum length
     * @param requireUppercase Require at least one uppercase letter
     * @param requireLowercase Require at least one lowercase letter
     * @param requireDigit Require at least one digit
     * @param requireSpecial Require at least one special character
     * @return true if password meets all requirements
     */
    static bool validateStrength(const std::string& password,
                                int minLength = 8,
                                bool requireUppercase = true,
                                bool requireLowercase = true,
                                bool requireDigit = true,
                                bool requireSpecial = true);

    /**
     * @brief Calculate password strength score
     * @param password Password to evaluate
     * @return Strength score from 0 (very weak) to 100 (very strong)
     *
     * Scoring criteria:
     * - Length: up to 40 points
     * - Character variety: up to 40 points
     * - Pattern avoidance: up to 20 points
     */
    static int calculateStrengthScore(const std::string& password);

    /**
     * @brief Get password strength description
     * @param password Password to evaluate
     * @return Description: "Very Weak", "Weak", "Fair", "Strong", "Very Strong"
     */
    static std::string getStrengthDescription(const std::string& password);

    // ========================================================================
    // Password Policy
    // ========================================================================

    /**
     * @brief Check if password is common/weak
     * @param password Password to check
     * @return true if password is in common passwords list
     *
     * Checks against a list of commonly used weak passwords
     * like "password123", "qwerty", etc.
     */
    static bool isCommonPassword(const std::string& password);

    /**
     * @brief Check if password contains user information
     * @param password Password to check
     * @param username Username (optional)
     * @param email Email (optional)
     * @return true if password contains user info
     *
     * Prevents users from including their username or email in password
     */
    static bool containsUserInfo(const std::string& password,
                                const std::string& username = "",
                                const std::string& email = "");

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set PBKDF2 iteration count
     * @param iterations Number of iterations
     *
     * Higher iterations = more secure but slower
     * Recommended: 100,000 or higher
     * OWASP recommends 310,000 for PBKDF2-HMAC-SHA256 as of 2023
     */
    static void setIterations(int iterations);

    /**
     * @brief Get current iteration count
     * @return Number of iterations
     */
    static int getIterations();

    /**
     * @brief Set derived key length
     * @param keyLength Length in bytes
     *
     * Recommended: 32 bytes (256 bits)
     */
    static void setKeyLength(int keyLength);

    /**
     * @brief Get current key length
     * @return Key length in bytes
     */
    static int getKeyLength();

private:
    // ========================================================================
    // PBKDF2 Implementation
    // ========================================================================

    /**
     * @brief PBKDF2 key derivation
     * @param password Password to derive from
     * @param salt Salt value
     * @param iterations Number of iterations
     * @param keyLength Desired key length in bytes
     * @return Derived key
     *
     * Implements PBKDF2-HMAC-SHA256 as specified in RFC 2898
     */
    static std::vector<uint8_t> pbkdf2(const std::string& password,
                                       const std::vector<uint8_t>& salt,
                                       int iterations,
                                       int keyLength);

    /**
     * @brief PBKDF2 PRF (Pseudo-Random Function)
     * Uses HMAC-SHA256
     */
    static std::vector<uint8_t> prf(const std::string& password,
                                    const std::vector<uint8_t>& data);

    // ========================================================================
    // Cryptographic Functions
    // ========================================================================

    /**
     * @brief HMAC-SHA256
     * @param key Key
     * @param data Data to authenticate
     * @return HMAC tag
     */
    static std::vector<uint8_t> hmacSha256(const std::string& key,
                                          const std::vector<uint8_t>& data);

    /**
     * @brief SHA256 hash
     * @param data Data to hash
     * @return Hash value
     */
    static std::vector<uint8_t> sha256(const std::string& data);

    /**
     * @brief SHA256 hash (bytes version)
     * @param data Data to hash
     * @return Hash value
     */
    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);

    // ========================================================================
    // Secure Random
    // ========================================================================

    /**
     * @brief Generate cryptographically secure random bytes
     * @param length Number of bytes to generate
     * @return Random bytes
     *
     * Uses OS-provided cryptographically secure random number generator
     */
    static std::vector<uint8_t> generateSecureRandom(int length);

    // ========================================================================
    // Encoding/Decoding
    // ========================================================================

    /**
     * @brief Convert bytes to hex string
     * @param bytes Bytes to convert
     * @return Hex-encoded string (lowercase)
     */
    static std::string bytesToHex(const std::vector<uint8_t>& bytes);

    /**
     * @brief Convert hex string to bytes
     * @param hex Hex-encoded string
     * @return Bytes
     *
     * @throws std::runtime_error if hex string is invalid
     */
    static std::vector<uint8_t> hexToBytes(const std::string& hex);

    /**
     * @brief Convert string to bytes
     * @param str String to convert
     * @return Bytes
     */
    static std::vector<uint8_t> stringToBytes(const std::string& str);

    /**
     * @brief Convert bytes to string
     * @param bytes Bytes to convert
     * @return String
     */
    static std::string bytesToString(const std::vector<uint8_t>& bytes);

    // ========================================================================
    // Comparison (Timing-Attack Resistant)
    // ========================================================================

    /**
     * @brief Constant-time comparison of two byte arrays
     * @param a First array
     * @param b Second array
     * @return true if arrays are equal
     *
     * Prevents timing attacks that could reveal password information
     */
    static bool constantTimeCompare(const std::vector<uint8_t>& a,
                                   const std::vector<uint8_t>& b);

    // ========================================================================
    // Password Analysis
    // ========================================================================

    /**
     * @brief Check for common patterns in password
     * @param password Password to check
     * @return true if common patterns found
     *
     * Checks for:
     * - Repeated characters (aaa, 111)
     * - Sequential characters (abc, 123)
     * - Keyboard patterns (qwerty, asdf)
     */
    static bool hasCommonPatterns(const std::string& password);

    /**
     * @brief Count character types in password
     * @param password Password to analyze
     * @return Map of character type to count
     *
     * Types: uppercase, lowercase, digits, specials
     */
    static std::map<std::string, int> countCharacterTypes(const std::string& password);

    // ========================================================================
    // Members
    // ========================================================================

    static int iterations_;
    static int keyLength_;

    // Default values
    static const int DEFAULT_ITERATIONS = 100000;
    static const int DEFAULT_KEY_LENGTH = 32;
    static const int MIN_ITERATIONS = 10000;
    static const int MAX_ITERATIONS = 10000000;
};

// ============================================================================
// Inline Functions
// ============================================================================

inline int PasswordHasher::getIterations() {
    return iterations_;
}

inline int PasswordHasher::getKeyLength() {
    return keyLength_;
}

} // namespace PaperCrawler
