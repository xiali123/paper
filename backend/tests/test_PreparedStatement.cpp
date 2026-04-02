#include <gtest/gtest.h>
#include "data/PreparedStatement.hpp"
#include "data/IDatabase.hpp"

using namespace PaperCrawler;

class PreparedStatementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化测试数据库
        // database_ = std::make_shared<MockDatabase>();
        // database_->connect(":memory:");
    }

    void TearDown() override {
        // 清理
    }

    std::shared_ptr<IDatabase> database_;
};

TEST_F(PreparedStatementTest, BindPositionalParams) {
    PreparedStatement stmt(database_, "SELECT * FROM papers WHERE id = ? AND title = ?");

    stmt.bind(1, 123);
    stmt.bind(2, std::string("Test Paper"));

    std::string finalSQL = stmt.buildFinalSQL();

    EXPECT_EQ(finalSQL, "SELECT * FROM papers WHERE id = 123 AND title = 'Test Paper'");
}

TEST_F(PreparedStatementTest, BindNamedParams) {
    PreparedStatement stmt(database_,
        "SELECT * FROM papers WHERE id = :id AND title = :title AND year = :year");

    stmt.bind("id", 456);
    stmt.bind("title", std::string("Another Paper"));
    stmt.bind("year", 2024);

    std::string finalSQL = stmt.buildFinalSQL();

    EXPECT_EQ(finalSQL,
        "SELECT * FROM papers WHERE id = 456 AND title = 'Another Paper' AND year = 2024");
}

TEST_F(PreparedStatementTest, EscapeStrings) {
    PreparedStatement stmt(database_, "INSERT INTO papers (title) VALUES (?)");

    stmt.bind(1, std::string("Paper's Title"));

    std::string finalSQL = stmt.buildFinalSQL();

    EXPECT_EQ(finalSQL, "INSERT INTO papers (title) VALUES ('Paper''s Title')");
}

TEST_F(PreparedStatementTest, NullValues) {
    PreparedStatement stmt(database_, "INSERT INTO papers (title, abstract) VALUES (?, ?)");

    stmt.bind(1, std::string("Test"));
    stmt.bind(2, nullptr);

    std::string finalSQL = stmt.buildFinalSQL();

    EXPECT_EQ(finalSQL, "INSERT INTO papers (title, abstract) VALUES ('Test', NULL)");
}

TEST_F(PreparedStatementTest, BooleanValues) {
    PreparedStatement stmt(database_, "SELECT * FROM papers WHERE is_read = ? AND is_favorite = ?");

    stmt.bind(1, true);
    stmt.bind(2, false);

    std::string finalSQL = stmt.buildFinalSQL();

    EXPECT_EQ(finalSQL, "SELECT * FROM papers WHERE is_read = 1 AND is_favorite = 0");
}

TEST_F(PreparedStatementTest, DoubleValues) {
    PreparedStatement stmt(database_, "SELECT * FROM papers WHERE score > ?");

    stmt.bind(1, 8.75);

    std::string finalSQL = stmt.buildFinalSQL();

    EXPECT_EQ(finalSQL, "SELECT * FROM papers WHERE score > 8.75");
}

TEST_F(PreparedStatementTest, ClearParams) {
    PreparedStatement stmt(database_, "SELECT * FROM papers WHERE id = ? AND title = ?");

    stmt.bind(1, 123);
    stmt.bind(2, std::string("Test"));

    EXPECT_NO_THROW(stmt.clear());

    // 清除后参数应该为空
    // 这里我们无法直接访问positionalParams_，但可以验证构建SQL不会失败
}

// QueryBuilder测试
class QueryBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // database_ = std::make_shared<MockDatabase>();
    }

    std::shared_ptr<IDatabase> database_;
};

TEST_F(QueryBuilderTest, BasicSelect) {
    QueryBuilder builder(database_);

    std::string sql = builder
        .select({"id", "title", "authors"})
        .from("papers")
        .buildSQL();

    EXPECT_EQ(sql, "SELECT id, title, authors FROM papers");
}

TEST_F(QueryBuilderTest, SelectWithWhere) {
    QueryBuilder builder(database_);

    std::string sql = builder
        .select()
        .from("papers")
        .where("year", ">", 2020)
        .where("is_read", "=", true)
        .buildSQL();

    EXPECT_EQ(sql, "SELECT * FROM papers WHERE (year > 2020) AND (is_read = 1)");
}

TEST_F(QueryBuilderTest, SelectWithJoin) {
    QueryBuilder builder(database_);

    std::string sql = builder
        .select()
        .from("papers")
        .join("authors", "papers.author_id = authors.id")
        .where("authors.name", "=", std::string("John Doe"))
        .buildSQL();

    EXPECT_EQ(sql, "SELECT * FROM papers JOIN authors ON papers.author_id = authors.id WHERE (authors.name = 'John Doe')");
}

TEST_F(QueryBuilderTest, SelectWithOrderByAndLimit) {
    QueryBuilder builder(database_);

    std::string sql = builder
        .select()
        .from("papers")
        .orderBy("year", false)
        .limit(10)
        .offset(20)
        .buildSQL();

    EXPECT_EQ(sql, "SELECT * FROM papers ORDER BY year DESC LIMIT 10 OFFSET 20");
}

TEST_F(QueryBuilderTest, SelectWithGroupBy) {
    QueryBuilder builder(database_);

    std::string sql = builder
        .select({"year", "COUNT(*) as count"})
        .from("papers")
        .groupBy("year")
        .having("count > 10")
        .buildSQL();

    EXPECT_EQ(sql, "SELECT year, COUNT(*) as count FROM papers GROUP BY year HAVING count > 10");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
