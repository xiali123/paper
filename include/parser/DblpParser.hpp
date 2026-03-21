#pragma once

#include "parser/HtmlParser.hpp"
#include "models/Paper.hpp"
#include <vector>
#include <string>

namespace PaperCrawler {

/**
 * @brief Parser for DBLP search results
 *
 * DBLP (Database Systems and Logic Programming) is a bibliographic database
 * This parser extracts paper information from DBLP search results
 */
class DblpParser {
public:
    DblpParser() = default;
    ~DblpParser() = default;

    /**
     * @brief Parse search results from DBLP
     * @param html HTML content from DBLP search
     * @return Vector of parsed papers
     */
    std::vector<Paper> parseSearchResults(const std::string& html);

    /**
     * @brief Extract total number of results
     * @param html HTML content from DBLP search
     * @return Total result count
     */
    int extractTotalCount(const std::string& html);

    /**
     * @brief Parse individual paper entry from HTML snippet
     * @param entryHtml HTML snippet for one paper
     * @return Parsed paper object
     */
    Paper parsePaperEntry(const std::string& entryHtml);

    /**
     * @brief Build DBLP search URL
     * @param keyword Search keyword
     * @param start Start index for pagination
     * @param count Number of results per page
     * @return Complete URL
     */
    static std::string buildSearchUrl(const std::string& keyword, int start = 0, int count = 30);

private:
    HtmlParser htmlParser_;

    // Regex patterns for DBLP
    static const char* PAPER_ENTRY_PATTERN;
    static const char* TITLE_PATTERN;
    static const char* JOURNAL_PATTERN;
    static const char* YEAR_PATTERN;
    static const char* DOI_PATTERN;
    static const char* TOTAL_COUNT_PATTERN;
};

} // namespace PaperCrawler
