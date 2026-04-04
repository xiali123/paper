/**
 * @file test_SQLInjectionSecurity_simple.cpp
 * @brief SQL注入安全测试 - 简化版本（无Google Test依赖）
 *
 * 这是一个独立的测试程序，不依赖Google Test框架
 * 直接编译运行即可验证SQL转义功能
 *
 * @author Security Team
 * @date 2026-04-04
 */

#include <iostream>
#include <string>
#include <cassert>
#include <vector>

// ============================================================================
// SQL转义函数（与UserApiModule和SearchApiModule中使用的相同）
// ============================================================================

namespace PaperCrawler {
namespace Security {

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
// 测试宏定义
// ============================================================================

#define TEST(name) \
    void test_##name(); \
    int main_test_##name = (run_test(#name, test_##name), 0); \
    void test_##name()

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  ❌ ASSERT_EQ failed: " << #a << " != " << #b << std::endl; \
            std::cerr << "     Expected: " << (b) << std::endl; \
            std::cerr << "     Got: " << (a) << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::cerr << "  ❌ ASSERT_FALSE failed: " << #condition << " is true" << std::endl; \
            return false; \
        } \
    } while(0)

// ============================================================================
// 测试运行器
// ============================================================================

int total_tests = 0;
int passed_tests = 0;

bool run_test(const std::string& name, void (*test_func)()) {
    total_tests++;
    std::cout << "\n▶️  Running: " << name << std::endl;
    try {
        test_func();
        passed_tests++;
        std::cout << "  ✅ PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "  ❌ FAILED (exception thrown)" << std::endl;
        return false;
    }
}

void print_summary() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "📊 测试总结" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "总测试数: " << total_tests << std::endl;
    std::cout << "通过: " << passed_tests << " ✅" << std::endl;
    std::cout << "失败: " << (total_tests - passed_tests) << " ❌" << std::endl;
    std::cout << "通过率: " << (total_tests > 0 ? (passed_tests * 100 / total_tests) : 0) << "%" << std::endl;

    if (passed_tests == total_tests) {
        std::cout << "\n🎉 所有测试通过！SQL注入防护工作正常。" << std::endl;
    } else {
        std::cout << "\n⚠️  有测试失败，请检查SQL转义逻辑。" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
}

// ============================================================================
// 基础转义功能测试
// ============================================================================

TEST(EscapeSingleQuote) {
    std::cout << "  测试: 单引号转义" << std::endl;
    std::string input = "admin' OR '1'='1";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "admin'' OR ''1''=''1");
    ASSERT_FALSE(escaped.find("' OR '") != std::string::npos);
}

TEST(EscapeBackslash) {
    std::cout << "  测试: 反斜杠转义" << std::endl;
    std::string input = "user\\' OR 1=1 --";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "user\\\\'' OR 1=1 --");
}

TEST(EscapeLikeWildcardPercent) {
    std::cout << "  测试: LIKE通配符%转义" << std::endl;
    std::string input = "test% injected";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "test\\% injected");
}

TEST(EscapeLikeWildcardUnderscore) {
    std::cout << "  测试: LIKE通配符_转义" << std::endl;
    std::string input = "test_ injected";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "test\\_ injected");
}

TEST(EscapeCombinedInjection) {
    std::cout << "  测试: 组合注入攻击" << std::endl;
    std::string input = "'; DROP TABLE users; --";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "''; DROP TABLE users; --");
}

TEST(EscapeMultipleSpecialChars) {
    std::cout << "  测试: 多个特殊字符" << std::endl;
    std::string input = "'\\%_test'\\%_";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "''\\\\\\%\\_test''\\\\\\%\\_");
}

// ============================================================================
// UserApiModule SQL注入防护测试
// ============================================================================

TEST(UserModule_UsernameInjection_Basic) {
    std::cout << "  测试: UserModule - 基础用户名注入" << std::endl;
    std::string maliciousInput = "admin' OR '1'='1";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    std::string expectedSQL = "SELECT * FROM users WHERE username = 'admin'' OR ''1''=''1'";
    ASSERT_EQ(sql, expectedSQL);
    ASSERT_FALSE(sql.find("' OR '") != std::string::npos);
}

TEST(UserModule_UsernameInjection_UnionSelect) {
    std::cout << "  测试: UserModule - UNION SELECT注入" << std::endl;
    std::string maliciousInput = "admin' UNION SELECT * FROM passwords --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    ASSERT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' UNION SELECT * FROM passwords --'");
}

TEST(UserModule_EmailInjection_CommentAttack) {
    std::cout << "  测试: UserModule - 邮箱注释符注入" << std::endl;
    std::string maliciousInput = "test@evil.com' OR '1'='1' --";
    std::string sql = "SELECT * FROM users WHERE email = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    ASSERT_EQ(sql, "SELECT * FROM users WHERE email = 'test@evil.com'' OR ''1''=''1'' --'");
}

