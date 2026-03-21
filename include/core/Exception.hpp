#pragma once

#include <stdexcept>
#include <string>

namespace PaperCrawler {

/**
 * @brief Error codes for different types of exceptions
 */
enum class ErrorCode {
    NETWORK_ERROR,       ///< Network-related errors
    PARSE_ERROR,         ///< HTML parsing errors
    DATABASE_ERROR,      ///< Database operation errors
    CONFIG_ERROR,        ///< Configuration errors
    FILE_IO_ERROR,       ///< File input/output errors
    VALIDATION_ERROR     ///< Data validation errors
};

/**
 * @brief Base exception class for the PaperCrawler application
 *
 * Provides detailed error information including error codes and context
 */
class Exception : public std::runtime_error {
public:
    /**
     * @brief Construct exception with message and error code
     * @param message Error message
     * @param code Error code
     */
    explicit Exception(const std::string& message, ErrorCode code = ErrorCode::NETWORK_ERROR)
        : std::runtime_error(message), code_(code) {}

    /**
     * @brief Construct exception with message, error code, and context
     * @param message Error message
     * @param code Error code
     * @param context Additional context information
     */
    Exception(const std::string& message, ErrorCode code, const std::string& context)
        : std::runtime_error(message), code_(code), context_(context) {}

    /**
     * @brief Get the error code
     * @return Error code
     */
    ErrorCode code() const noexcept { return code_; }

    /**
     * @brief Get the context information
     * @return Context string
     */
    const std::string& context() const noexcept { return context_; }

    /**
     * @brief Get full error message with context
     * @return Complete error message
     */
    std::string fullMessage() const {
        if (context_.empty()) {
            return std::runtime_error::what();
        }
        return std::string(std::runtime_error::what()) + " [Context: " + context_ + "]";
    }

private:
    ErrorCode code_;
    std::string context_;
};

/**
 * @brief Network-related exceptions
 */
class NetworkException : public Exception {
public:
    explicit NetworkException(const std::string& message, const std::string& context = "")
        : Exception(message, ErrorCode::NETWORK_ERROR, context) {}
};

/**
 * @brief HTML parsing exceptions
 */
class ParseException : public Exception {
public:
    explicit ParseException(const std::string& message, const std::string& context = "")
        : Exception(message, ErrorCode::PARSE_ERROR, context) {}
};

/**
 * @brief Database operation exceptions
 */
class DatabaseException : public Exception {
public:
    explicit DatabaseException(const std::string& message, const std::string& context = "")
        : Exception(message, ErrorCode::DATABASE_ERROR, context) {}
};

/**
 * @brief Configuration-related exceptions
 */
class ConfigException : public Exception {
public:
    explicit ConfigException(const std::string& message, const std::string& context = "")
        : Exception(message, ErrorCode::CONFIG_ERROR, context) {}
};

/**
 * @brief File I/O exceptions
 */
class FileIOException : public Exception {
public:
    explicit FileIOException(const std::string& message, const std::string& context = "")
        : Exception(message, ErrorCode::FILE_IO_ERROR, context) {}
};

/**
 * @brief Data validation exceptions
 */
class ValidationException : public Exception {
public:
    explicit ValidationException(const std::string& message, const std::string& context = "")
        : Exception(message, ErrorCode::VALIDATION_ERROR, context) {}
};

} // namespace PaperCrawler
