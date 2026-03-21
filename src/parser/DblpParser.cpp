#include "parser/DblpParser.hpp"
#include <regex>
#include <sstream>
#include <algorithm>

namespace PaperCrawler {

// Regex patterns for DBLP
const char* DblpParser::PAPER_ENTRY_PATTERN =
    R"(<li\s+class="entry.*?</li>)";

const char* DblpParser::TITLE_PATTERN =
    R"(<span\s+class="title"[^>]*>(.*?)</span>)";

const char* DblpParser::JOURNAL_PATTERN =
    R"(<span\s+itemprop="isPartOf"[^>]*>.*?<span\s+itemprop="name">(.*?)</span>)";

const char* DblpParser::YEAR_PATTERN =
    R"(<span\s+itemprop="datePublished">(\d{4})</span>)";

const char* DblpParser::DOI_PATTERN =
    R"(<li\s+class="ee"[^>]*>.*?<a\s+href="([^"]+)")";

const char* DblpParser::TOTAL_COUNT_PATTERN =
    R"(<p[^>]*>found\s+(\d+))";

std::vector<Paper> DblpParser::parseSearchResults(const std::string& html) {
    std::vector<Paper> papers;

    // Extract all paper entries
    std::vector<std::string> entries = htmlParser_.extractAll(html, PAPER_ENTRY_PATTERN);

    for (const auto& entry : entries) {
        try {
            Paper paper = parsePaperEntry(entry);
            if (!paper.getTitle().empty()) {
                papers.push_back(paper);
            }
        } catch (const ParseException& e) {
            // Skip invalid entries
            LOG_WARN("Failed to parse paper entry: {}", e.what());
        }
    }

    return papers;
}

int DblpParser::extractTotalCount(const std::string& html) {
    std::string countStr = htmlParser_.extract(html, TOTAL_COUNT_PATTERN);

    if (!countStr.empty()) {
        // Remove commas from number
        countStr.erase(std::remove(countStr.begin(), countStr.end(), ','), countStr.end());

        try {
            return std::stoi(countStr);
        } catch (...) {
            return 0;
        }
    }

    return 0;
}

Paper DblpParser::parsePaperEntry(const std::string& entryHtml) {
    Paper paper;

    // Extract title
    std::string title = htmlParser_.extract(entryHtml, TITLE_PATTERN);
    paper.setTitle(htmlParser_.cleanText(title));

    // Extract journal/conference name
    std::string journal = htmlParser_.extract(entryHtml, JOURNAL_PATTERN);
    paper.setJournalShort(htmlParser_.cleanText(journal));

    // Extract publication year
    std::string year = htmlParser_.extract(entryHtml, YEAR_PATTERN);
    paper.setYear(year);

    // Extract DOI URL
    std::string doiUrl = htmlParser_.extract(entryHtml, DOI_PATTERN);
    paper.setDoiUrl(doiUrl);

    // Extract journal URL (from the journal link)
    std::string journalUrlPattern = R"(<a\s+href="([^"]*)"[^>]*itemprop="isPartOf")";
    std::string journalUrl = htmlParser_.extract(entryHtml, journalUrlPattern);
    paper.setJournalUrl(journalUrl);

    return paper;
}

std::string DblpParser::buildSearchUrl(const std::string& keyword, int start, int count) {
    if (start == 0) {
        return "https://dblp.uni-trier.de/search?q=" + keyword;
    } else {
        return "https://dblp.uni-trier.de/search/publ/inc?q=" + keyword +
               "&s=ydvspc&h=" + std::to_string(count) + "&b=" + std::to_string(start);
    }
}

} // namespace PaperCrawler
