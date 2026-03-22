#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
#include <cstdint>

namespace PaperCrawler {

/**
 * @brief Rate limiter for login attempts and API requests
 *
 * Implements sliding window rate limiting to prevent brute force attacks
 * Thread-safe for use in multi-threaded environments
 *
 * Usage:
 * @code
 * RateLimiter limiter;
 *
 * // Check if rate limited
 * if (limiter.isRateLimited("user@example.com", 5, 60000)) {
 *     std::cout << "Too many attempts. Try again later." << std::endl;
 * } else {
 *     // Process login attempt
 *     limiter.recordAttempt("user@example.com", true);
 * }
 * @endcode
 *
 * Sliding Window Algorithm:
 * - Tracks timestamps of recent attempts
 * - Counts attempts within the time window
 * - Automatically cleans up old timestamps
 *
 * Default Configuration:
 * - Max attempts: 5
 * - Time window: 60,000ms (1 minute)
 * - Cleanup interval: 300,000ms (5 minutes)
 */
class RateLimiter {
public:
    // ========================================================================
    // Constructor
    // ========================================================================

    /**
     * @brief Constructor
     * @param maxAttempts Maximum attempts allowed (default: 5)
     * @param windowMs Time window in milliseconds (default: 60000)
     * @param cleanupIntervalMs Cleanup interval in milliseconds (default: 300000)
     */
    explicit RateLimiter(int maxAttempts = 5,
                       int64_t windowMs = 60000,
                       int64_t cleanupIntervalMs = 300000);

    /**
     * @brief Destructor
     */
    ~RateLimiter() = default;

    // ========================================================================
    // Rate Limiting Operations
    // ========================================================================

    /**
     * @brief Check if identifier is rate limited
     * @param identifier Email, IP address, or other identifier
     * @param maxAttempts Maximum attempts allowed (uses default if 0)
     * @param windowMs Time window in milliseconds (uses default if 0)
     * @return true if rate limited
     *
     * This method:
     * 1. Cleans up old timestamps outside the window
     * 2. Counts recent attempts
     * 3. Returns true if count >= maxAttempts
     */
    bool isRateLimited(const std::string& identifier,
                      int maxAttempts = 0,
                      int64_t windowMs = 0);

    /**
     * @brief Record an attempt
     * @param identifier Email, IP address, or other identifier
     * @param success Whether the attempt was successful
     *
     * If success is true, the attempt is still recorded but may not
     * count towards rate limiting (depending on configuration)
     */
    void recordAttempt(const std::string& identifier, bool success = true);

    /**
     * @brief Record a failed attempt
     * @param identifier Email, IP address, or other identifier
     *
     * Convenience method for recording failed login attempts
     */
    void recordFailedAttempt(const std::string& identifier);

    /**
     * @brief Record a successful attempt
     * @param identifier Email, IP address, or other identifier
     *
     * Convenience method for recording successful attempts
     * May clear previous failed attempts depending on configuration
     */
    void recordSuccessfulAttempt(const std::string& identifier);

    // ========================================================================
    // Query Operations
    // ========================================================================

    /**
     * @brief Get number of attempts for identifier
     * @param identifier Email, IP address, or other identifier
     * @param windowMs Time window in milliseconds (uses default if 0)
     * @return Number of attempts in the time window
     */
    int getAttemptCount(const std::string& identifier, int64_t windowMs = 0);

    /**
     * @brief Get remaining attempts before rate limit
     * @param identifier Email, IP address, or other identifier
     * @param maxAttempts Maximum attempts (uses default if 0)
     * @param windowMs Time window in milliseconds (uses default if 0)
     * @return Remaining attempts (0 if rate limited)
     */
    int getRemainingAttempts(const std::string& identifier,
                            int maxAttempts = 0,
                            int64_t windowMs = 0);

    /**
     * @brief Get time until next attempt is allowed
     * @param identifier Email, IP address, or other identifier
     * @param windowMs Time window in milliseconds (uses default if 0)
     * @return Milliseconds until next attempt (0 if not rate limited)
     */
    int64_t getTimeUntilNextAttempt(const std::string& identifier,
                                   int64_t windowMs = 0);

    /**
     * @brief Check if identifier is currently blocked
     * @param identifier Email, IP address, or other identifier
     * @return true if blocked
     */
    bool isBlocked(const std::string& identifier);

    // ========================================================================
    // Management Operations
    // ========================================================================

    /**
     * @brief Clear attempts for identifier
     * @param identifier Email, IP address, or other identifier
     *
     * Useful for manual intervention or after successful login
     */
    void clearAttempts(const std::string& identifier);

    /**
     * @brief Clear all attempts
     *
     * Useful for testing or maintenance
     */
    void clearAll();

    /**
     * @brief Cleanup old timestamps for all identifiers
     *
     * Automatically called periodically, but can be called manually
     */
    void cleanup();

    /**
     * @brief Get number of tracked identifiers
     * @return Count of unique identifiers
     */
    size_t getTrackedCount() const;

