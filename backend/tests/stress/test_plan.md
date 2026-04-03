# 🔥 压力测试方案 (Stress Test Plan)

**版本**: 1.0.0
**日期**: 2026-04-04
**目的**: 模拟高并发场景，验证系统稳定性

---

## 🎯 测试目标

### 性能指标
- **并发用户**: 100+ 同时连接
- **请求成功率**: > 99%
- **错误率**: < 1%
- **系统稳定性**: 无崩溃、无内存泄漏

---

## 🧪 测试场景

### 场景1: 并发用户测试
**目标**: 验证100个并发用户下的系统表现

**测试脚本** (Python + Locust):
```python
#!/usr/bin/env python3
# stress_test_concurrent_users.py

from locust import HttpUser, task, between, events
import random

class PaperCrawlerUser(HttpUser):
    wait_time = between(1, 3)
    
    def on_start(self):
        """登录"""
        response = self.client.post("/api/users/login", json={
            "username": "test_user",
            "password": "test_password"
        })
        
    @task(3)
    def search_papers(self):
        """搜索论文（高频操作）"""
        queries = ["machine learning", "deep learning", "AI", "data science"]
        self.client.post("/api/papers/search", json={
            "query": random.choice(queries),
            "page": random.randint(1, 10),
            "limit": 20
        })
    
    @task(2)
    def get_paper_details(self):
        """查看论文详情（中频操作）"""
        paper_id = random.randint(1, 1000)
        self.client.get(f"/api/papers/{paper_id}")
    
    @task(1)
    def get_stats(self):
        """查看统计信息（低频操作）"""
        self.client.get("/api/stats")
    
    @task(1)
    def advanced_search(self):
        """高级搜索（中频操作）"""
        self.client.post("/api/papers/advanced-search", json={
            "query": "test",
            "year_from": 2020,
            "year_to": 2024,
            "page": 1,
            "limit": 10
        })

class WebsiteUser(HttpUser):
    """普通网站用户（低流量）"""
    wait_time = between(5, 10)
    
    @task
    def view_homepage(self):
        self.client.get("/")
    
    @task
    def view_about(self):
        self.client.get("/about")
```

**运行压力测试**:
```bash
# 安装Locust
pip install locust

# 运行测试
locust -f stress_test_concurrent_users.py --host=http://localhost:8080 --users=100 --spawn-rate=10 --run-time=5m
```

**测试参数**:
- `--users=100`: 模拟100个并发用户
- `--spawn-rate=10`: 每秒增加10个用户
- `--run-time=5m`: 持续5分钟

**预期结果**:
```
Name                                                          # reqs      # fails | Avg     Min     Max    | Median | req/s | fail/s
---------------------------------------------------------------------------------------------------------------------------------------
Aggregated                                                        30000      0     (0.00%) | 45      20      120    | 42     |   100 | 0.00
---------------------------------------------------------------------------------------------------------------------------------------

Response time percentiles (approximated)
Type     Name                                                         50%    66%    75%    80%    90%    95%    98%    99%    99.9%  99.99%
----------------------------------------------------------------------------------------------------------------------------------------
---------+-----------------------------------------------------------+-----+-----+-----+-----+-----+-----+------+-------+-------
GET      /api/papers/{id}                                           40    45    50    55    65    75    85    95     98     100
POST     /api/papers/search                                       50    55    60    70    80    95    110   125    140    150
```

**成功标准**:
- ✅ 请求成功率 > 99%
- ✅ 平均响应时间 < 100ms
- ✅ 无错误或崩溃
- ✅ 无内存泄漏

---

### 场景2: 峰值流量测试
**目标**: 验证系统在瞬时高负载下的表现

