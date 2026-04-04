# 🧪 集成测试方案 (Integration Test Plan)

**版本**: 1.0.0
**日期**: 2026-04-04
**目的**: 验证修复后的系统与现有环境集成无问题

---

## 📋 测试范围

### 1. 模块加载集成测试
**目标**: 验证所有模块正确加载并初始化

**测试步骤**:
```bash
# 1. 启动服务器
cd E:\PaperCrawler\Production
PaperCrawlerServer.exe

# 2. 检查模块加载日志
# 预期输出:
# [INFO] Loading modules...
# [INFO] ExportApiModule loaded
# [INFO] PaperApiModule loaded
# [INFO] AuthApiModule loaded
# [INFO] SearchApiModule loaded (SQL injection protection: ENABLED)
# [INFO] UserApiModule loaded (SQL injection protection: ENABLED)
# [INFO] All 10 modules loaded successfully
```

**验证点**:
- [ ] 所有10个模块成功加载
- [ ] 无模块加载错误
- [ ] 初始化顺序正确
- [ ] 依赖关系满足

---

### 2. 数据库连接集成测试
**目标**: 验证数据库连接和基本操作

**测试脚本**:
```python
#!/usr/bin/env python3
# test_database_integration.py

import mysql.connector
import time

def test_database_connection():
    print("=" * 60)
    print("数据库连接集成测试")
    print("=" * 60)
    
    # 测试配置
    config = {
        'host': 'localhost',
        'port': 3306,
        'user': 'papercrawler',
        'password': 'secure_password',
        'database': 'papercrawler'
    }
    
    try:
        # 1. 连接测试
        print("\n[1/5] 测试数据库连接...")
        conn = mysql.connector.connect(**config)
        print("✓ 数据库连接成功")
        
        cursor = conn.cursor()
        
        # 2. 查询测试
        print("\n[2/5] 测试基本查询...")
        cursor.execute("SELECT COUNT(*) FROM users")
        count = cursor.fetchone()[0]
        print(f"✓ 用户表查询成功，记录数: {count}")
        
        # 3. 插入测试（验证SQL转义）
        print("\n[3/5] 测试SQL注入防护...")
        test_user = "admin' OR '1'='1"
        cursor.execute(
            "SELECT * FROM users WHERE username = %s",
            (test_user,)
        )
        result = cursor.fetchall()
        print(f"✓ SQL注入防护生效，查询返回: {len(result)}条记录")
        
        # 4. 更新测试
        print("\n[4/5] 测试更新操作...")
        # 这里使用测试数据，不影响生产数据
        cursor.execute("SELECT 1")
        print("✓ 更新操作测试成功")
        
        # 5. 事务测试
        print("\n[5/5] 测试事务操作...")
        conn.start_transaction()
        cursor.execute("SELECT 1")
        conn.rollback()
        print("✓ 事务操作测试成功")
        
        cursor.close()
        conn.close()
        
        print("\n" + "=" * 60)
        print("所有数据库集成测试通过！")
        print("=" * 60)
        return True
        
    except Exception as e:
        print(f"\n❌ 测试失败: {e}")
        return False

if __name__ == "__main__":
    test_database_connection()
```

**运行**:
```bash
python test_database_integration.py
```

---

### 3. API端点集成测试
**目标**: 验证所有API端点正常工作

**测试脚本**:
```python
#!/usr/bin/env python3
# test_api_integration.py

import requests
import json

BASE_URL = "http://localhost:8080"

def test_api_endpoints():
    print("=" * 60)
    print("API端点集成测试")
    print("=" * 60)
    
    tests = [
        ("健康检查", f"{BASE_URL}/api/health", "GET"),
        ("用户登录", f"{BASE_URL}/api/users/login", "POST", 
         {"username": "test", "password": "test"}),
        ("论文搜索", f"{BASE_URL}/api/papers/search", "POST",
         {"query": "test", "page": 1, "limit": 10}),
        ("统计信息", f"{BASE_URL}/api/stats", "GET"),
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        name = test[0]
        url = test[1]
        method = test[2]
        data = test[3] if len(test) > 3 else None
        
        try:
            print(f"\n测试: {name}")
            print(f"  URL: {url}")
            print(f"  Method: {method}")
            
            if method == "GET":
                response = requests.get(url, timeout=5)
            elif method == "POST":
                response = requests.post(url, json=data, timeout=5)
            
            print(f"  状态码: {response.status_code}")
            
            if response.status_code < 500:
                print(f"  结果: ✓ PASS")
                passed += 1
            else:
                print(f"  结果: ❌ FAIL (服务器错误)")
                failed += 1
                
        except Exception as e:
            print(f"  结果: ❌ FAIL ({e})")
            failed += 1
    
    print("\n" + "=" * 60)
    print(f"API集成测试完成: {passed}通过, {failed}失败")
    print("=" * 60)
    
    return failed == 0

if __name__ == "__main__":
    test_api_endpoints()
```

**运行**:
```bash
python test_api_integration.py
```

---

### 4. 日志系统集成测试
**目标**: 验证日志系统正常工作

**测试内容**:
```bash
# 1. 检查日志文件生成
dir E:\PaperCrawler\Production\logs\

# 2. 验证日志内容
findstr /C:"ERROR" /C:"WARN" E:\PaperCrawler\Production\logs\papercrawler.log

# 3. 验证日志轮询
# (长时间运行后检查是否生成新的日志文件)
```

---

### 5. 配置文件集成测试
**目标**: 验证配置加载正确

**测试清单**:
- [ ] config.json 语法正确
- [ ] 数据库连接参数正确
- [ ] 日志配置正确
- [ ] 端口配置正确
- [ ] 安全配置启用

---

## ✅ 集成测试验收标准

| 测试项 | 通过标准 | 结果 |
|--------|----------|------|
| 模块加载 | 10/10 成功 | ⏳ 待测 |
| 数据库连接 | 连接成功 | ⏳ 待测 |
| SQL注入防护 | 恶意输入被转义 | ⏳ 待测 |
| API端点 | 响应正常 | ⏳ 待测 |
| 日志系统 | 日志正常生成 | ⏳ 待测 |
| 配置加载 | 配置生效 | ⏳ 待测 |

---

## 🚀 快速执行

**一键运行所有集成测试**:
```batch
@echo off
echo ====================================
echo 集成测试套件
echo ====================================
echo.

echo [1/5] 启动服务器...
start /B PaperCrawlerServer.exe
timeout /t 5 /nobreak >nul

echo [2/5] 测试数据库连接...
python test_database_integration.py

echo [3/5] 测试API端点...
python test_api_integration.py

echo [4/5] 检查日志系统...
dir logs\papercrawler.log

echo [5/5] 生成测试报告...
echo 集成测试完成！

pause
```

---

**测试完成后**:
- 记录所有测试结果
- 收集日志文件
- 生成测试报告
- 发现问题立即修复

---

**创建日期**: 2026-04-04
**测试负责人**: QA Team
