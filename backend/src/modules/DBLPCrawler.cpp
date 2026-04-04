/**
 * @file DBLPCrawler.cpp
 * @brief DBLP爬虫实现 - 从DBLP搜索计算机科学论文
 *
 * 参考 Python 实现逻辑：
 * - URL: https://dblp.uni-trier.de/search?q={keyword}
 * - 解析HTML获取论文信息
 * - 分页爬取
 */

#include "modules/CrawlerModule.hpp"
#include <spdlog/spdlog.h>
#include <regex>
#include <sstream>
#include <algorithm>
#include <optional>

namespace PaperCrawler::Modules {

// ============================================================================
// DBLP Crawler Implementation
// ============================================================================

DBLPCrawler::DBLPCrawler(const CrawlerSource& source)
    : ICrawler() {
    source_ = source;
    httpClient_ = std::make_shared<Network::HttpClient>();
    httpClient_->setTimeout(15);
    httpClient_->setDefaultHeader("User-Agent", "PaperCrawler/1.0");
}

std::string DBLPCrawler::buildSearchUrl(
    const std::string& query,
    int page,
    int batchSize
) {
    // DBLP搜索URL格式
    // 首页: https://dblp.uni-trier.de/search?q={keyword}
    // 分页: https://dblp.uni-trier.de/search/publ/inc?q={keyword}&s=ydvspc&h=30&b={page*30}

    std::ostringstream url;
    std::string encodedQuery = query;

    // 替换空格为+号
    std::replace(encodedQuery.begin(), encodedQuery.end(), ' ', '+');

    if (page == 0) {
        // 首页请求
        url << source_.baseUrl << "/search?q=" << encodedQuery;
    } else {
        // 分页请求 (对应Python第123行)
        url << source_.baseUrl << "/search/publ/inc?q=" << encodedQuery
            << "&s=ydvspc&h=" << batchSize
            << "&b=" << (page * batchSize);
    }

    return url.str();
}

int DBLPCrawler::getTotalResults(const std::string& html) {
    // 对应Python第117行正则: found ([0-9]+,*[0-9]*)
    std::regex countRegex(R"(found\s+([0-9,]+))");
    std::smatch match;

    if (std::regex_search(html, match, countRegex)) {
        std::string countStr = match[1].str();
        // 移除逗号
        countStr.erase(std::remove(countStr.begin(), countStr.end(), ','), countStr.end());
        try {
            return std::stoi(countStr);
        } catch (const std::exception& e) {
            spdlog::error("[DBLP] Failed to parse total count: {}", e.what());
        }
    }

    return 0;
}

CrawledPaper DBLPCrawler::parseEntry(const std::string& entryHtml) {
    // 对应Python第145行的复杂正则表达式
    // 提取: DOI链接、标题、期刊URL、期刊名称、年份

    CrawledPaper paper;
    paper.source = "DBLP";

    try {
        // 提取标题
        std::regex titleRegex(R"(<span\s+class="title"[^>]*>(.*?)</span>)");
        std::smatch titleMatch;
        if (std::regex_search(entryHtml, titleMatch, titleRegex)) {
            paper.title = titleMatch[1].str();
        }

        // 提取DOI/EE链接
        std::regex eeRegex(R"(<li\s+class="ee"[^>]*>.*?<a\s+href="([^"]+)")");
        std::smatch eeMatch;
        if (std::regex_search(entryHtml, eeMatch, eeRegex)) {
            paper.url = eeMatch[1].str();
        }

        // 提取期刊/会议名称和URL
        std::regex venueRegex(R"(<a\s+href="([^"]+)">.*?<span\s+itemprop="isPartOf"[^>]*>.*?<span\s+itemprop="name"[^>]*>([^<]+)</span>)");
        std::smatch venueMatch;
        if (std::regex_search(entryHtml, venueMatch, venueRegex)) {
            paper.publication = venueMatch[2].str();
            // 可以保存venue URL用于后续查询等级
        }

        // 提取年份
        std::regex yearRegex(R"(<span\s+itemprop="datePublished"[^>]*>(\d{4})</span>)");
        std::smatch yearMatch;
        if (std::regex_search(entryHtml, yearMatch, yearRegex)) {
            try {
                paper.year = std::stoi(yearMatch[1].str());
            } catch (...) {
                paper.year = 0;
            }
        }

        // 提取作者 (简化版本)
        std::regex authorRegex(R"(<span\s+itemprop="name"[^>]*>([^<]+)</span>)");
        std::sregex_iterator authorIt(entryHtml.begin(), entryHtml.end(), authorRegex);
        std::sregex_iterator authorEnd;
        std::vector<std::string> authors;
        for (; authorIt != authorEnd; ++authorIt) {
            authors.push_back((*authorIt)[1].str());
        }
        if (!authors.empty()) {
            std::ostringstream authorStream;
            for (size_t i = 0; i < authors.size(); ++i) {
                if (i > 0) authorStream << ", ";
                authorStream << authors[i];
            }
            paper.authors = authorStream.str();
        }

    } catch (const std::exception& e) {
        spdlog::error("[DBLP] Error parsing entry: {}", e.what());
    }

    return paper;
}

std::vector<CrawledPaper> DBLPCrawler::parseDBLPHtml(const std::string& html) {
    std::vector<CrawledPaper> papers;

    try {
        // 对应Python第110行: 提取每个论文条目
        std::regex entryRegex(R"(<li\s+class="entry[^>]*>.*?</li>\s*<meta[^>]*>)");
        std::sregex_iterator it(html.begin(), html.end(), entryRegex);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            std::string entryHtml = it->str();
            CrawledPaper paper = parseEntry(entryHtml);

            if (!paper.title.empty()) {
                papers.push_back(paper);
            }
        }

        spdlog::info("[DBLP] Parsed {} papers from HTML", papers.size());

    } catch (const std::exception& e) {
        spdlog::error("[DBLP] Error parsing HTML: {}", e.what());
    }

    return papers;
}

std::vector<CrawledPaper> DBLPCrawler::fetchPapers(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    spdlog::info("[DBLP] Searching for: {}", query);

    std::vector<CrawledPaper> allPapers;

    try {
        // 获取参数
        int limit = 30; // 默认每页30条
        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        int maxPages = 10; // 限制最大页数防止无限爬取
        auto maxPagesIt = params.find("max_pages");
        if (maxPagesIt != params.end()) {
            maxPages = std::stoi(maxPagesIt->second);
        }

        // 首次请求 - 获取总结果数
        std::string firstUrl = buildSearchUrl(query, 0, limit);
        auto httpResponse = httpClient_->get(firstUrl);

        if (!httpResponse.isSuccess()) {
            spdlog::error("[DBLP] Request failed: {}", httpResponse.errorMessage);
            return allPapers;
        }

        int totalResults = getTotalResults(httpResponse.body);
        spdlog::info("[DBLP] Total results found: {}", totalResults);

        // 解析首页结果
        std::regex bodyRegex(R"(<div\s+class="body\s+hide-body"[^>]*>(.*?)</div>\s*<div\s+id="frontpage"\s+hidden>)");
        std::smatch bodyMatch;
        if (std::regex_search(httpResponse.body, bodyMatch, bodyRegex)) {
            std::string resultsHtml = bodyMatch[1].str();

            // 提取论文列表部分
            std::regex listRegex(R"(<ul\s+class="publ-list"[^>]*>(.*?)</ul>)");
            std::smatch listMatch;
            if (std::regex_search(resultsHtml, listMatch, listRegex)) {
                std::string papersHtml = listMatch[1].str();
                std::vector<CrawledPaper> firstPapers = parseDBLPHtml(papersHtml);
                allPapers.insert(allPapers.end(), firstPapers.begin(), firstPapers.end());
            }
        }

        // 分页爬取 (对应Python第185-190行)
        if (totalResults > limit) {
            int totalPages = std::min({totalResults / limit + 1, maxPages});

            for (int page = 1; page < totalPages; ++page) {
                spdlog::info("[DBLP] Fetching page {}/{}", page + 1, totalPages);

                std::string pageUrl = buildSearchUrl(query, page, limit);
                auto pageResponse = httpClient_->get(pageUrl);

                if (pageResponse.isSuccess()) {
                    std::vector<CrawledPaper> pagePapers = parseDBLPHtml(pageResponse.body);
                    allPapers.insert(allPapers.end(), pagePapers.begin(), pagePapers.end());

                    // 每10页休眠1秒 (对应Python第189行)
                    if (page % 10 == 9) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                } else {
                    spdlog::warn("[DBLP] Failed to fetch page {}: {}", page, pageResponse.errorMessage);
                }
            }
        }

        spdlog::info("[DBLP] Total papers fetched: {}", allPapers.size());

    } catch (const std::exception& e) {
        spdlog::error("[DBLP] Error in fetchPapers: {}", e.what());
    }

    return allPapers;
}

std::optional<CrawledPaper> DBLPCrawler::fetchPaper(const std::string& id) {
    spdlog::warn("[DBLP] fetchPaper by ID not implemented");
    return std::nullopt;
}

bool DBLPCrawler::checkAvailability() {
    try {
        auto response = httpClient_->get(source_.baseUrl);
        return response.isSuccess();
    } catch (...) {
        return false;
    }
}

std::map<std::string, std::string> DBLPCrawler::fetchVenueInfo(const std::string& venueName) {
    // 对应Python第231行 - 从期刊URL获取完整信息
    std::map<std::string, std::string> info;

    try {
        std::string url = source_.baseUrl + "/" + venueName;
        auto response = httpClient_->get(url);

        if (response.isSuccess()) {
            // 提取期刊全称
            std::regex nameRegex(R"(<span\s+itemprop="name"[^>]*>(.*?)</span>)");
            std::smatch match;
            if (std::regex_search(response.body, match, nameRegex)) {
                info["fullname"] = match[1].str();
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[DBLP] Error fetching venue info: {}", e.what());
    }

    return info;
}

// ============================================================================
// CCF Rank Querier Implementation
// ============================================================================

CCFRankQuerier::CCFRankQuerier(std::shared_ptr<Network::HttpClient> client)
    : httpClient_(client) {
    httpClient_->setTimeout(10);
}

std::string CCFRankQuerier::buildQueryUrl(const std::string& venueName) {
    // 对应Python第248行
    // https://www.myhuiban.com/search?SearchForm%5Bkey%5D={venueName}

    std::ostringstream url;
    url << "https://www.myhuiban.com/search?SearchForm%5Bkey%5D=";

    // 替换空格为+
    std::string encodedName = venueName;
    std::replace(encodedName.begin(), encodedName.end(), ' ', '+');
    url << encodedName;

    return url.str();
}

std::vector<std::vector<std::string>> CCFRankQuerier::parseRankHtml(const std::string& html) {
    std::vector<std::vector<std::string>> results;

    try {
        // 对应Python第271-273行
        // 提取tbody中的内容，然后提取每行的等级、简称、全称

        std::regex rowRegex(
            R"(<tr\s+class="[^"]*">\s*<td>\s*<span\s+class="badge\s+badge-warning">([^<]+)</span></td>\s*<td>[^<]*</td>\s*<td>[^<]*</td>\s*<td>([^<]*)</td>\s*<td><a[^>]*>([^<]+)</a>\s*</tr>)"
        );

        std::sregex_iterator it(html.begin(), html.end(), rowRegex);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            std::vector<std::string> row;
            row.push_back((*it)[1].str()); // 等级
            row.push_back((*it)[2].str()); // 简称或全称
            row.push_back((*it)[3].str()); // 另一个名称
            results.push_back(row);
        }

        spdlog::debug("[CCF] Parsed {} rank entries", results.size());

    } catch (const std::exception& e) {
        spdlog::error("[CCF] Error parsing rank HTML: {}", e.what());
    }

    return results;
}

std::string CCFRankQuerier::determineLevel(
    const std::vector<std::vector<std::string>>& rankData,
    const std::string& venueName
) {
    // 对应Python第294-303行的逻辑
    std::string level = "t"; // 默认未知
    std::string flevel = "t";

    for (const auto& row : rankData) {
        if (row.size() == 3) {
            // 检查是否匹配期刊名称
            if (row[0] == "a" && (row[1] == venueName || row[2] == venueName)) {
                level = "a";
                break;
            }
        } else {
            // 如果有A类，直接设为A
            if (row[0] == "a") {
                flevel = "a";
                level = "a";
                break;
            }
        }
    }

    return level;
}

std::optional<VenueRankInfo> CCFRankQuerier::queryVenueRank(const std::string& venueName) {
    spdlog::info("[CCF] Querying rank for: {}", venueName);

    try {
        std::string url = buildQueryUrl(venueName);
        auto response = httpClient_->get(url);

        if (!response.isSuccess()) {
            spdlog::warn("[CCF] Failed to query venue: {}", response.errorMessage);
            return std::nullopt;
        }

        auto rankData = parseRankHtml(response.body);
        if (rankData.empty()) {
            spdlog::warn("[CCF] No rank data found for: {}", venueName);
            return std::nullopt;
        }

        VenueRankInfo info;
        info.name = venueName;
        info.level = determineLevel(rankData, venueName);

        // 提取全称（如果有的话）
        for (const auto& row : rankData) {
            if (row.size() >= 3 && !row[2].empty()) {
                info.fullname = row[2];
                break;
            }
        }

        info.url = url;
        info.info = "Queried from myhuiban.com";

        spdlog::info("[CCF] Found rank: {} -> Level: {}", venueName, info.level);

        return info;

    } catch (const std::exception& e) {
        spdlog::error("[CCF] Error querying venue rank: {}", e.what());
        return std::nullopt;
    }
}

std::map<std::string, VenueRankInfo> CCFRankQuerier::queryVenueRanks(
    const std::vector<std::string>& venueNames
) {
    std::map<std::string, VenueRankInfo> results;

    for (const auto& name : venueNames) {
        auto info = queryVenueRank(name);
        if (info.has_value()) {
            results[name] = info.value();
        }

        // 避免请求过于频繁
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return results;
}

} // namespace PaperCrawler::Modules
