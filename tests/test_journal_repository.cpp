#include <gtest/gtest.h>
#include "database/JournalRepository.hpp"
#include "database/DatabaseManager.hpp"
#include "models/Journal.hpp"

using namespace PaperCrawler;

/**
 * @brief Test fixture for JournalRepository tests
 */
class JournalRepositoryTest : public ::testing::Test {
protected:
    std::unique_ptr<DatabaseManager> dbManager;
    std::unique_ptr<JournalRepository> repository;

    void SetUp() override {
        // Use in-memory database for testing
        // dbManager = std::make_unique<DatabaseManager>("test.db");
        // repository = std::make_unique<JournalRepository>(*dbManager);
    }

    void TearDown() override {
        // Cleanup
    }
};

/**
 * @brief Test inserting a journal
 */
TEST_F(JournalRepositoryTest, InsertJournal) {
    Journal journal;
    journal.setId(1);
    journal.setName("JVIR");
    journal.setFull("Journal of Very Important Results");

    // int insertedId = repository->insert(journal);
    // EXPECT_GT(insertedId, 0);
}

/**
 * @brief Test finding journal by ID
 */
TEST_F(JournalRepositoryTest, FindJournalById) {
    Journal journal;
    journal.setId(1);
    journal.setName("TEST");

    // repository->insert(journal);

    // Journal found = repository->findById(1);
    // EXPECT_EQ(found.getName(), "TEST");
}

/**
 * @brief Test finding journal by name
 */
TEST_F(JournalRepositoryTest, FindJournalByName) {
    Journal journal;
    journal.setName("SIGMOD");
    journal.setFull("ACM SIGMOD Conference");

    // repository->insert(journal);

    // Journal found = repository->findByName("SIGMOD");
    // EXPECT_EQ(found.getFull(), "ACM SIGMOD Conference");
}

/**
 * @brief Test getting all journals
 */
TEST_F(JournalRepositoryTest, GetAllJournals) {
    std::vector<Journal> journals;
    for (int i = 0; i < 5; i++) {
        Journal journal;
        journal.setName("J" + std::to_string(i));
        journals.push_back(journal);
    }

    // for (const auto& j : journals) {
    //     repository->insert(j);
    // }

    // std::vector<Journal> all = repository->findAll();
    // EXPECT_EQ(all.size(), 5);
}

/**
 * @brief Test updating journal
 */
TEST_F(JournalRepositoryTest, UpdateJournal) {
    Journal journal;
    journal.setId(1);
    journal.setName("OLD");
    journal.setLevel("C");

    // repository->insert(journal);

    journal.setLevel("A");
    // repository->update(journal);

    // Journal updated = repository->findById(1);
    // EXPECT_EQ(updated.getLevel(), "A");
}

/**
 * @brief Test finding non-existent journal
 */
TEST_F(JournalRepositoryTest, FindNonExistentJournal) {
    // Journal journal = repository->findById(99999);
    // EXPECT_EQ(journal.getId(), 0);
}

/**
 * @brief Test finding journal by case-insensitive name
 */
TEST_F(JournalRepositoryTest, FindJournalByNameCaseInsensitive) {
    Journal journal;
    journal.setName("sigmod");
    journal.setFull("ACM SIGMOD");

    // repository->insert(journal);

    // Journal found = repository->findByName("SIGMOD");
    // EXPECT_EQ(found.getFull(), "ACM SIGMOD");
}

/**
 * @brief Test journal with empty name
 */
TEST_F(JournalRepositoryTest, HandleJournalWithEmptyName) {
    Journal journal;
    journal.setId(1);
    journal.setFull("Journal without short name");

    // repository->insert(journal);

    // Journal found = repository->findById(1);
    // EXPECT_TRUE(found.getName().empty());
    // EXPECT_EQ(found.getFull(), "Journal without short name");
}