**测试脚本**:
```python
#!/usr/bin/env python3
# stress_test_spike.py

import requests
import threading
import time

class SpikeTest:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url
        self.results = []
    
    def send_request(self, thread_id):
        """发送单个请求"""
        start = time.time()
        try:
            response = requests.post(
                f"{self.base_url}/api/papers/search",
                json={"query": f"test_{thread_id}", "page": 1, "limit": 10},
                timeout=10
            )
            end = time.time()
            
            self.results.append({
                "thread_id": thread_id,
                "status_code": response.status_code,
                "response_time": (end - start) * 1000,
                "success": response.status_code < 500
            })
        except Exception as e:
            self.results.append({
                "thread_id": thread_id,
                "status_code": 0,
                "response_time": 0,
                "success": False,
                "error": str(e)
            })
    
    def spike_test(self, concurrent_users=200):
        """执行峰值测试"""
        print(f"\n{'='*60}")
        print(f"峰值流量测试")
        print(f"{'='*60}")
        print(f"并发用户: {concurrent_users}")
        print(f"测试时间: 30秒")
        print()
        
        # 创建线程
        threads = []
        for i in range(concurrent_users):
            t = threading.Thread(target=self.send_request, args=(i,))
            threads.append(t)
        
        # 启动所有线程
        print("启动所有线程...")
        start_time = time.time()
        
        for t in threads:
            t.start()
        
        # 等待30秒
        time.sleep(30)
        
        # 等待所有线程完成
        for t in threads:
            t.join()
        
        end_time = time.time()
        duration = end_time - start_time
        
        # 分析结果
        total_requests = len(self.results)
        successful_requests = sum(1 for r in self.results if r["success"])
        failed_requests = total_requests - successful_requests
        
        response_times = [r["response_time"] for r in self.results if r["success"]]
        avg_response = sum(response_times) / len(response_times) if response_times else 0
        max_response = max(response_times) if response_times else 0
        
        print(f"\n{'='*60}")
        print(f"峰值测试结果")
        print(f"{'='*60}")
        print(f"总请求数: {total_requests}")
        print(f"成功请求: {successful_requests} ({successful_requests*100/total_requests:.1f}%)")
        print(f"失败请求: {failed_requests} ({failed_requests*100/total_requests:.1f}%)")
        print(f"测试时长: {duration:.1f} 秒")
        print(f"QPS: {total_requests/duration:.1f}")
        print(f"平均响应时间: {avg_response:.2f} ms")
        print(f"最大响应时间: {max_response:.2f} ms")
        
        # 评估
        success_rate = successful_requests / total_requests
        if success_rate > 0.99:
            print(f"\n✓ 系统稳定性: 优秀 (成功率 {success_rate*100:.1f}%)")
        elif success_rate > 0.95:
            print(f"\n⚠ 系统稳定性: 良好 (成功率 {success_rate*100:.1f}%)")
        else:
            print(f"\n❌ 系统稳定性: 需优化 (成功率 {success_rate*100:.1f}%)")

if __name__ == "__main__":
    test = SpikeTest()
    test.spike_test(concurrent_users=200)
```

**运行**:
```bash
python stress_test_spike.py
```

---

### 场景3: SQL注入压力测试
**目标**: 验证SQL转义在高负载下的正确性

**测试脚本**:
```python
#!/usr/bin/env python3
# stress_test_sql_injection.py

import requests
import threading
import time

# SQL注入攻击载荷（常见攻击模式）
attack_payloads = [
    "admin' OR '1'='1",
    "admin' UNION SELECT * FROM users --",
    "admin'; DROP TABLE users; --",
    "admin' AND (SELECT SLEEP(10)) --",
    "%",  # LIKE通配符
    "_",  # LIKE通配符
    "admin\\' OR 1=1 --",
    "'\\%_'",
]

def test_sql_injection_resistance():
    """测试SQL注入防护在高并发下的表现"""
    print("\n" + "="*60)
    print("SQL注入压力测试")
    print("="*60)
    
    base_url = "http://localhost:8080"
    
    for payload in attack_payloads:
        print(f"\n测试载荷: {payload[:30]}...")
        
        # 发送100个并发请求
        threads = []
        results = []
        
        def send_request(thread_id):
            try:
                response = requests.post(
                    f"{base_url}/api/users/login",
                    json={"username": payload, "password": "test"},
                    timeout=10
                )
                results.append({
                    "status_code": response.status_code,
                    "success": response.status_code != 500 and 
                              "success" not in response.text.lower()
                })
            except Exception as e:
                results.append({
                    "status_code": 0,
                    "success": False
                })
        
        # 启动100个并发线程
        for i in range(100):
            t = threading.Thread(target=send_request, args=(i,))
            t.start()
            threads.append(t)
        
        # 等待完成
        for t in threads:
            t.join()
        
        # 分析结果
        successful = sum(1 for r in results if r["success"])
        failed = len(results) - successful
        
        print(f"  并发请求数: 100")
        print(f"  成功: {successful}")
        print(f"  失败: {failed}")
        print(f"  成功率: {successful}%")
        
        if successful == 100:
            print(f"  结果: ✓ 所有攻击被成功防护")
        elif successful > 95:
            print(f"  结果: ⚠ 大部分攻击被防护")
        else:
            print(f"  结果: ❌ 防护可能存在漏洞")

if __name__ == "__main__":
    test_sql_injection_resistance()
```

**运行**:
```bash
python stress_test_sql_injection.py
```

---

### 场景4: 长时间稳定性测试
**目标**: 验证系统长时间运行稳定性

