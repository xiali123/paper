# ⚡ 性能测试方案 (Performance Test Plan)

**版本**: 1.0.0
**日期**: 2026-04-04
**目的**: 验证SQL转义修复无性能影响

---

## 📊 测试目标

### 性能基准
- **响应时间**: < 100ms (平均)
- **吞吐量**: > 100 req/s
- **SQL转义开销**: < 0.1ms per query
- **内存占用**: < 500MB (稳定运行)

---

## 🧪 测试场景

### 场景1: SQL转义性能测试
**目标**: 验证转义函数无性能瓶颈

**测试代码**:
```cpp
// benchmark_sql_escape.cpp
#include <iostream>
#include <chrono>
#include <string>
#include <vector>

// SQL转义函数（与生产代码相同）
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

void benchmark_escape() {
    std::vector<std::string> test_cases = {
        "admin",
        "admin' OR '1'='1",
        "testuser@example.com",
        "Long text with special chars: '\\%_'",
        "Very long string " + std::string(10000, 'a') + "'\\%_'"
    };
    
    std::cout << "\nSQL转义性能测试" << std::endl;
    std::cout << "==================" << std::endl;
    
    for (const auto& test_case : test_cases) {
        int iterations = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            volatile auto result = escapeSQL(test_case);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avg_time = duration.count() / (double)iterations;
        
        std::cout << "\n测试用例长度: " << test_case.length() << " 字符" << std::endl;
        std::cout << "迭代次数: " << iterations << std::endl;
        std::cout << "总耗时: " << duration.count() << " μs" << std::endl;
        std::cout << "平均耗时: " << avg_time << " μs/次" << std::endl;
        
        // 性能判断
        if (avg_time < 100) {
            std::cout << "性能评估: ✓ 优秀 (< 0.1ms)" << std::endl;
        } else if (avg_time < 1000) {
            std::cout << "性能评估: ⚠ 良好 (< 1ms)" << std::endl;
        } else {
            std::cout << "性能评估: ❌ 需优化 (> 1ms)" << std::endl;
        }
    }
}

int main() {
    benchmark_escape();
    return 0;
}
```

**编译运行**:
```batch
cl /EHsc /std:c++17 /O2 /Fe:benchmark.exe benchmark_sql_escape.cpp
benchmark.exe
```

**预期结果**:
```
SQL转义性能测试
==================

测试用例长度: 5 字符
迭代次数: 10000
总耗时: 150 μs
平均耗时: 0.015 μs/次
性能评估: ✓ 优秀 (< 0.1ms)

测试用例长度: 10007 字符
迭代次数: 10000
总耗时: 8500 μs
平均耗时: 0.85 μs/次
性能评估: ✓ 优秀 (< 0.1ms)
```

---

### 场景2: API响应时间测试
**目标**: 验证API响应时间符合要求

**测试脚本**:
```python
#!/usr/bin/env python3
# benchmark_api_response.py

import requests
import time
import statistics
import json

BASE_URL = "http://localhost:8080"

def benchmark_endpoint(endpoint, method="GET", data=None, iterations=100):
    print(f"\n{'='*60}")
    print(f"API性能测试: {endpoint}")
    print(f"{'='*60}")
    
    response_times = []
    
    for i in range(iterations):
        start = time.perf_counter()
        
        try:
            if method == "GET":
                response = requests.get(f"{BASE_URL}{endpoint}", timeout=5)
            elif method == "POST":
                response = requests.post(f"{BASE_URL}{endpoint}", json=data, timeout=5)
            
            end = time.perf_counter()
            response_time = (end - start) * 1000  # 转换为毫秒
            response_times.append(response_time)
            
        except Exception as e:
            print(f"  请求 {i+1}: 失败 ({e})")
    
    if response_times:
        avg = statistics.mean(response_times)
        median = statistics.median(response_times)
        p95 = statistics.quantiles(response_times, n=20)[18]  # 95th percentile
        p99 = statistics.quantiles(response_times, n=20)[19]  # 99th percentile
        min_time = min(response_times)
        max_time = max(response_times)
        
        print(f"\n样本数: {len(response_times)}")
        print(f"平均响应时间: {avg:.2f} ms")
        print(f"中位数: {median:.2f} ms")
        print(f"P95: {p95:.2f} ms")
        print(f"P99: {p99:.2f} ms")
        print(f"最小值: {min_time:.2f} ms")
        print(f"最大值: {max_time:.2f} ms")
        
        # 性能评估
        print(f"\n性能评估:")
        if avg < 50:
            print(f"  平均响应时间: ✓ 优秀 (< 50ms)")
        elif avg < 100:
            print(f"  平均响应时间: ✅ 良好 (< 100ms)")
        else:
            print(f"  平均响应时间: ⚠ 需关注 (> 100ms)")
            
        if p95 < 200:
            print(f"  P95响应时间: ✓ 优秀 (< 200ms)")
        else:
            print(f"  P95响应时间: ⚠ 需关注 (> 200ms)")

def run_all_benchmarks():
    print("\n" + "="*60)
    print("API性能测试套件")
    print("="*60)
    
    # 测试各端点
    benchmark_endpoint("/api/health", "GET")
    
    benchmark_endpoint("/api/papers/search", "POST", {
        "query": "test",
        "page": 1,
        "limit": 10
    }, iterations=50)
    
    print("\n" + "="*60)
    print("性能测试完成")
    print("="*60)

if __name__ == "__main__":
    run_all_benchmarks()
```

