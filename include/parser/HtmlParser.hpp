#pragma once

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include "core/Exception.hpp"

// Forward declaration for Gumbo
typedef struct GumboOutput GumboOutput;

namespace PaperCrawler {

/**
 * @brief HTML Parser class using Gumbo Parser
 *
 * Provides high-level HTML parsing utilities
 * RAII-compliant resource management
 */
class HtmlParser {
public:
    HtmlParser() = default;
    ~HtmlParser() = default;

    /**
     * @brief Extract text matching regex pattern from HTML
     * @param html HTML content
     * @param pattern Regex pattern (with ECMAScript syntax)
     * @return First match or empty string if not found
     */
    std::string extract(const std::string& html, const std::string& pattern);

    /**
     * @brief Extract all matches of regex pattern from HTML
     * @param html HTML content
     * @param pattern Regex pattern
     * @return Vector of all matches
     */
    std::vector<std::string> extractAll(const std::string& html, const std::string& pattern);

    /**
     * @brief Clean and normalize text
     * @param text Text to clean
     * @return Cleaned text
     */
    std::string cleanText(const std::string& text);

    /**
     * @brief Extract text from HTML (remove all tags)
     * @param html HTML content
     * @return Plain text
     */
    std::string stripTags(const std::string& html);

    /**
     * @brief Extract attribute value from HTML elements
     * @param html HTML content
     * @param tag Tag name to search
     * @param attr Attribute name
     * @return Attribute value or empty string
     */
    std::string extractAttr(const std::string& html, const std::string& tag, const std::string& attr);

    /**
     * @brief Extract text content from specific element
     * @param html HTML content
     * @param selector CSS-like selector (simplified)
     * @return Element text content
     */
    std::string extractElementText(const std::string& html, const std::string& selector);

private:
    /**
     * @brief Parse HTML string into Gumbo tree
     * @param html HTML content
     * @return Gumbo output pointer (RAII managed)
     */
    std::unique_ptr<GumboOutput, void(*)(GumboOutput*)> parse(const std::string& html);

    std::regex::flag_type regexFlags_{std::regex::ECMAScript | std::regex::icase | std::regex::optimize};
};

/**
 * @brief RAII wrapper for Gumbo parser initialization
 */
class GumboInitializer {
public:
    GumboInitializer();
    ~GumboInitializer();
};

} // namespace PaperCrawler