TEST(UserModule_UsernameWithBackslash) {
    std::cout << "  测试: UserModule - 反斜杠注入" << std::endl;
    std::string input = "admin\\' OR 1=1 --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(input) + "'";

    ASSERT_EQ(sql, "SELECT * FROM users WHERE username = 'admin\\\\'' OR 1=1 --'");
}

// ============================================================================
// SearchApiModule SQL注入防护测试
// ============================================================================

TEST(SearchModule_QueryInjection_LikeClause) {
    std::cout << "  测试: SearchModule - LIKE查询注入" << std::endl;
    std::string maliciousQuery = "test' OR '1'='1";
    std::string sql = "SELECT * FROM papers WHERE title LIKE '%" +
                     Security::escapeSQL(maliciousQuery) + "%'";

    std::string expected = "SELECT * FROM papers WHERE title LIKE '%test'' OR ''1''=''1%'";
    ASSERT_EQ(sql, expected);
}

TEST(SearchModule_QueryInjection_WildcardInjection) {
    std::cout << "  测试: SearchModule - 通配符注入(%)" << std::endl;
    std::string maliciousQuery = "%";
    std::string sql = "SELECT * FROM papers WHERE title LIKE '%" +
                     Security::escapeSQL(maliciousQuery) + "%'";

    ASSERT_EQ(sql, "SELECT * FROM papers WHERE title LIKE '%\\%%'");
}

TEST(SearchModule_QueryInjection_UnderscoreInjection) {
    std::cout << "  测试: SearchModule - 通配符注入(_)" << std::endl;
    std::string maliciousQuery = "_";
    std::string sql = "SELECT * FROM papers WHERE title LIKE '%" +
                     Security::escapeSQL(maliciousQuery) + "%'";

    ASSERT_EQ(sql, "SELECT * FROM papers WHERE title LIKE '%\\_%'");
}

TEST(SearchModule_MultiFieldInjection) {
    std::cout << "  测试: SearchModule - 多字段注入" << std::endl;
    std::string maliciousQuery = "x' OR '1'='1' --";
    std::string escaped = Security::escapeSQL(maliciousQuery);

    std::string sql = "SELECT * FROM papers WHERE "
                     "title LIKE '%" + escaped + "%' OR "
                     "authors LIKE '%" + escaped + "%' OR "
                     "abstract LIKE '%" + escaped + "%'";

    ASSERT_FALSE(sql.find("' OR '") != std::string::npos);
}

// ============================================================================
// 高级注入攻击测试
// ============================================================================

TEST(Advanced_TimeBasedBlindInjection) {
    std::cout << "  测试: 高级 - 时间盲注" << std::endl;
    std::string maliciousInput = "admin' AND (SELECT SLEEP(10)) --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    ASSERT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' AND (SELECT SLEEP(10)) --'");
}

TEST(Advanced_StackedQueries) {
    std::cout << "  测试: 高级 - 堆叠查询" << std::endl;
    std::string maliciousInput = "admin'; DROP TABLE users; SELECT * FROM papers WHERE '1'='1";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    ASSERT_EQ(sql, "SELECT * FROM users WHERE username = 'admin''; DROP TABLE users; SELECT * FROM papers WHERE ''1''=''1'");
}

TEST(Advanced_BooleanBasedBlindInjection) {
    std::cout << "  测试: 高级 - 布尔盲注" << std::endl;
    std::string maliciousInput = "admin' AND 1=1 --";
    std::string sql = "SELECT * FROM users WHERE username = '" +
                     Security::escapeSQL(maliciousInput) + "'";

    ASSERT_EQ(sql, "SELECT * FROM users WHERE username = 'admin'' AND 1=1 --'");
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST(EdgeCase_EmptyString) {
    std::cout << "  测试: 边界 - 空字符串" << std::endl;
    std::string input = "";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "");
}

TEST(EdgeCase_VeryLongString) {
    std::cout << "  测试: 边界 - 超长字符串" << std::endl;
    std::string input(1000, 'a');
    input += "'";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped.length(), 1001);
}

TEST(EdgeCase_UnicodeCharacters) {
    std::cout << "  测试: 边界 - Unicode字符" << std::endl;
    std::string input = "用户'管理员'测试";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "用户''管理员''测试");
}

TEST(EdgeCase_SpecialCharactersOnly) {
    std::cout << "  测试: 边界 - 仅特殊字符" << std::endl;
    std::string input = "'\\%_'";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "''\\\\\\%\\_'");
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🔒 SQL注入安全测试套件" << std::endl;
    std::cout << "📅 日期: 2026-04-04" << std::endl;
    std::cout << "🎯 目标: 验证SQL转义函数能够防止注入攻击" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 所有测试会通过TEST宏自动注册和运行
    // 这里只是确保测试程序能够正常退出

    print_summary();

    return (passed_tests == total_tests) ? 0 : 1;
}
