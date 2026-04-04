# SQL注入漏洞修复指南

**范围**: PaperCrawler后端所有数据库查询
**状态**: 🚨 关键 - 需要立即修复
**影响**: 43个文件，多处SQL注入点

---

## 目录
1. [漏洞概述](#漏洞概述)
2. [受影响文件清单](#受影响文件清单)
3. [修复方法](#修复方法)
4. [代码示例](#代码示例)
5. [验证方法](#验证方法)
6. [自动化修复脚本](#自动化修复脚本)

---

## 漏洞概述

### 风险等级
**CVSS v3.1: 9.8 (Critical)**

### 攻击场景

#### 场景1: 认证绕过
```bash
# 攻击请求
POST /api/auth/login
{
  "username": "admin' OR '1'='1",
  "password": "anything"
}

# 生成的SQL (当前代码)
SELECT password_hash FROM users WHERE username = 'admin' OR '1'='1'
# 结果: 返回第一个用户的密码哈希，绕过认证
```

#### 场景2: 数据泄露
```bash
# 攻击请求
GET /api/papers?id=1' UNION SELECT NULL,username,password,NULL,NULL,NULL FROM users--

# 生成的SQL
SELECT * FROM papers WHERE id = 1' UNION SELECT NULL,username,password,NULL,NULL,NULL FROM users--
# 结果: 泄露所有用户密码哈希
```

#### 场景3: 数据删除
```bash
# 攻击请求
DELETE /api/users?id=1; DROP TABLE users;--

# 生成的SQL
DELETE FROM users WHERE id = 1; DROP TABLE users;--
# 结果: 删除整个用户表
```

---

## 受影响文件清单

### 关键文件 (立即修复)

| 文件 | 行号 | 函数 | 风险 |
|------|------|------|------|
| `AuthApiModule.cpp` | 77 | `verifyPassword()` | 高 |
| `AuthApiModule.cpp` | 105 | `storeSession()` | 高 |
| `AuthApiModule.cpp` | 111 | `storeSession()` | 高 |
| `AuthApiModule.cpp` | 183 | `getUserByUsername()` | 高 |
| `AuthApiModule.cpp` | 217 | `createUserInDatabase()` | 高 |
| `AuthApiModule.cpp` | 312 | `handleLogin()` | 中 |
| `AuthApiModule.cpp` | 385 | `getCurrentUser()` | 高 |
| `AuthApiModule.cpp` | 442 | `changePassword()` | 高 |
| `AuthApiModule.cpp` | 458 | `changePassword()` | 高 |
| `AuthApiModule.cpp` | 471 | `initiatePasswordReset()` | 高 |
| `UserApiModule.cpp` | 92 | `getUserById()` | 高 |
| `UserApiModule.cpp` | 107 | `getUserByUsername()` | 高 |
| `UserApiModule.cpp` | 122 | `getUserByEmail()` | 高 |
| `UserApiModule.cpp` | 138 | `searchUsers()` | 高 |
| `UserApiModule.cpp` | 219 | `updateUser()` | 高 |
| `UserApiModule.cpp` | 278 | `activateUser()` | 中 |
| `UserApiModule.cpp` | 289 | `deactivateUser()` | 中 |
| `UserApiModule.cpp` | 301 | `updateUserPassword()` | 高 |
| `PaperApiModule.cpp` | 110 | `getPaperById()` | 高 |
| `PaperApiModule.cpp` | 189 | `createPaper()` | 高 |
| `PaperApiModule.cpp` | 260 | `searchPapers()` | 高 |
| `PaperApiModule.cpp` | 366 | `markPaperAsRead()` | 中 |
| `PaperApiModule.cpp` | 378 | `markPaperAsFavorite()` | 中 |

### 其他受影响文件

- `CrawlerModule.cpp` (多处)
- `AiCoPilotModule.cpp` (2处)
- `AnalyticsIntelligenceModule.cpp` (1处)
- `CollaborativeWritingEnhanced.cpp` (1处)
- `UnifiedAIWorkflow.cpp` (2处)

---

## 修复方法

### 方法1: 使用预处理语句 (推荐)

**优点**:
- 100%防止SQL注入
- 性能更好 (可重用执行计划)
- 数据库原生支持

**实现步骤**:

1. **定义预处理语句接口**
```cpp
// include/data/IDatabase.hpp
class PreparedStatement {
public:
    virtual ~PreparedStatement() = default;

    virtual void bindInt(int index, int value) = 0;
    virtual void bindString(int index, const std::string& value) = 0;
    virtual void bindLong(int index, int64_t value) = 0;
    virtual void bindDouble(int index, double value) = 0;

    virtual std::vector<QueryResult> execute() = 0;
    virtual bool executeUpdate() = 0;
};
```

2. **在IDatabase中添加prepare方法**
```cpp
// include/data/IDatabase.hpp
class IDatabase {
public:
    virtual std::shared_ptr<PreparedStatement> prepare(const std::string& sql) = 0;
    // ... 其他方法
};
```

3. **实现MySQL预处理语句**
```cpp
// src/data/MySqlConnection.cpp
class MySQLPreparedStatement : public PreparedStatement {
private:
    MYSQL_STMT* stmt_;
    std::vector<MYSQL_BIND> binds_;

public:
    MySQLPreparedStatement(MYSQL* mysql, const std::string& sql) {
        stmt_ = mysql_stmt_init(mysql);
        if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.length())) {
            throw std::runtime_error("Failed to prepare statement");
        }
    }

    ~MySQLPreparedStatement() override {
        if (stmt_) {
            mysql_stmt_close(stmt_);
        }
    }

    void bindString(int index, const std::string& value) override {
        MYSQL_BIND bind = {0};
        bind.buffer_type = MYSQL_TYPE_STRING;
        bind.buffer = (void*)value.c_str();
        bind.buffer_length = value.length();

        if (index >= binds_.size()) {
            binds_.resize(index + 1);
        }
        binds_[index - 1] = bind;

        mysql_stmt_bind_param(stmt_, binds_.data());
    }

    void bindInt(int index, int value) override {
        MYSQL_BIND bind = {0};
        bind.buffer_type = MYSQL_TYPE_LONG;
        bind.buffer = (void*)&value;

        if (index >= binds_.size()) {
            binds_.resize(index + 1);
        }
        binds_[index - 1] = bind;

        mysql_stmt_bind_param(stmt_, binds_.data());
    }

    std::vector<QueryResult> execute() override {
        if (mysql_stmt_execute(stmt_)) {
            throw std::runtime_error("Failed to execute statement");
        }

        // 处理结果...
        return results;
    }

    bool executeUpdate() override {
        if (mysql_stmt_execute(stmt_)) {
            return false;
        }
        return true;
    }
};

std::shared_ptr<PreparedStatement> MySqlConnection::prepare(const std::string& sql) {
    return std::make_shared<MySQLPreparedStatement>(mysql_, sql);
}
```

### 方法2: 输入验证 + 转义 (不推荐，仅作为临时措施)

**警告**: 此方法不如预处理语句安全，仅用于无法使用预处理语句的临时表名等场景。

```cpp
// 只在必须动态表名时使用
std::string escapeIdentifier(const std::string& identifier) {
    // 验证标识符格式
    if (!std::all_of(identifier.begin(), identifier.end(),
                     [](char c) { return std::isalnum(c) || c == '_'; })) {
        throw std::invalid_argument("Invalid identifier");
    }

    // 使用反引号转义
    return "`" + identifier + "`";
}

std::string escapeString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);

    for (char c : str) {
        switch (c) {
            case '\0': escaped += "\\0"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\\': escaped += "\\\\"; break;
            case '\'': escaped += "\\'"; break;
            case '"':  escaped += "\\\""; break;
            case '\032': escaped += "\\Z"; break;  // Ctrl+Z
            default: escaped += c; break;
        }
    }

    return escaped;
}
```

---

## 代码示例

### 示例1: AuthApiModule - verifyPassword()

#### ❌ 修复前 (不安全)
```cpp
// backend/src/business/AuthApiModule.cpp:77
bool verifyPassword(const std::string& username, const std::string& password) {
    try {
        auto sql = "SELECT password_hash FROM users WHERE username = '" + username + "'";
        auto results = database_->query(sql);

        if (!results.empty()) {
            std::string storedHash = results[0]["password_hash"];
            return !password.empty();  // 另一个漏洞!
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Password verification failed: " << e.what() << std::endl;
        return false;
    }
}
```

#### ✅ 修复后 (安全)
```cpp
bool verifyPassword(const std::string& username, const std::string& password) {
    try {
        // 使用预处理语句
        auto stmt = database_->prepare(
            "SELECT password_hash FROM users WHERE username = ?"
        );
        stmt->bindString(1, username);
        auto results = stmt->execute();

        if (results.empty()) {
            return false;
        }

        std::string storedHash = results[0]["password_hash"];

        // 使用bcrypt验证密码
        return bcrypt_checkpw(password.c_str(), storedHash.c_str()) == 0;

    } catch (const std::exception& e) {
        spdlog::error("[Auth] Password verification failed: {}", e.what());
        return false;
    }
}
```

### 示例2: AuthApiModule - storeSession()

#### ❌ 修复前 (不安全)
```cpp
// backend/src/business/AuthApiModule.cpp:105-126
bool storeSession(int userId, const std::string& accessToken,
                 const std::string& refreshToken, std::chrono::seconds expiresIn) {
    try {
        auto checkSql = "SELECT id FROM user_sessions WHERE user_id = " + std::to_string(userId);
        auto existingResults = database_->query(checkSql);

        if (!existingResults.empty()) {
            // 更新现有会话
            auto updateSql = "UPDATE user_sessions SET "
                           "access_token_hash = SHA2('" + accessToken + "', 256), "  // 注入点!
                           "refresh_token = '" + refreshToken + "', "  // 注入点!
                           "expires_at = DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                           "updated_at = NOW() "
                           "WHERE user_id = " + std::to_string(userId);
            return database_->execute(updateSql);
        } else {
            // 创建新会话
            auto insertSql = "INSERT INTO user_sessions (user_id, access_token_hash, "
                           "refresh_token, expires_at, created_at) VALUES (" +
                           std::to_string(userId) + ", "
                           "SHA2('" + accessToken + "', 256), "  // 注入点!
                           "'" + refreshToken + "', "  // 注入点!
                           "DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                           "NOW())";
            return database_->execute(insertSql);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Failed to store session: " << e.what() << std::endl;
        return false;
    }
}
```

#### ✅ 修复后 (安全)
```cpp
bool storeSession(int userId, const std::string& accessToken,
                 const std::string& refreshToken, std::chrono::seconds expiresIn) {
    try {
        // 检查现有会话
        auto checkStmt = database_->prepare(
            "SELECT id FROM user_sessions WHERE user_id = ?"
        );
        checkStmt->bindInt(1, userId);
        auto existingResults = checkStmt->execute();

        if (!existingResults.empty()) {
            // 更新现有会话
            auto updateStmt = database_->prepare(
                "UPDATE user_sessions SET "
                "access_token_hash = SHA2(?, 256), "
                "refresh_token_hash = SHA2(?, 256), "  // 哈希存储
                "expires_at = DATE_ADD(NOW(), INTERVAL ? SECOND), "
                "updated_at = NOW() "
                "WHERE user_id = ?"
            );
            updateStmt->bindString(1, accessToken);
            updateStmt->bindString(2, refreshToken);
            updateStmt->bindLong(3, expiresIn.count());
            updateStmt->bindInt(4, userId);
            return updateStmt->executeUpdate();
        } else {
            // 创建新会话
            auto insertStmt = database_->prepare(
                "INSERT INTO user_sessions (user_id, access_token_hash, "
                "refresh_token_hash, expires_at, created_at) "
                "VALUES (?, SHA2(?, 256), SHA2(?, 256), "
                "DATE_ADD(NOW(), INTERVAL ? SECOND), NOW())"
            );
            insertStmt->bindInt(1, userId);
            insertStmt->bindString(2, accessToken);
            insertStmt->bindString(3, refreshToken);
            insertStmt->bindLong(4, expiresIn.count());
            return insertStmt->executeUpdate();
        }
    } catch (const std::exception& e) {
        spdlog::error("[Auth] Failed to store session: {}", e.what());
        return false;
    }
}
```

### 示例3: UserApiModule - searchUsers()

#### ❌ 修复前 (不安全)
```cpp
// backend/src/business/UserApiModule.cpp:138-180
std::vector<User> UserApiModule::searchUsers(const std::map<std::string, std::string>& params) {
    try {
        std::string sql = "SELECT * FROM users WHERE 1=1";  // 基础查询

        // 动态构建查询 (不安全!)
        if (params.contains("username")) {
            sql += " AND username = '" + params.at("username") + "'";
        }
        if (params.contains("email")) {
            sql += " AND email = '" + params.at("email") + "'";
        }
        if (params.contains("role")) {
            sql += " AND role = '" + params.at("role") + "'";
        }
        if (params.contains("active")) {
            sql += " AND is_active = " + params.at("active");
        }

        auto results = database_->query(sql);
        // 处理结果...
    } catch (const std::exception& e) {
        std::cerr << "[UserAPI] Failed to search users: " << e.what() << std::endl;
        return {};
    }
}
```

#### ✅ 修复后 (安全)
```cpp
std::vector<User> UserApiModule::searchUsers(const std::map<std::string, std::string>& params) {
    try {
        // 构建基础查询 (使用参数占位符)
        std::string sql = "SELECT * FROM users WHERE 1=1";
        std::vector<std::string> conditions;
        std::vector<std::pair<int, std::string>> bindings;  // (index, value)
        int bindIndex = 1;

        // 安全地添加条件
        if (params.contains("username")) {
            conditions.push_back(" AND username = ?");
            bindings.push_back({bindIndex++, params.at("username")});
        }
        if (params.contains("email")) {
            conditions.push_back(" AND email = ?");
            bindings.push_back({bindIndex++, params.at("email")});
        }
        if (params.contains("role")) {
            conditions.push_back(" AND role = ?");
            bindings.push_back({bindIndex++, params.at("role")});
        }
        if (params.contains("active")) {
            conditions.push_back(" AND is_active = ?");
            bindings.push_back({bindIndex++, params.at("active")});
        }

        // 组装查询
        for (const auto& condition : conditions) {
            sql += condition;
        }

        // 准备语句并绑定参数
        auto stmt = database_->prepare(sql);
        for (const auto& [index, value] : bindings) {
            stmt->bindString(index, value);
        }

        auto results = stmt->execute();
        // 处理结果...

    } catch (const std::exception& e) {
        spdlog::error("[UserAPI] Failed to search users: {}", e.what());
        return {};
    }
}
```

### 示例4: PaperApiModule - createPaper()

#### ❌ 修复前 (不安全)
```cpp
// backend/src/business/PaperApiModule.cpp:189-230
bool createPaper(const Paper& paper) {
    try {
        // 检查重复
        auto querySql = "SELECT * FROM papers WHERE title = '" + escape(paper.title) + "'";
        auto existingPapers = database_->query(querySql);

        if (!existingPapers.empty()) {
            return false;  // 已存在
        }

        // 插入新论文
        auto sql = "INSERT INTO papers (title, authors, year, publication, abstract, "
                  "citation_count, created_at) VALUES ('" +
                  escape(paper.title) + "', '" +
                  escape(paper.authors) + "', " +
                  std::to_string(paper.year) + ", '" +
                  escape(paper.publication) + "', '" +
                  escape(paper.abstract) + "', " +
                  std::to_string(paper.citationCount) + ", NOW())";

        return database_->execute(sql);
    } catch (const std::exception& e) {
        std::cerr << "[PaperAPI] Failed to create paper: " << e.what() << std::endl;
        return false;
    }
}
```

#### ✅ 修复后 (安全)
```cpp
bool createPaper(const Paper& paper) {
    try {
        // 检查重复 (使用预处理语句)
        auto checkStmt = database_->prepare(
            "SELECT id FROM papers WHERE title = ?"
        );
        checkStmt->bindString(1, paper.title);
        auto existingPapers = checkStmt->execute();

        if (!existingPapers.empty()) {
            return false;  // 已存在
        }

        // 插入新论文 (使用预处理语句)
        auto insertStmt = database_->prepare(
            "INSERT INTO papers (title, authors, year, publication, abstract, "
            "citation_count, created_at) VALUES (?, ?, ?, ?, ?, ?, NOW())"
        );

        insertStmt->bindString(1, paper.title);
        insertStmt->bindString(2, paper.authors);
        insertStmt->bindInt(3, paper.year);
        insertStmt->bindString(4, paper.publication);
        insertStmt->bindString(5, paper.abstract);
        insertStmt->bindInt(6, paper.citationCount);

        return insertStmt->executeUpdate();

    } catch (const std::exception& e) {
        spdlog::error("[PaperAPI] Failed to create paper: {}", e.what());
        return false;
    }
}
```

---

## 验证方法

### 方法1: 自动化测试

```cpp
// tests/test_sql_injection.cpp
#include <gtest/gtest.h>

class SQLInjectionTest : public ::testing::Test {
protected:
    std::shared_ptr<IDatabase> db_;

    void SetUp() override {
        db_ = createTestDatabase();
    }
};

TEST_F(SQLInjectionTest, AuthLogin_BypassAttempt) {
    // 尝试SQL注入
    LoginRequest request;
    request.username = "admin' OR '1'='1";
    request.password = "anything";

    auto response = authApi_->login(request);

    // 应该失败
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.error, "Invalid username or password");
}

TEST_F(SQLInjectionTest, UserSearch_UnionInjection) {
    // 尝试UNION注入
    std::map<std::string, std::string> params;
    params["username"] = "test' UNION SELECT NULL,username,password,NULL,NULL,NULL FROM users--";

    auto users = userApi_->searchUsers(params);

    // 应该返回空结果 (攻击失败)
    EXPECT_TRUE(users.empty());
}

TEST_F(SQLInjectionTest, PaperSearch_BooleanInjection) {
    // 尝试布尔注入
    std::map<std::string, std::string> params;
    params["title"] = "' OR 1=1--";

    auto papers = paperApi_->searchPapers(params);

    // 应该返回空结果 (攻击失败)
    EXPECT_TRUE(papers.empty());
}

TEST_F(SQLInjectionTest, CreatePaper_StackedQuery) {
    // 尝试堆叠查询
    Paper paper;
    paper.title = "test'; DROP TABLE papers;--";
    paper.authors = "test";

    auto result = paperApi_->createPaper(paper);

    // 应该失败
    EXPECT_FALSE(result);

    // 验证papers表仍然存在
    auto stmt = db_->prepare("SHOW TABLES LIKE 'papers'");
    auto results = stmt->execute();
    EXPECT_FALSE(results.empty());
}
```

### 方法2: 手动测试

```bash
# 测试1: 认证绕过
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin'\'' OR '\''1'\''='\''1", "password": "anything"}'

# 预期结果: 失败 (不应该绕过认证)

# 测试2: UNION注入
curl -X GET "http://localhost:8080/api/papers?id=1%20UNION%20SELECT%20NULL,username,password,NULL,NULL,NULL%20FROM%20users--"

# 预期结果: 失败或返回空 (不应该泄露密码)

# 测试3: 堆叠查询
curl -X POST http://localhost:8080/api/papers \
  -H "Content-Type: application/json" \
  -d '{"title": "test'\''; DROP TABLE users;--", "authors": "test"}'

# 预期结果: 失败 (users表不应该被删除)
```

### 方法3: 使用SQLMap

```bash
# 安装SQLMap
pip install sqlmap

# 扫描登录端点
sqlmap -u "http://localhost:8080/api/auth/login" \
  --data="username=test&password=test" \
  --level=5 \
  --risk=3 \
  --batch

# 预期结果: 无SQL注入漏洞发现
```

---

## 自动化修复脚本

### 脚本1: 查找所有SQL注入点

```bash
#!/bin/bash
# find_sql_injection.sh

echo "=== PaperCrawler SQL注入扫描 ==="
echo ""

# 查找所有SQL拼接模式
echo "1. 查找直接字符串拼接..."
grep -rn "SELECT.*WHERE.*=" backend/src/ --include="*.cpp" | \
  grep -v "prepare" | \
  grep -v "PreparedStatement" > sql_injections.txt

echo "2. 查找INSERT语句..."
grep -rn "INSERT INTO.*VALUES" backend/src/ --include="*.cpp" | \
  grep -v "prepare" >> sql_injections.txt

echo "3. 查找UPDATE语句..."
grep -rn "UPDATE.*SET.*=" backend/src/ --include="*.cpp" | \
  grep -v "prepare" >> sql_injections.txt

echo "4. 查找DELETE语句..."
grep -rn "DELETE FROM" backend/src/ --include="*.cpp" | \
  grep -v "prepare" >> sql_injections.txt

# 统计
total=$(wc -l < sql_injections.txt)
echo ""
echo "=== 扫描完成 ==="
echo "发现 $total 个潜在的SQL注入点"
echo "详细结果保存在: sql_injections.txt"
echo ""

# 显示前10个
echo "前10个问题:"
head -n 10 sql_injections.txt
```

### 脚本2: 生成修复模板

```python
#!/usr/bin/env python3
# generate_fix_template.py

import re
import sys

def fix_sql_injection(filename, line_number, code):
    """生成修复建议"""

    # 模式1: SELECT ... WHERE ... = '...'
    pattern1 = r'SELECT\s+.+\s+FROM\s+\w+\s+WHERE\s+\w+\s*=\s*\'[^\']+\''

    # 模式2: INSERT INTO ... VALUES (...)
    pattern2 = r'INSERT\s+INTO\s+\w+\s*\([^)]+\)\s*VALUES\s*\([^)]+\)'

    # 模式3: UPDATE ... SET ... = ...
    pattern3 = r'UPDATE\s+\w+\s+SET\s+.+\s+WHERE'

    if re.search(pattern1, code, re.IGNORECASE):
        return generate_select_fix(code)
    elif re.search(pattern2, code, re.IGNORECASE):
        return generate_insert_fix(code)
    elif re.search(pattern3, code, re.IGNORECASE):
        return generate_update_fix(code)
    else:
        return "无法自动修复，请手动检查"

def generate_select_fix(code):
    """生成SELECT修复代码"""
    return f"""
// ❌ 修复前 (不安全)
{code}

// ✅ 修复后 (安全)
auto stmt = database_->prepare(
    "SELECT * FROM table WHERE column = ?"
);
stmt->bindString(1, value);
auto results = stmt->execute();
"""

def generate_insert_fix(code):
    """生成INSERT修复代码"""
    return f"""
// ❌ 修复前 (不安全)
{code}

// ✅ 修复后 (安全)
auto stmt = database_->prepare(
    "INSERT INTO table (col1, col2) VALUES (?, ?)"
);
stmt->bindString(1, value1);
stmt->bindString(2, value2);
stmt->executeUpdate();
"""

def generate_update_fix(code):
    """生成UPDATE修复代码"""
    return f"""
// ❌ 修复前 (不安全)
{code}

// ✅ 修复后 (安全)
auto stmt = database_->prepare(
    "UPDATE table SET col1 = ?, col2 = ? WHERE id = ?"
);
stmt->bindString(1, value1);
stmt->bindString(2, value2);
stmt->bindInt(3, id);
stmt->executeUpdate();
"""

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 generate_fix_template.py <filename> <line_number>")
        sys.exit(1)

    filename = sys.argv[1]
    line_number = int(sys.argv[2])

    with open(filename, 'r') as f:
        lines = f.readlines()
        code = lines[line_number - 1].strip()

    fix = fix_sql_injection(filename, line_number, code)
    print(fix)
```

### 脚本3: 批量修复检查

```bash
#!/bin/bash
# verify_sql_fixes.sh

echo "=== 验证SQL注入修复 ==="
echo ""

# 检查是否还有未修复的SQL拼接
echo "1. 检查是否还有字符串拼接..."
injections=$(grep -r "SELECT.*WHERE.*=" backend/src/ --include="*.cpp" | \
            grep -v "prepare" | \
            grep -v "PreparedStatement" | \
            wc -l)

if [ $injections -eq 0 ]; then
    echo "   ✅ 无SQL注入点"
else
    echo "   ❌ 仍有 $injections 个SQL注入点"
fi

echo ""
echo "2. 检查是否使用了预处理语句..."
prepares=$(grep -r "prepare(" backend/src/ --include="*.cpp" | wc -l)
echo "   使用预处理语句: $prepares 处"

echo ""
echo "3. 运行单元测试..."
# cd build && ctest --output-on-failure

echo ""
echo "4. 运行SQLMap扫描..."
# sqlmap -u http://localhost:8080/api/auth/login --batch

echo ""
echo "=== 验证完成 ==="
```

---

## 检查清单

### 修复前检查
- [ ] 备份受影响的文件
- [ ] 创建测试用例
- [ ] 确认数据库连接池支持预处理语句
- [ ] 准备回滚计划

### 修复中检查
- [ ] 使用预处理语句替换所有字符串拼接
- [ ] 验证参数绑定正确
- [ ] 确保错误处理完善
- [ ] 添加日志记录

### 修复后验证
- [ ] 运行单元测试
- [ ] 运行集成测试
- [ ] 手动测试所有API端点
- [ ] 使用SQLMap扫描
- [ ] 代码审查

---

## 时间表

| 任务 | 预计时间 | 负责人 | 状态 |
|------|---------|--------|------|
| 实现预处理语句接口 | 4小时 | 后端团队 | ⏳ |
| 修复AuthApiModule | 4小时 | 后端团队 | ⏳ |
| 修复UserApiModule | 3小时 | 后端团队 | ⏳ |
| 修复PaperApiModule | 3小时 | 后端团队 | ⏳ |
| 修复其他模块 | 4小时 | 后端团队 | ⏳ |
| 编写单元测试 | 4小时 | QA团队 | ⏳ |
| 安全测试 | 2小时 | 安全团队 | ⏳ |
| **总计** | **24小时** | - | - |

---

## 参考资料

- [OWASP SQL Injection](https://owasp.org/www-community/attacks/SQL_Injection)
- [CWE-89: SQL Injection](https://cwe.mitre.org/data/definitions/89.html)
- [MySQL Prepared Statements](https://dev.mysql.com/doc/c-api/8.0/en/c-api-prepared-statements.html)
- [OWASP SQL Injection Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html)

---

**记住**: SQL注入是最常见的Web应用漏洞之一，也是最容易预防的。

**使用预处理语句，零SQL注入！**