**运行**:
```bash
python benchmark_api_response.py
```

---

### 场景3: 数据库查询性能测试
**目标**: 验证SQL转义不影响查询性能

**测试SQL**:
```sql
-- 测试1: 基础查询（无转义）
SELECT * FROM papers WHERE id = 1;

-- 测试2: 查询带转义
SELECT * FROM users WHERE username = 'admin'' OR ''1''=''1';

-- 测试3: LIKE查询（含转义）
SELECT * FROM papers WHERE title LIKE '%test'' OR ''1''=''1%';

-- 性能对比
EXPLAIN SELECT * FROM papers WHERE title LIKE '%test%';
EXPLAIN SELECT * FROM papers WHERE title LIKE '%test'' OR ''1''=''1%';
```

**运行**:
```bash
mysql -u papercrawler -p papercrawler < benchmark_queries.sql
```

---

### 场景4: 内存使用测试
**目标**: 验证无内存泄漏

**测试工具**: Windows Performance Monitor 或任务管理器

**测试步骤**:
1. 启动服务器
2. 记录初始内存占用
3. 运行1000次查询
4. 记录最终内存占用
5. 对比分析

**测试脚本**:
```python
# benchmark_memory.py
import psutil
import requests
import time

def monitor_memory(duration_seconds=60):
    print("\n内存使用监控")
    print("="*60)
    
    process = psutil.Process(0)  # 当前Python进程
    server_process = None
    
    # 查找PaperCrawlerServer.exe进程
    for proc in psutil.process_iter(['name', 'pid', 'memory_info']):
        if 'PaperCrawlerServer' in proc.info['name']:
            server_process = psutil.Process(proc.info['pid'])
            break
    
    if not server_process:
        print("❌ 未找到PaperCrawlerServer.exe进程")
        return
    
    print(f"监控进程: {server_process.name()}")
    print(f"初始内存: {server_process.memory_info().rss / 1024 / 1024:.2f} MB")
    print(f"监控时长: {duration_seconds} 秒")
    print("\n开始监控...\n")
    
    start_time = time.time()
    initial_memory = server_process.memory_info().rss
    max_memory = initial_memory
    
    # 模拟负载
    for i in range(100):
        try:
            requests.post("http://localhost:8080/api/papers/search", 
                          json={"query": "test"}, timeout=5)
        except:
            pass
        
        if i % 10 == 0:
            current_memory = server_process.memory_info().rss
            max_memory = max(max_memory, current_memory)
            
            print(f"[{i+1}/100] 当前内存: {current_memory / 1024 / 1024:.2f} MB, "
                  f"峰值: {max_memory / 1024 / 1024:.2f} MB")
        
        time.sleep(0.1)
    
    final_memory = server_process.memory_info().rss
    memory_growth = final_memory - initial_memory
    
    print(f"\n{'='*60}")
    print(f"内存增长: {memory_growth / 1024 / 1024:.2f} MB")
    
    if memory_growth < 10 * 1024 * 1024:  # < 10MB
        print("评估: ✓ 内存稳定（增长 < 10MB）")
    elif memory_growth < 50 * 1024 * 1024:  # < 50MB
        print("评估: ⚠ 内存增长可接受（增长 < 50MB）")
    else:
        print("评估: ❌ 可能存在内存泄漏（增长 > 50MB）")

if __name__ == "__main__":
    monitor_memory(60)
```

---

## ✅ 性能测试验收标准

| 指标 | 目标 | 预期结果 |
|------|------|---------|
| SQL转义开销 | < 0.1ms | ⏳ 待测 |
| API平均响应时间 | < 100ms | ⏳ 待测 |
| API P95响应时间 | < 200ms | ⏳ 待测 |
| 内存增长（1000次查询） | < 10MB | ⏳ 待测 |
| 吞吐量 | > 100 req/s | ⏳ 待测 |

---

## 🚀 快速执行

**一键性能测试**:
```batch
@echo off
echo ====================================
echo 性能测试套件
echo ====================================
echo.

echo [1/4] SQL转义性能测试...
cl /EHsc /O2 benchmark_sql_escape.cpp >nul 2>&1
benchmark.exe

echo [2/4] API响应时间测试...
python benchmark_api_response.py

echo [3/4] 内存使用监控...
python benchmark_memory.py

echo [4/4] 生成性能报告...
echo 性能测试完成！

pause
```

---

**创建日期**: 2026-04-04
**测试负责人**: Performance Team
