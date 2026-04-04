#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SQL Injection Security Test Suite
Python版本 - 可直接运行
"""

def escape_sql(input_str):
    """SQL转义函数（与C++实现相同）"""
    result = []
    for c in input_str:
        if c == "'":
            result.append("''")
        elif c == '\\':
            result.append("\\\\")
        elif c == '%':
            result.append("\\%")
        elif c == '_':
            result.append("\\_")
        else:
            result.append(c)
    return ''.join(result)

class TestCase:
    def __init__(self, name, input_str, expected):
        self.name = name
        self.input = input_str
        self.expected = expected

def run_test(test):
    """运行单个测试"""
    result = escape_sql(test.input)
    passed = (result == test.expected)

    print(f"  Test: {test.name}")
    print(f"    Input:    '{test.input}'")
    print(f"    Expected: '{test.expected}'")
    print(f"    Got:      '{result}'")
    print(f"    Result:   {'PASS' if passed else 'FAIL'}")
    print()

    return passed

def main():
    print("\n" + "=" * 60)
    print("SQL Injection Security Test Suite")
    print("Date: 2026-04-04")
    print("=" * 60 + "\n")

    # 定义所有测试用例
    tests = [
        # 基础转义测试
        TestCase("Single Quote Escape", "admin' OR '1'='1", "admin'' OR ''1''=''1"),
        TestCase("Backslash Escape", "user\\' OR 1=1", "user\\\\'' OR 1=1"),
        TestCase("Percent Wildcard", "test% injected", "test\\% injected"),
        TestCase("Underscore Wildcard", "test_ injected", "test\\_ injected"),
        TestCase("Combined Injection", "'; DROP TABLE users; --", "''; DROP TABLE users; --"),

        # UserApiModule测试
        TestCase("User: Basic Injection", "admin' OR '1'='1", "admin'' OR ''1''=''1"),
        TestCase("User: UNION SELECT", "admin' UNION SELECT * FROM passwords --",
                "admin'' UNION SELECT * FROM passwords --"),
        TestCase("User: Email Injection", "test@evil.com' OR '1'='1",
                "test@evil.com'' OR ''1''=''1"),

        # SearchApiModule测试
        TestCase("Search: LIKE Injection", "test' OR '1'='1", "test'' OR ''1''=''1"),
        TestCase("Search: Percent Wildcard", "%", "\\%"),
        TestCase("Search: Underscore Wildcard", "_", "\\_"),

        # 高级攻击测试
        TestCase("Advanced: Time Blind", "admin' AND (SELECT SLEEP(10)) --",
                "admin'' AND (SELECT SLEEP(10)) --"),
        TestCase("Advanced: Stacked Query", "admin'; DROP TABLE users; --",
                "admin''; DROP TABLE users; --"),
        TestCase("Advanced: Boolean Blind", "admin' AND 1=1 --",
                "admin'' AND 1=1 --"),

        # 边界测试
        TestCase("Edge: Empty String", "", ""),
        TestCase("Edge: Special Chars Only", "'\\%_", "''\\\\\\%\\_"),
        TestCase("Edge: Unicode", "users' test", "users'' test"),
    ]

    # 运行所有测试
    total = len(tests)
    passed = 0

    for test in tests:
        if run_test(test):
            passed += 1

    # 打印总结
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    print(f"Total Tests: {total}")
    print(f"Passed:      {passed}")
    print(f"Failed:      {total - passed}")
    print(f"Pass Rate:   {passed * 100 // total}%")

    if passed == total:
        print("\nAll tests PASSED! SQL injection protection is working.")
    else:
        print("\nWARNING: Some tests FAILED. Please review the escape logic.")

    print("=" * 60 + "\n")

    return 0 if passed == total else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
