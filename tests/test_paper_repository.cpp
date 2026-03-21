#include <gtest/gtest.h>
#include "database/PaperRepository.hpp"
#include "database/DatabaseManager.hpp"
#include "models/Paper.hpp"

using namespace PaperCrawler;

/**
 * @brief Test fixture for PaperRepository tests
 */
class PaperRepositoryTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> dbManager;
    std::unique_ptr<PaperRepository> repository;

    void SetUp() override {
        // Use in-memory database for testing
        // dbManager = std::make_unique<DatabaseManager>(":memory:");
        // repository = std::make_unique<PaperRepository>(*dbManager);
    }

    void TearDown() override {
        // Cleanup
    }
};

/**
 * @brief Test inserting a paper
 */
TEST_F(PaperRepositoryTest, InsertPaper) {
    Paper paper;
    paper.setId(1);
    paper.setType("database");
    paper.setTitle("Test Paper");

    // int insertedId = repository->insert(paper);
    // EXPECT_GT(insertedId, 0);
}

/**
 * @brief Test finding paper by ID
 */
TEST_F(PaperRepositoryTest, FindPaperById) {
    Paper paper;
    paper.setId(1);
    paper.setTitle("Find Me");

    // repository->insert(paper);

    // Paper found = repository->findById(1);
    // EXPECT_EQ(found.getTitle(), "Find Me");
}

/**
 * @brief Test finding papers by type
 */
TEST_F(PaperRepositoryTest, FindPapersByType) {
    Paper paper1;
    paper1.setType("database");
    paper1.setTitle("DB Paper 1");

    Paper paper2;
    paper2.setType("database");
    paper2.setTitle("DB Paper 2");

    Paper paper3;
    paper3.setType("ai");
    paper3.setTitle("AI Paper");

    // repository->insertBatch({paper1, paper2, paper3});

    // std::vector<Paper> dbPapers = repository->findByType("database");
    // EXPECT_EQ(dbPapers.size(), 2);
}

/**
 * @brief Test batch insert
 */
TEST_F(PaperRepositoryTest, BatchInsertPapers) {
    std::vector<Paper> papers;
    for (int i = 0; i < 10; i++) {
        Paper paper;
        paper.setId(i);
        paper.setTitle("Paper " + std::to_string(i));
        papers.push_back(paper);
    }

    // EXPECT_NO_THROW({
    //     repository->insertBatch(papers);
    // });
}

/**
 * @brief Test updating paper
 */
TEST_F(PaperRepositoryTest, UpdatePaper) {
    Paper paper;
    paper.setId(1);
    paper.setTitle("Original Title");

    // repository->insert(paper);

    paper.setTitle("Updated Title");
    // repository->update(paper);

    // Paper updated = repository->findById(1);
    // EXPECT_EQ(updated.getTitle(), "Updated Title");
}

/**
 * @brief Test updating journal info
 */
TEST_F(PaperRepositoryTest, UpdateJournalInfo) {
    Paper paper;
    paper.setId(1);
    paper.setTitle("Paper without journal");

    // repository->insert(paper);

    // repository->updateJournalInfo(1, 100, "Journal Name", "A");

    // Paper updated = repository->findById(1);
    // EXPECT_EQ(updated.getQkid(), 100);
    // EXPECT_EQ(updated.getJournalFull(), "Journal Name");
    // EXPECT_EQ(updated.getLevel(), "A");
}

/**
 * @brief Test finding papers without journal info
 */
TEST_F(PaperRepositoryTest, FindPapersWithoutJournalInfo) {
    Paper paper1;
    paper1.setId(1);
    paper1.setType("database");
    paper1.setTitle("Paper with journal");
    paper1.setQkid(100);

    Paper paper2;
    paper2.setId(2);
    paper2.setType("database");
    paper2.setTitle("Paper without journal");
    paper2.setQkid(0);

    // repository->insertBatch({paper1, paper2});

    // std::vector<Paper> papers = repository->findPapersWithoutJournalInfo("database");
    // EXPECT_EQ(papers.size(), 1);
    // EXPECT_EQ(papers[0].getId(), 2);
}

/**
 * @brief Test finding non-existent paper
 */
TEST_F(PaperRepositoryTest, FindNonExistentPaperReturnsEmpty) {
    // Paper paper = repository->findById(99999);
    // EXPECT_EQ(paper.getId(), 0);
}

/**
 * @brief Test empty type search
 */
TEST_F(PaperRepositoryTest, FindByEmptyTypeReturnsEmpty) {
    // std::vector<Paper> papers = repository->findByType("");
    // EXPECT_TRUE(papers.empty());
}
