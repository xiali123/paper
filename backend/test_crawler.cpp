/**
 * @file test_crawler.cpp
 * @brief 独立的爬虫测试工具 - 验证HTML解析逻辑
 *
 * 编译: g++ test_crawler.cpp -o test_crawler -std=c++17 -lcurl
 * 运行: ./test_crawler
 */

#include <iostream>
#include <string>
#include <regex>
#include <sstream>
#include <fstream>

// libcurl用于HTTP请求
#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <curl/curl.h>
#endif

/**
 * @brief 简单的HTTP GET请求
 */
std::string httpGet(const std::string& url) {
#ifdef _WIN32
    // Windows版本：使用WinINet（临时方案）
    HINTERNET hInternet = InternetOpenA("Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, INTERNET_FLAG_RELOAD, 0);

    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    std::string result;
    char buffer[4096];
    DWORD bytesRead;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return result;
#else
    // Linux版本：使用libcurl
    CURL* curl = curl_easy_init();
    std::string result;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* s) -> size_t {
            size_t newLength = size * nmemb;
            s->append((char*)contents, newLength);
            return newLength;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "HTTP request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    return result;
#endif
}

/**
 * @brief 测试DBLP爬虫
 */
void testDBLPCrawler(const std::string& query) {
    std::cout << "=== Testing DBLP Crawler ===" << std::endl;
    std::cout << "Query: " << query << std::endl << std::endl;

    // 构建DBLP URL
    std::string encodedQuery = query;
    for (auto& c : encodedQuery) {
        if (c == ' ') c = '+';
    }
    std::string dblpUrl = "https://dblp.uni-trier.de/search?q=" + encodedQuery;

    std::cout << "Fetching: " << dblpUrl << std::endl;

    // 获取HTML
    std::string html = httpGet(dblpUrl);

    if (html.empty()) {
        std::cerr << "Failed to fetch HTML" << std::endl;
        return;
    }

    std::cout << "Received " << html.length() << " bytes" << std::endl;

    // 解析结果总数 (Python第117行)
    std::regex countRegex(R"(found\s+([0-9,]+))");
    std::smatch countMatch;
    int totalResults = 0;
    if (std::regex_search(html, countMatch, countMatch)) {
        std::string countStr = countMatch[1].str();
        countStr.erase(std::remove(countStr.begin(), countStr.end(), ','), countStr.end());
        try {
            totalResults = std::stoi(countStr);
        } catch (...) {
            totalResults = 0;
        }
    }

    std::cout << "Total results: " << totalResults << std::endl;

    // 解析论文条目 (Python第110行)
    std::regex entryRegex(R"(<li\s+class="entry[^>]*>.*?</li>\s*<meta[^>]*>)");
    std::sregex_iterator it(html.begin(), html.end(), entryRegex);
    std::sregex_iterator end;

    int count = 0;
    for (; it != end && count < 5; ++it) {
        std::string entryHtml = it->str();

        std::cout << "\n--- Paper " << (count + 1) << " ---" << std::endl;

        // 提取标题
        std::regex titleRegex(R"(<span\s+class="title"[^>]*>([^<]+)</span>)");
        std::smatch titleMatch;
        if (std::regex_search(entryHtml, titleRegex, titleMatch)) {
            std::cout << "Title: " << titleMatch[1].str() << std::endl;
        }

        // 提取期刊
        std::regex venueRegex(R"(<span\s+itemprop="name"[^>]*>([^<]+)</span>)");
        std::smatch venueMatch;
        if (std::regex_search(entryHtml, venueRegex, venueMatch)) {
            std::cout << "Publication: " << venueMatch[1].str() << std::endl;
        }

        // 提取年份
        std::regex yearRegex(R"(<span\s+itemprop="datePublished"[^>]*>(\d{4})</span>)");
        std::smatch yearMatch;
        if (std::regex_search(entryHtml, yearRegex, yearMatch)) {
            std::cout << "Year: " << yearMatch[1].str() << std::endl;
        }

        count++;
    }

    std::cout << "\n=== DBLP Crawler Test Complete ===" << std::endl;
}

/**
 * @brief 测试CCF等级查询
 */
void testCCFRankQuerier(const std::string& venue) {
    std::cout << "\n=== Testing CCF Rank Querier ===" << std::endl;
    std::cout << "Venue: " << venue << std::endl << std::endl;

    // 构建查询URL (Python第248行)
    std::string encodedVenue = venue;
    for (auto& c : encodedVenue) {
        if (c == ' ') c = '+';
    }
    std::string ccfUrl = "https://www.myhuiban.com/search?SearchForm%5Bkey%5D=" + encodedVenue;

    std::cout << "Fetching: " << ccfUrl << std::endl;

    // 获取HTML
    std::string html = httpGet(ccfUrl);

    if (html.empty()) {
        std::cerr << "Failed to fetch HTML" << std::endl;
        return;
    }

    std::cout << "Received " << html.length() << " bytes" << std::endl;

    // 解析等级信息 (Python第272-273行)
    std::regex rowRegex(
        R"(<tr\s+class="[^"]*">\s*<td>\s*<span\s+class="badge\s+badge-warning">([^<]+)</span></td>\s*<td>[^<]*</td>\s*<td>[^<]*</td>\s*<td>([^<]*)</td>\s*<td><a[^>]*>([^<]+)</a>\s*</tr>)"
    );

    std::sregex_iterator it(html.begin(), html.end(), rowRegex);
    std::sregex_iterator end;

    int count = 0;
    std::string level = "T"; // 默认未知

    for (; it != end; ++it) {
        std::string rank = (*it)[1].str();
        std::string name1 = (*it)[2].str();
        std::string name2 = (*it)[3].str();

        std::cout << "Rank: " << rank << ", Name: " << name1 << " / " << name2 << std::endl;

        // 确定等级 (Python第294-303行逻辑)
        if (rank == "a" && (name1 == venue || name2 == venue)) {
            level = "A";
        }

        count++;
    }

    std::cout << "\nFinal CCF Level: " << level << std::endl;
    std::cout << "=== CCF Rank Querier Test Complete ===" << std::endl;
}

int main() {
    std::cout << "PaperCrawler - Crawler Testing Tool" << std::endl;
    std::cout << "=====================================" << std::endl;

#ifdef _WIN32
    // 初始化WinINet
    std::cout << "Initializing WinINet..." << std::endl;
    // InternetOpenA已经在第一个HTTP调用中自动初始化
#endif

    // 测试DBLP爬虫
    testDBLPCrawler("deep learning");

    // 测试CCF等级查询
    testCCFRankQuerier("CVPR");

    std::cout << "\n=====================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;

    return 0;
}
