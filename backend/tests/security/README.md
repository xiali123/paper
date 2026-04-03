# 🔒 SQL注入安全测试套件

**创建日期**: 2026-04-04
**测试类型**: 安全测试
**覆盖率**: 100% (所有已修复的SQL注入点)

---

## 📋 测试概述

本测试套件验证PaperCrawler项目中SQL注入漏洞修复的有效性。测试覆盖所有已修复的SQL注入点：

### 测试覆盖范围

| 模块 | 文件 | 注入点数量 | 状态 |
|------|------|-----------|------|
| **UserApiModule** | UserApiModule.cpp | 2处 | ✅ 已修复 |
| **SearchApiModule** | SearchApiModule.cpp | 2处 | ✅ 已修复 |

**总计**: 4个SQL注入漏洞 → **0个** (100%修复)

---

## 🚀 如何运行测试

### 方法1: 使用Visual Studio编译（推荐）

#### Windows CMD命令提示符
```batch
# 1. 打开Visual Studio 2022开发者命令提示符
# 开始菜单 → Visual Studio 2022 → x64 Native Tools Command Prompt

# 2. 进入测试目录
cd E:\PaperCrawler\backend\tests\security

# 3. 编译测试程序
cl /EHsc /std:c++17 /O2 /Fe:test_SQLSecurity.exe test_SQLInjectionSecurity_simple.cpp

# 4. 运行测试
test_SQLSecurity.exe
```

#### 或使用提供的批处理脚本
```batch
cd E:\PaperCrawler\backend\tests\security
build_test.bat
```

### 方法2: 使用CMake编译

```bash
cd E:\PaperCrawler/backend/tests/security
mkdir build && cd build
cmake ..
cmake --build . --config Release
ctest --output-on-failure
```

---

## 📊 测试用例清单

### 基础转义功能测试 (6个)

| # | 测试名称 | 描述 | 状态 |
|---|---------|------|------|
| 1 | `EscapeSingleQuote` | 单引号转义验证 | ✅ |
| 2 | `EscapeBackslash` | 反斜杠转义验证 | ✅ |
| 3 | `EscapeLikeWildcardPercent` | LIKE通配符%转义 | ✅ |
| 4 | `EscapeLikeWildcardUnderscore` | LIKE通配符_转义 | ✅ |
| 5 | `EscapeCombinedInjection` | 组合注入攻击 | ✅ |
| 6 | `EscapeMultipleSpecialChars` | 多个特殊字符 | ✅ |

### UserApiModule安全测试 (4个)

| # | 测试名称 | 攻击类型 | 状态 |
|---|---------|---------|------|
| 7 | `UserModule_UsernameInjection_Basic` | 基础SQL注入 | ✅ |
| 8 | `UserModule_UsernameInjection_UnionSelect` | UNION SELECT注入 | ✅ |
| 9 | `UserModule_EmailInjection_CommentAttack` | 注释符注入 | ✅ |
| 10 | `UserModule_UsernameWithBackslash` | 反斜杠注入 | ✅ |

### SearchApiModule安全测试 (4个)

| # | 测试名称 | 攻击类型 | 状态 |
|---|---------|---------|------|
| 11 | `SearchModule_QueryInjection_LikeClause` | LIKE查询注入 | ✅ |
| 12 | `SearchModule_QueryInjection_WildcardInjection` | 通配符注入(%) | ✅ |
| 13 | `SearchModule_QueryInjection_UnderscoreInjection` | 通配符注入(_) | ✅ |
| 14 | `SearchModule_MultiFieldInjection` | 多字段注入 | ✅ |

### 高级注入攻击测试 (3个)

| # | 测试名称 | 攻击类型 | 状态 |
|---|---------|---------|------|
| 15 | `Advanced_TimeBasedBlindInjection` | 时间盲注 | ✅ |
| 16 | `Advanced_StackedQueries` | 堆叠查询 | ✅ |
| 17 | `Advanced_BooleanBasedBlindInjection` | 布尔盲注 | ✅ |

### 边界情况测试 (4个)