    /**
     * @brief Get statistics about an identifier
     * @param identifier Email, IP address, or other identifier
     * @return Map of statistics
     *
     * Returns:
     * - "total_attempts": Total attempts ever recorded
     * - "failed_attempts": Failed attempts in current window
     * - "successful_attempts": Successful attempts in current window
     * - "is_blocked": Whether currently blocked
     * - "time_until_next": Ms until next attempt allowed
     */
    std::map<std::string, int64_t> getStatistics(const std::string& identifier);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set maximum attempts
     * @param maxAttempts Maximum attempts allowed
     */
    void setMaxAttempts(int maxAttempts);

    /**
     * @brief Get maximum attempts
     * @return Maximum attempts
     */
    int getMaxAttempts() const;

    /**
     * @brief Set time window
     * @param windowMs Time window in milliseconds
     */
    void setWindowMs(int64_t windowMs);

    /**
     * @brief Get time window
     * @return Time window in milliseconds
     */
    int64_t getWindowMs() const;

    /**
     * @brief Set cleanup interval
     * @param intervalMs Cleanup interval in milliseconds
     */
    void setCleanupInterval(int64_t intervalMs);

    /**
     * @brief Get cleanup interval
     * @return Cleanup interval in milliseconds
     */
    int64_t getCleanupInterval() const;

    /**
     * @brief Enable/disable counting successful attempts
     * @param count true to count successful attempts
     *
     * When disabled, successful attempts are recorded but don't count
     * towards rate limiting. This allows successful logins to reset
     * the counter behaviorally.
     */
    void setCountSuccessfulAttempts(bool count);

    /**
     * @brief Check if successful attempts are counted
     * @return true if counted
     */
    bool getCountSuccessfulAttempts() const;

    /**
     * @brief Enable/disable automatic cleanup
     * @param enable true to enable automatic cleanup
     *
     * When enabled, old timestamps are automatically cleaned up
     * periodically to prevent memory leaks
     */
    void setAutoCleanup(bool enable);

    /**
     * @brief Check if automatic cleanup is enabled
     * @return true if enabled
     */
    bool getAutoCleanup() const;

private:
    // ========================================================================
    // Internal Types
    // ========================================================================

    /**
     * @brief Attempt record for tracking timestamps
     */
    struct AttemptRecord {
        std::vector<int64_t> timestamps;  // Timestamps of attempts
        int64_t lastCleanupTime;           // Last time this record was cleaned
        int64_t totalAttempts;             // Total attempts ever (for stats)

        AttemptRecord() : lastCleanupTime(0), totalAttempts(0) {}
    };

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Get current timestamp in milliseconds
     * @return Current timestamp
     */
    int64_t getCurrentTime() const;

    /**
     * @brief Clean old timestamps for a specific identifier
     * @param record Attempt record to clean
     * @param windowMs Time window
     * @return Number of timestamps removed
     */
    int cleanOldAttempts(AttemptRecord& record, int64_t windowMs);

    /**
     * @brief Get or create attempt record for identifier
     * @param identifier Email, IP, etc.
     * @return Reference to attempt record
     */
    AttemptRecord& getOrCreateRecord(const std::string& identifier);

    /**
     * @brief Remove empty records to save memory
     */
    void removeEmptyRecords();

    /**
     * @brief Perform automatic cleanup if needed
     */
    void performAutoCleanup();

    // ========================================================================
    // Members
    // ========================================================================

    mutable std::mutex mutex_;  // Protects access to attempts_

    std::map<std::string, AttemptRecord> attempts_;  // Tracking data

    // Configuration
    int maxAttempts_;           // Maximum attempts allowed
    int64_t windowMs_;          // Time window in milliseconds
    int64_t cleanupIntervalMs_; // Cleanup interval
    bool countSuccessful_;      // Count successful attempts
    bool autoCleanup_;          // Automatic cleanup enabled

    int64_t lastGlobalCleanup_; // Last global cleanup time
};

// ============================================================================
// Inline Functions
// ============================================================================

inline void RateLimiter::recordFailedAttempt(const std::string& identifier) {
    recordAttempt(identifier, false);
}

inline void RateLimiter::recordSuccessfulAttempt(const std::string& identifier) {
    recordAttempt(identifier, true);
}

inline int RateLimiter::getMaxAttempts() const {
    return maxAttempts_;
}

inline int64_t RateLimiter::getWindowMs() const {
    return windowMs_;
}

inline int64_t RateLimiter::getCleanupInterval() const {
    return cleanupIntervalMs_;
}

inline bool RateLimiter::getCountSuccessfulAttempts() const {
    return countSuccessful_;
}

inline bool RateLimiter::getAutoCleanup() const {
    return autoCleanup_;
}

inline void RateLimiter::setCountSuccessfulAttempts(bool count) {
    std::lock_guard<std::mutex> lock(mutex_);
    countSuccessful_ = count;
}

inline void RateLimiter::setAutoCleanup(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    autoCleanup_ = enable;
}

inline int64_t RateLimiter::getCurrentTime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

} // namespace PaperCrawler
