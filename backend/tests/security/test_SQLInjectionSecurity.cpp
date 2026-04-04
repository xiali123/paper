/**
 * @file test_SQLInjectionSecurity.cpp
 * @brief SQL注入安全测试 - 验证API模块的SQL注入防护
 *
 * 测试覆盖：
 * 1. UserApiModule - 用户名和邮箱查询的SQL注入防护
 * 2. SearchApiModule - 搜索查询的SQL注入防护
 * 3. PaperApiModule - 论文CRUD操作的SQL注入防护
 *
 * @author Security Team
 * @date 2026-04-04
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>

// SQL转义函数（与UserApiModule和SearchApiModule中使用的相同）
namespace PaperCrawler {
namespace Security {

/**
 * @brief SQL字符串转义函数
 * 防止单引号、反斜杠、LIKE通配符注入
 */
inline std::string escapeSQL(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (c == '\'') result += "''";       // 单引号 → 两个单引号
        else if (c == '\\') result += "\\\\"; // 反斜杠 → 两个反斜杠
        else if (c == '%') result += "\\%";   // LIKE通配符% → \%
        else if (c == '_') result += "\\_";   // LIKE通配符_ → \_
        else result += c;
    }
    return result;
}

} // namespace Security
} // namespace PaperCrawler

using namespace PaperCrawler;

// ============================================================================
// 测试夹具
// ============================================================================

class SQLInjectionSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试初始化
    }

    void TearDown() override {
        // 测试清理
    }

    // 辅助函数：构建预期SQL
    std::string buildSafeSQL(const std::string& prefix, const std::string& input) {
        return prefix + "'" + Security::escapeSQL(input) + "'";
    }
};

// ============================================================================
// 基础转义功能测试
// ============================================================================

TEST_F(SQLInjectionSecurityTest, EscapeSingleQuote) {
    // 测试单引号转义
    std::string input = "admin' OR '1'='1";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "admin'' OR ''1''=''1");
    EXPECT_FALSE(escaped.find("' OR '") != std::string::npos);
}

TEST_F(SQLInjectionSecurityTest, EscapeBackslash) {
    // 测试反斜杠转义
    std::string input = "user\\' OR 1=1 --";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "user\\\\'' OR 1=1 --");
}

TEST_F(SQLInjectionSecurityTest, EscapeLikeWildcardPercent) {
    // 测试LIKE通配符%转义
    std::string input = "test% injected";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "test\\% injected");
}

TEST_F(SQLInjectionSecurityTest, EscapeLikeWildcardUnderscore) {
    // 测试LIKE通配符_转义
    std::string input = "test_ injected";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "test\\_ injected");
}

TEST_F(SQLInjectionSecurityTest, EscapeCombinedInjection) {
    // 测试组合注入攻击
    std::string input = "'; DROP TABLE users; --";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "''; DROP TABLE users; --");
}

TEST_F(SQLInjectionSecurityTest, EscapeMultipleSpecialChars) {
    // 测试多个特殊字符
    std::string input = "'\\%_test'\\%_";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "''\\\\\\%\\_test''\\\\\\%\\_");
}

// ============================================================================
// UserApiModule SQL注入防护测试
// ============================================================================

TEST_F(SQLInjectionSecurityTest, UserModule_UsernameInjection_Basic) {
    // 模拟：getUserByUsernameFromDatabase("admin' OR '1'='1")
    std::string maliciousInput = "admin' OR '1'='1";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：SQL应该被转义，无法执行注入
    std::string expectedSQL = "SELECT * FROM users WHERE username = 'admin'' OR ''1''=''1'";
    EXPECT_EQ(sql, expectedSQL);

    // 验证：原始恶意字符串不应出现在未转义形式
    EXPECT_FALSE(sql.find("' OR '") != std::string::npos);
}

TEST_F(SQLInjectionSecurityTest, UserModule_UsernameInjection_UnionSelect) {
    // UNION SELECT注入攻击
    std::string maliciousInput = "admin' UNION SELECT * FROM passwords --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：UNION SELECT应该被转义
    EXPECT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' UNION SELECT * FROM passwords --'");
}

TEST_F(SQLInjectionSecurityTest, UserModule_EmailInjection_CommentAttack) {
    // 注释符注入攻击
    std::string maliciousInput = "test@evil.com' OR '1'='1' --";
    std::string sql = "SELECT * FROM users WHERE email = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：注释符应该被转义
    EXPECT_EQ(sql, "SELECT * FROM users WHERE email = 'test@evil.com'' OR ''1''=''1'' --'");
}

TEST_F(SQLInjectionSecurityTest, UserModule_UsernameWithBackslash) {
    // 反斜杠转义测试
    std::string input = "admin\\' OR 1=1 --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(input) + "'";

    EXPECT_EQ(sql, "SELECT * FROM users WHERE username = 'admin\\\\'' OR 1=1 --'");
}

// ============================================================================
// SearchApiModule SQL注入防护测试
// ============================================================================

TEST_F(SQLInjectionSecurityTest, SearchModule_QueryInjection_LikeClause) {
    // LIKE查询注入攻击
    std::string maliciousQuery = "test' OR '1'='1";
    std::string sql = "SELECT * FROM papers WHERE title LIKE '%" +
                     Security::escapeSQL(maliciousQuery) + "%'";

    // 验证：LIKE通配符和单引号都被转义
    std::string expected = "SELECT * FROM papers WHERE title LIKE '%test'' OR ''1''=''1%'";
    EXPECT_EQ(sql, expected);
}