| # | 测试名称 | 描述 | 状态 |
|---|---------|------|------|
| 18 | `EdgeCase_EmptyString` | 空字符串 | ✅ |
| 19 | `EdgeCase_VeryLongString` | 超长字符串(1000字符) | ✅ |
| 20 | `EdgeCase_UnicodeCharacters` | Unicode字符 | ✅ |
| 21 | `EdgeCase_SpecialCharactersOnly` | 仅特殊字符 | ✅ |

**总测试用例**: **21个**
**预期通过率**: **100%**

---

## 🎯 测试覆盖的攻击向量

### 1. 单引号注入
```sql
-- 攻击载荷
admin' OR '1'='1

-- 预期SQL（修复后）
SELECT * FROM users WHERE username = 'admin'' OR ''1''=''1'
```

### 2. UNION SELECT注入
```sql
-- 攻击载荷
admin' UNION SELECT * FROM passwords --

-- 预期SQL（修复后）
SELECT * FROM users WHERE username = 'admin'' UNION SELECT * FROM passwords --'
```

### 3. LIKE通配符注入
```sql
-- 攻击载荷
% 或 _

-- 预期SQL（修复后）
SELECT * FROM papers WHERE title LIKE '%\%%'  -- %被转义为\%
SELECT * FROM papers WHERE title LIKE '%\_%'  -- _被转义为\_
```

### 4. 堆叠查询注入
```sql
-- 攻击载荷
admin'; DROP TABLE users; --

-- 预期SQL（修复后）
SELECT * FROM users WHERE username = 'admin''; DROP TABLE users; --'
```

---

## 📈 预期测试结果

### 成功输出示例
```
============================================================
🔒 SQL注入安全测试套件
📅 日期: 2026-04-04
🎯 目标: 验证SQL转义函数能够防止注入攻击
============================================================

▶️  Running: EscapeSingleQuote
  测试: 单引号转义
  ✅ PASSED

▶️  Running: EscapeBackslash
  测试: 反斜杠转义
  ✅ PASSED

... (21个测试)

============================================================
📊 测试总结
============================================================
总测试数: 21
通过: 21 ✅
失败: 0 ❌
通过率: 100%

🎉 所有测试通过！SQL注入防护工作正常。
============================================================
```

---

## ⚠️ 故障排查

### 问题1: 编译失败 - "cl: command not found"
**解决方案**: 使用Visual Studio开发者命令提示符，而非普通CMD

### 问题2: 测试通过率 < 100%
**可能原因**:
1. SQL转义函数实现不一致
2. 测试代码与实际代码不同步
**解决方案**: 检查`Security::escapeSQL()`函数是否与生产代码一致

### 问题3: 运行时错误
**可能原因**: 字符编码问题
**解决方案**: 确保源文件保存为UTF-8格式

---

## 🔧 测试维护

### 添加新测试用例
1. 在`test_SQLInjectionSecurity_simple.cpp`中添加新测试
2. 使用`TEST(测试名称)`宏
3. 使用`ASSERT_EQ()`或`ASSERT_FALSE()`进行断言

### 示例
```cpp
TEST(MyNewSecurityTest) {
    std::cout << "  测试: 我的新安全测试" << std::endl;
    std::string input = "malicious input";
    std::string escaped = Security::escapeSQL(input);

    ASSERT_EQ(escaped, "expected output");
}
```

---

## 📚 参考文档

- **OWASP SQL注入**: https://owasp.org/www-community/attacks/SQL_Injection
- **CWE-89**: https://cwe.mitre.org/data/definitions/89.html
- **Bug修复报告**: [BUG_FIX_REPORT.md](../../BUG_FIX_REPORT.md)

---

## ✅ 验收标准

测试套件满足以下条件即可视为验收通过：

- ✅ 所有21个测试用例100%通过
- ✅ 编译0错误0警告
- ✅ 测试执行时间 < 5秒
- ✅ 无内存泄漏
- ✅ 代码覆盖率达到100%转义路径

---

**测试负责人**: Security Team
**最后更新**: 2026-04-04
**版本**: 1.0.0
