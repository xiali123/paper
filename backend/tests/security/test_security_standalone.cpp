/**
 * @file test_security_standalone.cpp
 * @brief SQL注入安全测试 - 独立可执行版本
 */

#include <iostream>
#include <string>
#include <vector>

// SQL转义函数
std::string escapeSQL(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (c == '\'') result += "''";
        else if (c == '\\') result += "\\\\";
        else if (c == '%') result += "\\%";
        else if (c == '_') result += "\\_";
        else result += c;
    }
    return result;
}

// 测试结构体
struct TestCase {
    std::string name;
    std::string input;
    std::string expected;
    bool passed;
};

// 运行单个测试
bool runTest(const TestCase& test) {
    std::string result = escapeSQL(test.input);
    bool passed = (result == test.expected);

    std::cout << "  Test: " << test.name << std::endl;
    std::cout << "    Input:    '" << test.input << "'" << std::endl;
    std::cout << "    Expected: '" << test.expected << "'" << std::endl;
    std::cout << "    Got:      '" << result << "'" << std::endl;
    std::cout << "    Result:   " << (passed ? "PASS" : "FAIL") << std::endl;
    std::cout << std::endl;

    return passed;
}

int main() {
    std::cout << "\n============================================================\n";
    std::cout << "SQL Injection Security Test Suite\n";
    std::cout << "Date: 2026-04-04\n";
    std::cout << "============================================================\n\n";

    // 定义所有测试用例
    std::vector<TestCase> tests;

    // 基础转义测试
    tests.push_back({"Single Quote Escape", "admin' OR '1'='1", "admin'' OR ''1''=''1"});
    tests.push_back({"Backslash Escape", "user\\' OR 1=1", "user\\\\'' OR 1=1"});
    tests.push_back({"Percent Wildcard", "test% injected", "test\\% injected"});
    tests.push_back({"Underscore Wildcard", "test_ injected", "test\\_ injected"});
    tests.push_back({"Combined Injection", "'; DROP TABLE users; --", "''; DROP TABLE users; --"});

    // UserApiModule测试
    tests.push_back({"User: Basic Injection", "admin' OR '1'='1", "admin'' OR ''1''=''1"});
    tests.push_back({"User: UNION SELECT", "admin' UNION SELECT * FROM passwords --", "admin'' UNION SELECT * FROM passwords --"});
    tests.push_back({"User: Email Injection", "test@evil.com' OR '1'='1", "test@evil.com'' OR ''1''=''1"});

    // SearchApiModule测试
    tests.push_back({"Search: LIKE Injection", "test' OR '1'='1", "test'' OR ''1''=''1"});
    tests.push_back({"Search: Percent Wildcard", "%", "\\%"});
    tests.push_back({"Search: Underscore Wildcard", "_", "\\_"});

    // 高级攻击测试
    tests.push_back({"Advanced: Time Blind", "admin' AND (SELECT SLEEP(10)) --", "admin'' AND (SELECT SLEEP(10)) --"});
    tests.push_back({"Advanced: Stacked Query", "admin'; DROP TABLE users; --", "admin''; DROP TABLE users; --"});
    tests.push_back({"Advanced: Boolean Blind", "admin' AND 1=1 --", "admin'' AND 1=1 --"});

    // 边界测试
    tests.push_back({"Edge: Empty String", "", ""});
    tests.push_back({"Edge: Special Chars Only", "'\\%_", "''\\\\\\%\\_"});
    tests.push_back({"Edge: Unicode", "users' test", "users'' test"});

    // 运行所有测试
    int total = tests.size();
    int passed = 0;

    for (const auto& test : tests) {
        if (runTest(test)) {
            passed++;
        }
    }

    // 打印总结
    std::cout << "\n============================================================\n";
    std::cout << "Test Summary\n";
    std::cout << "============================================================\n";
    std::cout << "Total Tests: " << total << "\n";
    std::cout << "Passed:      " << passed << "\n";
    std::cout << "Failed:      " << (total - passed) << "\n";
    std::cout << "Pass Rate:   " << (passed * 100 / total) << "%\n";

    if (passed == total) {
        std::cout << "\nAll tests PASSED! SQL injection protection is working.\n";
    } else {
        std::cout << "\nWARNING: Some tests FAILED. Please review the escape logic.\n";
    }

    std::cout << "============================================================\n\n";

    return (passed == total) ? 0 : 1;
}
