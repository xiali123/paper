#include "parser/HuibanParser.hpp"
#include <regex>

namespace PaperCrawler {

const char* HuibanParser::RANKING_PATTERN =
    R"(<span\s+class="badge\s+badge-warning">(.+?)</span>.*?<td><a[^>]*>(.+?)</a>)";

const char* HuibanParser::FULL_NAME_PATTERN =
    R"(<span\s+itemprop="name">(.*?)</span>)";

const char* HuibanParser::LEVEL_PATTERN =
    R"(<span\s+class="badge\s+badge-warning">([abcABC])</span>)";

std::vector<Journal> HuibanParser::parseRankingInfo(const std::string& html) {
    std::vector<Journal> journals;

    std::vector<std::string> matches = htmlParser_.extractAll(html, RANKING_PATTERN);

    for (const auto& match : matches) {
        // Extract level and name from match
        std::string level = htmlParser_.extract(match, LEVEL_PATTERN);
        std::string name = htmlParser_.extract(match, R"(<a[^>]*>(.+?)</a>)");

        if (!name.empty()) {
            Journal journal;
            journal.setName(htmlParser_.cleanText(name));
            journal.setLevel(htmlParser_.cleanText(level));
            journal.setFLevel(htmlParser_.cleanText(level));
            journals.push_back(journal);
        }
    }

    return journals;
}

std::string HuibanParser::extractFullName(const std::string& html) {
    std::string name = htmlParser_.extract(html, FULL_NAME_PATTERN);
    return htmlParser_.cleanText(name);
}

std::string HuibanParser::extractLevel(const std::string& html, const std::string& journalName) {
    std::vector<std::string> entries = htmlParser_.extractAll(html, RANKING_PATTERN);

    for (const auto& entry : entries) {
        if (isMatchingJournal({entry}, journalName)) {
            std::string level = htmlParser_.extract(entry, LEVEL_PATTERN);
            return htmlParser_.cleanText(level);
        }
    }

    return "t";  // Unknown
}

std::string HuibanParser::buildSearchUrl(const std::string& journalName) {
    // Convert journal name to URL format (replace spaces with +)
    std::string formattedName = journalName;

    // Replace spaces with +
    size_t pos = 0;
    while ((pos = formattedName.find(' ', pos)) != std::string::npos) {
        formattedName.replace(pos, 1, "+");
        pos += 1;
    }

    return "https://www.myhuiban.com/search?SearchForm%5Bkey%5D=" + formattedName;
}

bool HuibanParser::isMatchingJournal(const std::vector<std::string>& entry, const std::string& journalName) {
    if (entry.empty()) return false;

    // Extract name from entry
    std::string name = htmlParser_.extract(entry[0], R"(<a[^>]*>(.+?)</a>)");
    name = htmlParser_.cleanText(name);

    // Case-insensitive comparison
    if (name.length() < journalName.length()) {
        return std::equal(name.begin(), name.end(), journalName.begin(),
                         [](char a, char b) { return tolower(a) == tolower(b); });
    }

    return false;
}

} // namespace PaperCrawler
