#pragma once

#include "parser/HtmlParser.hpp"
#include "models/Journal.hpp"
#include <vector>
#include <string>
#include <map>

namespace PaperCrawler {

/**
 * @brief Parser for Huiban (会议榜) conference/journal rankings
 *
 * Huiban (www.myhuiban.com) provides CCF conference rankings
 */
class HuibanParser {
public:
    HuibanParser() = default;
    ~HuibanParser() = default;

    /**
     * @brief Parse journal/conference ranking information
     * @param html HTML content from huiban
     * @return Vector of ranking entries
     */
    std::vector<Journal> parseRankingInfo(const std::string& html);

    /**
     * @brief Extract journal full name
     * @param html HTML content
     * @return Full name or empty string
     */
    std::string extractFullName(const std::string& html);

    /**
     * @brief Extract journal ranking level
     * @param html HTML content
     * @param journalName Short journal name
     * @return Level ('a', 'b', 'c', 't' for unknown)
     */
    std::string extractLevel(const std::string& html, const std::string& journalName);

    /**
     * @brief Build huiban search URL
     * @param journalName Journal/conference name
     * @return Complete URL
     */
    static std::string buildSearchUrl(const std::string& journalName);

private:
    HtmlParser htmlParser_;

    /**
     * @brief Check if ranking entry matches journal name
     * @param entry Ranking entry data
     * @param journalName Journal name to match
     * @return True if matches
     */
    bool isMatchingJournal(const std::vector<std::string>& entry, const std::string& journalName);

    static const char* RANKING_PATTERN;
    static const char* FULL_NAME_PATTERN;
    static const char* LEVEL_PATTERN;
};

} // namespace PaperCrawler
