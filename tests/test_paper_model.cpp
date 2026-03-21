#include <gtest/gtest.h>
#include "models/Paper.hpp"
#include "models/Journal.hpp"

using namespace PaperCrawler;

/**
 * @brief Test fixture for Paper model tests
 */
class PaperTest : public ::testing::Test {
protected:
    Paper paper;

    void SetUp() override {
        // Set up a default paper for testing
        paper.setId(1);
        paper.setKid(100);
        paper.setType("database");
        paper.setTitle("Test Paper Title");
        paper.setJournalFull("Journal of Very Important Results");
        paper.setJournalShort("JVIR");
        paper.setYear("2024");
        paper.setAuthor("John Doe, Jane Smith");
        paper.setJournalUrl("https://example.com/journal");
        paper.setDoiUrl("https://doi.org/10.1234/test");
        paper.setInfo("Test abstract and information");
        paper.setQkid(50);
        paper.setLevel("A");
    }
};

/**
 * @brief Test paper getters
 */
TEST_F(PaperTest, GettersReturnCorrectValues) {
    EXPECT_EQ(paper.getId(), 1);
    EXPECT_EQ(paper.getKid(), 100);
    EXPECT_EQ(paper.getType(), "database");
    EXPECT_EQ(paper.getTitle(), "Test Paper Title");
    EXPECT_EQ(paper.getJournalFull(), "Journal of Very Important Results");
    EXPECT_EQ(paper.getJournalShort(), "JVIR");
    EXPECT_EQ(paper.getYear(), "2024");
    EXPECT_EQ(paper.getAuthor(), "John Doe, Jane Smith");
    EXPECT_EQ(paper.getJournalUrl(), "https://example.com/journal");
    EXPECT_EQ(paper.getDoiUrl(), "https://doi.org/10.1234/test");
    EXPECT_EQ(paper.getInfo(), "Test abstract and information");
    EXPECT_EQ(paper.getQkid(), 50);
    EXPECT_EQ(paper.getLevel(), "A");
}

/**
 * @brief Test paper setters
 */
TEST_F(PaperTest, SettersModifyValues) {
    paper.setId(2);
    EXPECT_EQ(paper.getId(), 2);

    paper.setTitle("Modified Title");
    EXPECT_EQ(paper.getTitle(), "Modified Title");

    paper.setYear("2025");
    EXPECT_EQ(paper.getYear(), "2025");

    paper.setLevel("B");
    EXPECT_EQ(paper.getLevel(), "B");
}

/**
 * @brief Test default paper construction
 */
TEST_F(PaperTest, DefaultConstruction) {
    Paper defaultPaper;
    EXPECT_EQ(defaultPaper.getId(), 0);
    EXPECT_EQ(defaultPaper.getKid(), 0);
    EXPECT_EQ(defaultPaper.getQkid(), 0);
    EXPECT_TRUE(defaultPaper.getType().empty());
    EXPECT_TRUE(defaultPaper.getTitle().empty());
}

/**
 * @brief Test toString method
 */
TEST_F(PaperTest, ToStringReturnsCorrectFormat) {
    std::string str = paper.toString();
    EXPECT_NE(str.find("id=1"), std::string::npos);
    EXPECT_NE(str.find("title=Test Paper Title"), std::string::npos);
    EXPECT_NE(str.find("journal=JVIR"), std::string::npos);
    EXPECT_NE(str.find("year=2024"), std::string::npos);
}

/**
 * @brief Test paper with special characters in title
 */
TEST(PaperTestSpecialChars, HandlesSpecialCharacters) {
    Paper paper;
    paper.setTitle("Paper with \"quotes\" and 'apostrophes'");
    paper.setAuthor("Müller, François, José");

    EXPECT_EQ(paper.getTitle(), "Paper with \"quotes\" and 'apostrophes'");
    EXPECT_EQ(paper.getAuthor(), "Müller, François, José");
}

/**
 * @brief Test paper with empty optional fields
 */
TEST(PaperTestOptionalFields, HandlesEmptyOptionalFields) {
    Paper paper;
    paper.setId(1);
    paper.setTitle("Minimal Paper");

    EXPECT_TRUE(paper.getJournalFull().empty());
    EXPECT_TRUE(paper.getDoiUrl().empty());
    EXPECT_TRUE(paper.getInfo().empty());
    EXPECT_EQ(paper.getQkid(), 0);
}

/**
 * @brief Test Journal model
 */
class JournalTest : public ::testing::Test {
protected:
    Journal journal;

    void SetUp() override {
        journal.setId(1);
        journal.setQkid(100);
        journal.setName("JVIR");
        journal.setFull("Journal of Very Important Results");
        journal.setLevel("A");
        journal.setInfo("Premier journal in testing");
    }
};

/**
 * @brief Test journal getters
 */
TEST_F(JournalTest, GettersReturnCorrectValues) {
    EXPECT_EQ(journal.getId(), 1);
    EXPECT_EQ(journal.getQkid(), 100);
    EXPECT_EQ(journal.getName(), "JVIR");
    EXPECT_EQ(journal.getFull(), "Journal of Very Important Results");
    EXPECT_EQ(journal.getLevel(), "A");
    EXPECT_EQ(journal.getInfo(), "Premier journal in testing");
}

/**
 * @brief Test journal setters
 */
TEST_F(JournalTest, SettersModifyValues) {
    journal.setLevel("B");
    EXPECT_EQ(journal.getLevel(), "B");

    journal.setName("NEW");
    EXPECT_EQ(journal.getName(), "NEW");
}

/**
 * @brief Test default journal construction
 */
TEST_F(JournalTest, DefaultConstruction) {
    Journal defaultJournal;
    EXPECT_EQ(defaultJournal.getId(), 0);
    EXPECT_EQ(defaultJournal.getQkid(), 0);
    EXPECT_TRUE(defaultJournal.getName().empty());
    EXPECT_TRUE(defaultJournal.getFull().empty());
}