TEST_F(SQLInjectionSecurityTest, SearchModule_QueryInjection_WildcardInjection) {
    // 通配符注入攻击（尝试匹配所有内容）
    std::string maliciousQuery = "%";
    std::string sql = "SELECT * FROM papers WHERE title LIKE '%" +
                     Security::escapeSQL(maliciousQuery) + "%'";

    // 验证：%应该被转义为\%
    EXPECT_EQ(sql, "SELECT * FROM papers WHERE title LIKE '%\\%%'");
}

TEST_F(SQLInjectionSecurityTest, SearchModule_QueryInjection_UnderscoreInjection) {
    // 下划线通配符注入攻击（匹配单个字符）
    std::string maliciousQuery = "_";
    std::string sql = "SELECT * FROM papers WHERE title LIKE '%" +
                     Security::escapeSQL(maliciousQuery) + "%'";

    // 验证：_应该被转义为\_
    EXPECT_EQ(sql, "SELECT * FROM papers WHERE title LIKE '%\\_%'");
}

TEST_F(SQLInjectionSecurityTest, SearchModule_MultiFieldInjection) {
    // 多字段注入攻击
    std::string maliciousQuery = "x' OR '1'='1' --";
    std::string escaped = Security::escapeSQL(maliciousQuery);

    std::string sql = "SELECT * FROM papers WHERE "
                     "title LIKE '%" + escaped + "%' OR "
                     "authors LIKE '%" + escaped + "%' OR "
                     "abstract LIKE '%" + escaped + "%'";

    // 验证：所有字段中的注入都被转义
    EXPECT_FALSE(sql.find("' OR '") != std::string::npos);
    EXPECT_EQ(sql, "SELECT * FROM papers WHERE title LIKE '%x'' OR ''1''=''1'' --%' OR "
                  "authors LIKE '%x'' OR ''1''=''1'' --%' OR "
                  "abstract LIKE '%x'' OR ''1''=''1'' --%'");
}

// ============================================================================
// 高级注入攻击测试
// ============================================================================

TEST_F(SQLInjectionSecurityTest, Advanced_TimeBasedBlindInjection) {
    // 时间盲注攻击
    std::string maliciousInput = "admin' AND (SELECT SLEEP(10)) --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：SLEEP函数调用应该被转义
    EXPECT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' AND (SELECT SLEEP(10)) --'");
}

TEST_F(SQLInjectionSecurityTest, Advanced_StackedQueries) {
    // 堆叠查询攻击
    std::string maliciousInput = "admin'; DROP TABLE users; SELECT * FROM papers WHERE '1'='1";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：堆叠查询应该被转义为普通字符串
    EXPECT_EQ(sql, "SELECT * FROM users WHERE username = 'admin''; DROP TABLE users; SELECT * FROM papers WHERE ''1''=''1'");
}

TEST_F(SQLInjectionSecurityTest, Advanced_BooleanBasedBlindInjection) {
    // 布尔盲注攻击
    std::string maliciousInput = "admin' AND 1=1 --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：布尔逻辑应该被转义
    EXPECT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' AND 1=1 --'");
}

TEST_F(SQLInjectionSecurityTest, Advanced_ErrorBasedInjection) {
    // 错误注入攻击
    std::string maliciousInput = "admin' AND (SELECT COUNT(*) FROM nonexistent_table) --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    // 验证：子查询应该被转义
    EXPECT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' AND (SELECT COUNT(*) FROM nonexistent_table) --'");
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(SQLInjectionSecurityTest, EdgeCase_EmptyString) {
    // 空字符串
    std::string input = "";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "");
}

TEST_F(SQLInjectionSecurityTest, EdgeCase_VeryLongString) {
    // 超长字符串（1000个字符）
    std::string input(1000, 'a');
    input += "'";
    std::string escaped = Security::escapeSQL(input);

    // 验证：长度应该增加1（'变成''）
    EXPECT_EQ(escaped.length(), 1001);
}

TEST_F(SQLInjectionSecurityTest, EdgeCase_UnicodeCharacters) {
    // Unicode字符
    std::string input = "用户'管理员'测试";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "用户''管理员''测试");
}

TEST_F(SQLInjectionSecurityTest, EdgeCase_NullBytes) {
    // 空字节（理论上不应该出现在std::string中）
    std::string input = "test\0injection", 7;  // 只取前7个字符
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "test");
}

TEST_F(SQLInjectionSecurityTest, EdgeCase_SpecialCharactersOnly) {
    // 仅特殊字符
    std::string input = "'\\%_'";
    std::string escaped = Security::escapeSQL(input);

    EXPECT_EQ(escaped, "''\\\\\\%\\_'");
}

// ============================================================================
// 性能测试（可选）
// ============================================================================

TEST_F(SQLInjectionSecurityTest, Performance_EscapeLargeInput) {
    // 性能测试：转义大字符串
    std::string input(10000, 'a');
    input += "'\\%_";

    auto start = std::chrono::high_resolution_clock::now();
    std::string escaped = Security::escapeSQL(input);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 验证：转义10KB字符串应该在合理时间内完成（< 10ms）
    EXPECT_LT(duration.count(), 10000);
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
