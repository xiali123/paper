# PaperCrawler 后端 API 测试套件

**目录**: `backend/tests/`
**创建时间**: 2026-04-04
**分支**: `test/all-modules-api-validation`

---

## 📋 目录结构

```
backend/tests/
├── api/
│   ├── run_all_tests.sh          # 主测试执行脚本 ⭐
│   ├── test_crawler_api.sh      # CrawlerApi测试 (29个端点)
│   ├── test_auth_api.sh          # AuthApi测试
│   ├── test_user_api.sh          # UserApi测试
│   ├── test_paper_api.sh         # PaperApi测试
│   ├── test_search_api.sh        # SearchApi测试
│   ├── test_export_api.sh        # ExportApi测试
│   ├── test_stats_api.sh         # StatsApi测试
│   ├── test_ai_api.sh            # AiApi测试
│   └── test_recommendation_api.sh # RecommendationApi测试
├── lib/
│   └── test_utils.sh             # 测试工具库
└── reports/
    ├── all_modules_test_report.json     # 综合测试报告
    ├── CrawlerApi_results.json          # CrawlerApi详细报告
    ├── AuthApi_results.json              # AuthApi详细报告
    ├── UserApi_results.json              # UserApi详细报告
    ├── PaperApi_results.json             # PaperApi详细报告
    ├── SearchApi_results.json            # SearchApi详细报告
    ├── ExportApi_results.json            # ExportApi详细报告
    ├── StatsApi_results.json             # StatsApi详细报告
    ├── AiApi_results.json                # AiApi详细报告
    └── RecommendationApi_results.json     # RecommendationApi详细报告
```

---

## 🚀 快速开始

### 1. 启动服务器

```bash
cd backend/build/Release
./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json
```

### 2. 运行所有测试

```bash
cd backend/tests
./api/run_all_tests.sh
```

### 3. 查看测试结果

```bash
# 查看综合报告
cat backend/tests/reports/all_modules_test_report.json | jq

# 查看特定模块报告
cat backend/tests/reports/CrawlerApi_results.json | jq
```

---

## 📊 测试覆盖范围

### CrawlerApi (29个端点 + 1个WebSocket)

**模板管理** (9个端点):
- ✅ GET /api/crawler/templates
- ✅ POST /api/crawler/templates
- ✅ GET /api/crawler/templates/:id
- ✅ PUT /api/crawler/templates/:id
- ✅ DELETE /api/crawler/templates/:id
- ✅ POST /api/crawler/templates/validate
- ✅ POST /api/crawler/templates/:id/test
- ✅ GET /api/crawler/templates/:id/export
- ✅ POST /api/crawler/templates/import

**任务管理** (7个端点):
- ✅ POST /api/crawler/tasks
- ✅ GET /api/crawler/tasks
- ✅ GET /api/crawler/tasks/:id
- ✅ DELETE /api/crawler/tasks/:id
- ✅ POST /api/crawler/tasks/:id/retry
- ✅ GET /api/crawler/tasks/:id/logs
- ✅ GET /api/crawler/tasks/statistics

**定时任务** (7个端点):
- ✅ POST /api/crawler/schedules
- ✅ GET /api/crawler/schedules
- ✅ PUT /api/crawler/schedules/:id
- ✅ DELETE /api/crawler/schedules/:id
- ✅ POST /api/crawler/schedules/:id/enable
- ✅ POST /api/crawler/schedules/:id/disable
- ✅ POST /api/crawler/schedules/:id/trigger

**工作节点** (4个端点):
- ✅ GET /api/crawler/workers
- ✅ GET /api/crawler/workers/:id
- ✅ POST /api/crawler/workers/:id/disable
- ✅ GET /api/crawler/workers/:id/statistics

**系统统计** (2个端点):
- ✅ GET /api/crawler/dashboard
- ✅ GET /api/crawler/statistics

**WebSocket** (1个端点):
- ⚠️ WS /api/crawler/ws (需要专门的WebSocket测试工具)

### 其他模块

| 模块 | 测试端点数 | 主要功能 |
|------|------------|----------|
| AuthApi | 6 | 用户认证、授权、profile管理 |
| UserApi | 5 | 用户CRUD操作 |
| PaperApi | 4 | 论文搜索、统计 |
| SearchApi | 2 | 高级搜索 |
| ExportApi | 3 | 数据导出 |
| StatsApi | 4 | 统计数据 |
| AiApi | 4 | AI聊天、摘要、关键词提取 |
| RecommendationApi | 3 | 论文推荐 |

**总测试端点**: 29 + 6 + 5 + 4 + 2 + 3 + 4 + 4 + 3 = **60**

---

## 📈 测试报告格式

### 综合报告 (`all_modules_test_report.json`)

```json
{
  "test_run": {
    "timestamp": "2026-04-04T10:45:00Z",
    "duration_seconds": 45,
    "branch": "test/all-modules-api-validation",
    "commit": "abc123..."
  },
  "summary": {
    "total_modules": 9,
    "total_tests": 60,
    "passed": 58,
    "failed": 2,
    "success_rate": 96.67
  },
  "modules": [
    {
      "name": "CrawlerApi",
      "status": "pass",
      "total": 29,
      "passed": 29,
      "failed": 0
    }
  ]
}
```

---

## 🎯 预期测试结果

基于Router.dll方案的实施，预期结果：

### 最佳情况 ✅

- **总测试数**: 60+
- **通过率**: 95%+
- **失败**: 主要是业务逻辑错误（数据库未连接等），不是路由错误

### 可接受的失败 ⚠️

- **401 Unauthorized**: 认证相关端点（需要先登录）
- **404 with message**: 业务逻辑未完全实现
- **500**: 依赖服务未启动

### 不可接受的失败 ❌

- **"Route not found"**: 路由未注册（应该已修复）
- **连接拒绝**: 服务器未启动
- **超时**: 服务器崩溃

---

## 🛠️ 高级用法

### 测试单个模块

```bash
cd backend/tests/api

# 测试CrawlerApi
./test_crawler_api.sh

# 测试AuthApi
./test_auth_api.sh
```

### 持续监控

```bash
# 定期运行测试并记录历史
while true; do
    ./run_all_tests.sh
    sleep 300  # 每5分钟测试一次
done
```

---

## 📝 维护指南

### 添加新测试

1. 在对应的模块测试脚本中添加 `test_endpoint()` 调用
2. 更新测试总数计数
3. 运行测试验证

### 添加新模块测试

1. 创建 `test_{ModuleName}_api.sh`
2. 使用 `test_utils.sh` 中的函数
3. 在 `run_all_tests.sh` 中添加到 `MODULES` 数组

---

**测试套件版本**: 1.0.0
**最后更新**: 2026-04-04
**维护者**: Claude Code (Sonnet 4.6)

---

## 🚀 快速命令参考

```bash
# 运行所有测试
cd backend/tests && ./api/run_all_tests.sh

# 查看结果
cat backend/tests/reports/all_modules_test_report.json | jq '.summary'

# 查看失败详情
cat backend/tests/reports/all_modules_test_report.json | jq '.modules[] | select(.status == "fail")'

# 统计通过率
cat backend/tests/reports/all_modules_test_report.json | jq '.summary.success_rate'
```
