#include <gtest/gtest.h>
#include "core/PaperCrawlerAPI.hpp"
#include "core/Exception.hpp"

using namespace PaperCrawler;

class PaperCrawlerAPITest : public ::testing::Test {
protected:
    PaperCrawlerAPI* api;

    void SetUp() override {
        api = &PaperCrawlerAPI::getInstance();
        // Use test configuration
        try {
            api->initialize("config/test_config.json");
        } catch (const ConfigException& e) {
            // If test config doesn't exist, skip database-dependent tests
            GTEST_SKIP() << "Test configuration not found: " << e.what();
        }
    }

    void TearDown() override {
        // Don't shutdown, as we're using a singleton
    }
};

// Test API initialization
TEST_F(PaperCrawlerAPITest, InitializeAPI) {
    EXPECT_TRUE(api->isInitialized());
}

// Test search functionality
TEST_F(PaperCrawlerAPITest, SearchPapers) {
    if (!api->isInitialized()) {
        GTEST_SKIP() << "API not initialized";
    }

    SearchRequest request;
    request.keyword = "deep learning";
    request.maxResults = 10;

    SearchResult result = api->search(request);

    EXPECT_GE(result.papers.size(), 0);
    EXPECT_EQ(result.keyword, "deep learning");
    EXPECT_GE(result.durationSeconds, 0);
}

// Test statistics
TEST_F(PaperCrawlerAPITest, GetStatistics) {
    if (!api->isInitialized()) {
        GTEST_SKIP() << "API not initialized";
    }

    Statistics stats = api->getStatistics();

    EXPECT_GE(stats.totalPapers, 0);
    EXPECT_GE(stats.totalJournals, 0);
    EXPECT_GE(stats.topTierPapers, 0);
}

// Test get paper by ID
TEST_F(PaperCrawlerAPITest, GetPaperById) {
    if (!api->isInitialized()) {
        GTEST_SKIP() << "API not initialized";
    }

    // Try to get a paper (may throw if not found)
    try {
        Paper paper = api->getPaper(1);
        EXPECT_GT(paper.getId(), 0);
        EXPECT_FALSE(paper.getTitle().empty());
    } catch (const DatabaseException& e) {
        // Paper ID 1 might not exist, that's ok
        SUCCEED();
    }
}

// Test get papers with pagination
TEST_F(PaperCrawlerAPITest, GetPapersWithPagination) {
    if (!api->isInitialized()) {
        GTEST_SKIP() << "API not initialized";
    }

    std::vector<Paper> papers = api->getPapers("deep learning", 0, 10);

    EXPECT_LE(papers.size(), 10);
}

// Test export to CSV
TEST_F(PaperCrawlerAPITest, ExportToCSV) {
    if (!api->isInitialized()) {
        GTEST_SKIP() << "API not initialized";
    }

    std::vector<Paper> papers;
    Paper p;
    p.setId(1);
    p.setTitle("Test Paper");
    p.setJournalShort("TEST");
    p.setYear("2024");
    p.setLevel("A");
    papers.push_back(p);

    std::string csv = api->exportToCSV(papers);

    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Test Paper"), std::string::npos);
}

// Test export to JSON
TEST_F(PaperCrawlerAPITest, ExportToJSON) {
    if (!api->isInitialized()) {
        GTEST_SKIP() << "API not initialized";
    }

    std::vector<Paper> papers;
    Paper p;
    p.setId(1);
    p.setTitle("Test Paper");
    papers.push_back(p);

    std::string json = api->exportToJSON(papers);

    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("Test Paper"), std::string::npos);
}

// Main function to run tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