**测试方案**:
```python
# stress_test_endurance.py

import requests
import time
import psutil

def endurance_test(duration_minutes=30):
    """长时间稳定性测试"""
    print("\n" + "="*60)
    print(f"长时间稳定性测试 ({duration_minutes}分钟)")
    print("="*60)
    
    base_url = "http://localhost:8080"
    start_time = time.time()
    end_time = start_time + duration_minutes * 60
    
    request_count = 0
    error_count = 0
    
    # 监控内存
    server_process = None
    for proc in psutil.process_iter(['name', 'pid', 'memory_info']):
        if 'PaperCrawlerServer' in proc.info['name']:
            server_process = psutil.Process(proc.info['pid'])
            break
    
    initial_memory = server_process.memory_info().rss if server_process else 0
    
    print(f"开始时间: {time.ctime(start_time)}")
    print(f"结束时间: {time.ctime(end_time)}")
    print(f"初始内存: {initial_memory / 1024 / 1024:.2f} MB")
    print(f"\n监控中...")
    
    while time.time() < end_time:
        try:
            # 发送各种请求
            requests.get(f"{base_url}/api/health", timeout=5)
            requests.post(f"{base_url}/api/papers/search", 
                          json={"query": "test"}, timeout=5)
            
            request_count += 2
            
            # 每10秒报告一次状态
            if request_count % 20 == 0:
                if server_process:
                    current_memory = server_process.memory_info().rss
                    memory_growth = (current_memory - initial_memory) / 1024 / 1024
                    elapsed = int((time.time() - start_time) / 60)
                    remaining = duration_minutes - elapsed
                    
                    print(f"[{elapsed:02d}m/{duration_minutes:02d}m] "
                          f"请求数: {request_count}, "
                          f"错误数: {error_count}, "
                          f"内存增长: {memory_growth:.2f} MB")
        
        except Exception as e:
            error_count += 1
            print(f"  错误: {e}")
    
    total_time = time.time() - start_time
    final_memory = server_process.memory_info().rss if server_process else initial_memory
    
    print(f"\n{'='*60}")
    print(f"稳定性测试结果")
    print(f"{'='*60}")
    print(f"总运行时长: {total_time / 60:.1f} 分钟")
    print(f"总请求数: {request_count}")
    print(f"错误数: {error_count}")
    print(f"错误率: {error_count / request_count * 100:.2f}%")
    print(f"内存增长: {(final_memory - initial_memory) / 1024 / 1024:.2f} MB")
    
    # 评估
    if error_count == 0 and (final_memory - initial_memory) < 50 * 1024 * 1024:
        print(f"\n✓ 系统稳定性: 优秀")
    elif error_count < request_count * 0.01:
        print(f"\n✓ 系统稳定性: 良好")
    else:
        print(f"\n⚠ 系统稳定性: 需要优化")

if __name__ == "__main__":
    endurance_test(duration_minutes=30)
```

**运行**:
```bash
python stress_test_endurance.py
```

---

## ✅ 压力测试验收标准

| 场景 | 指标 | 目标 | 预期 |
|------|------|------|------|
| 并发用户 | 并发数 | 100+ | ⏳ |
| 成功率 | 请求成功率 | > 99% | ⏳ |
| 峰值流量 | QPS | > 100 | ⏳ |
| SQL注入防护 | 攻击成功率 | 0% | ⏳ |
| 稳定性 | 长时间运行 | 无崩溃 | ⏳ |
| 内存 | 30分钟增长 | < 50MB | ⏳ |

---

## 🚀 快速执行

**一键压力测试**:
```batch
@echo off
echo ====================================
echo 压力测试套件
echo ====================================
echo.

echo [1/4] 并发用户测试...
echo 启动Locust Web界面: http://localhost:8080
echo 然后运行: locust -f stress_test_concurrent_users.py --users=100
pause

echo [2/4] 峰值流量测试...
python stress_test_spike.py
pause

echo [3/4] SQL注入压力测试...
python stress_test_sql_injection.py
pause

echo [4/4] 长时间稳定性测试...
echo 运行30分钟，监控内存和错误
python stress_test_endurance.py
pause

echo.
echo 压力测试完成！
pause
```

---

## 📊 测试报告模板

**压力测试报告**:
```
系统: PaperCrawler v1.0.0
测试日期: 2026-04-04
测试环境: Windows Server 2022, MySQL 8.0

1. 并发用户测试
   - 并发数: 100
   - 持续时间: 5分钟
   - 总请求数: 30,000
   - 成功率: 99.8%
   - 平均响应时间: 45ms
   - P95响应时间: 95ms
   - 结论: ✓ 通过

2. 峰值流量测试
   - 峰值并发: 200
   - 持续时间: 30秒
   - 总请求数: 200
   - 成功率: 100%
   - 平均响应时间: 52ms
   - 结论: ✓ 通过

3. SQL注入压力测试
   - 攻击载荷: 8种
   - 每种载荷并发: 100
   - 防护成功率: 100%
   - 结论: ✓ 通过

4. 长时间稳定性测试
   - 测试时长: 30分钟
   - 总请求数: ~500
   - 错误数: 0
   - 内存增长: 12 MB
   - 结论: ✓ 通过

总体评价: ✓ 系统在所有压力测试中表现优秀
```

---

**创建日期**: 2026-04-04
**测试负责人**: Performance Team
**测试工具**: Locust, Python, PowerShell
