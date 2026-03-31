/**
 * @file test_crawler.cpp
 * @brief 简化的爬虫测试工具
 */

#include <iostream>
#include <string>
#include <regex>
#include <sstream>

std::string httpGetSimple(const std::string& url) {
    // 临时使用：调用系统的curl命令
    std::ostringstream command;
    command << "curl -s \"" << url << "\"";

    FILE* pipe = _popen(command.str().c_str(), "r");
    if (!pipe) return "";

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

void testDBLP(const std::string& query) {
    std::cout << "=== DBLP Crawler Test ===" << std::endl;
    std::cout << "Query: " << query << std::endl;

    std::string url = "https://dblp.uni-trier.de/search?q=" + query;
    std::cout << "Fetching: " << url << std::endl;

    std::string html = httpGetSimple(url);

    if (html.empty()) {
        std::cerr << "Failed to fetch HTML" << std::endl;
        return;
    }

    std::cout << "Received " << html.length() << " bytes" << std::endl;

    // 解析论文条目
    std::regex entryRegex(R"(<li\s+class="entry[^>]*>.*?</li>)");
    std::sregex_iterator it(html.begin(), html.end(), entryRegex);
    std::sregex_iterator end;

    int count = 0;
    for (; it != end && count < 3; ++it) {
        std::string entry = it->str();

        // 简化提取：只找标题
        std::regex titleRegex(R"(<span\s+class="title"[^>]*>([^<]+)</span>)");
        std::smatch match;
        if (std::regex_search(entry, match, titleRegex)) {
            std::cout << "Paper " << (count+1) << ": " << match[1].str() << std::endl;
            count++;
        }
    }
}

void testCCF(const std::string& venue) {
    std::cout << "\n=== CCF Rank Test ===" << std::endl;
    std::cout << "Venue: " << venue << std::endl;

    std::string url = "https://www.myhuiban.com/search?SearchForm%5Bkey%5D=" + venue;
    std::cout << "Fetching: " << url << std::endl;

    std::string html = httpGetSimple(url);

    if (html.empty()) {
        std::cerr << "Failed to fetch HTML" << std::endl;
        return;
    }

    std::cout << "Received " << html.length() << " bytes" << std::endl;

    // 查找CCF等级
    std::regex badgeRegex(R"(<span\s+class="badge\s+badge-warning">([^<]+)</span>)");
    std::sregex_iterator it(html.begin(), html.end(), badgeRegex);
    std::sregex_iterator end;

    std::vector<std::string> ranks;
    for (; it != end; ++it) {
        ranks.push_back((*it)[1].str());
    }

    std::cout << "Found " << ranks.size() << " rank entries" << std::endl;
    if (!ranks.empty()) {
        std::cout << "CCF Level: " << ranks[0] << std::endl;
    }
}

int main() {
    std::cout << "PaperCrawler - Simplified Crawler Test" << std::endl;
    std::cout << "========================================" << std::endl;

    testDBLP("deep learning");
    testCCF("CVPR");

    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests completed!" << std::endl;
    return 0;
}
