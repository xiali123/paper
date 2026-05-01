# PaperCrawler API 测试框架

数据驱动的路由测试框架，自动从源码提取路由，与测试数据同步，一键运行全量测试。

---

## 目录结构

```
tests/
├── run_all_tests.sh           # 全量测试入口（支持 --ci）
├── run_module_test.sh         # 单模块测试入口
├── lib/
│   ├── test_framework.sh      # 测试引擎 v3
│   ├── auth_helpers.sh        # 认证辅助（token获取）
│   └── test_utils.sh          # 通用工具
├── routes/                    # 路由数据文件（每模块一个）
│   ├── admin_routes.sh        #   AdminApi (105)
│   ├── aicopilot_routes.sh    #   AiCoPilot (11)
│   ├── ai_routes.sh           #   AiApi (6)
│   ├── analytics_routes.sh    #   AnalyticsIntelligence (3)
│   ├── auth_routes.sh         #   AuthApi (15)
│   ├── collaborative_routes.sh#   CollaborativeWriting (15)
│   ├── core_routes.sh         #   Core (5)
│   ├── crawler_routes.sh      #   CrawlerApi (19)
│   ├── export_routes.sh       #   ExportApi (4)
│   ├── latex_routes.sh        #   LatexApi (33)
│   ├── paper_routes.sh        #   PaperApi (19)
│   ├── recommendation_routes.sh#  RecommendationApi (6)
│   ├── search_routes.sh       #   SearchApi (12)
│   ├── stats_routes.sh        #   StatsApi (6)
│   ├── user_routes.sh         #   UserApi (12)
│   └── fixtures/              #   复杂请求体JSON
│       ├── paper_complex.json
│       └── crawler_template.json
├── scripts/                   # 自动同步工具
│   ├── extract_routes.sh      #   从C++源码提取路由
│   └── sync_routes.sh         #   源码↔测试对比+同步
├── archive/                   # 旧脚本归档
└── reports/                   # 测试报告输出
```

**覆盖**: 15模块 / 271条路由 / 253条源码路由

---

## 快速开始

```bash
# 1. 启动服务器
cd backend/build/Release
./PaperCrawlerServerHotPlug ../../config/modules_auto.json

# 2. 运行全量测试
cd backend
bash tests/run_all_tests.sh

# 3. CI模式（自动同步 + 测试 + 报告）
bash tests/run_all_tests.sh --ci
```

---

## 命令参考

### run_all_tests.sh — 全量测试

```bash
bash tests/run_all_tests.sh [选项]
```

| 选项 | 简写 | 说明 |
|------|------|------|
| `--ci` | | CI模式: 自动sync + retry=2 + JUnit + JSON + perf=3000ms |
| `--sync` | `-s` | 测试前先从源码同步路由 |
| `--sync-dry-run` | | 只显示sync差异，不修改文件 |
| `--module <name>` | `-m` | 只跑指定模块 |
| `--filter <regex>` | `-f` | 过滤测试（匹配 method+path+name） |
| `--dry-run` | `-d` | 预览模式，不实际请求 |
| `--verbose` | `-v` | 失败时显示response body |
| `--retry <n>` | `-r` | 失败重试次数 |
| `--timeout <sec>` | `-t` | 请求超时（默认10秒） |
| `--base-url <url>` | | 覆盖 BASE_URL |
| `--junit <file>` | `-j` | 输出JUnit XML报告 |
| `--json <file>` | | 输出JSON报告 |
| `--perf <ms>` | `-p` | 慢请求警告阈值 |
| `--list` | `-l` | 列出所有模块 |
| `--help` | `-h` | 显示帮助 |

**示例:**

```bash
# CI流水线
bash tests/run_all_tests.sh --ci

# 只测auth模块，失败重试，显示response
bash tests/run_all_tests.sh -m auth -r 1 -v

# 预览所有测试（不发请求）
bash tests/run_all_tests.sh --dry-run

# 同步源码路由后测试，输出报告
bash tests/run_all_tests.sh --sync -j results.xml --json results.json

# 只测POST请求
bash tests/run_all_tests.sh -f "POST"
```

### run_module_test.sh — 单模块测试

```bash
bash tests/run_module_test.sh <模块名> [选项]
```

选项与 `run_all_tests.sh` 相同（不含 `--module`）。

```bash
bash tests/run_module_test.sh auth
bash tests/run_module_test.sh admin -v -r 1
bash tests/run_module_test.sh paper --dry-run
```

---

## 路由数据文件格式

每个 `routes/*_routes.sh` 文件定义一个模块的测试路由：

```bash
#!/bin/bash
MODULE_NAME="StatsApi"
ROUTES=(
    "METHOD|/path|body|expected_codes|test_name|headers"
    "GET|/api/stats||200|Get stats"
    "POST|/api/papers|{\"title\":\"test\"}|200,201|Create paper"
    "GET|/api/users/1||200,404|Get user"
    "DELETE|/api/users/1||200,404|Delete user|Authorization: Bearer {{TOKEN}}"
    "POST|/api/papers|@fixtures/paper_complex.json|200|Create complex paper"
)
```

**字段说明（`|` 分隔，前4个字段严格分割）：**

| 字段 | 必填 | 说明 |
|------|------|------|
| METHOD | 是 | GET / POST / PUT / DELETE / PATCH |
| PATH | 是 | 请求路径，支持路径参数如 `/users/1` |
| BODY | | JSON字符串、`null`（空body）、`@path/file.json`（文件引用） |
| EXPECTED | | 期望HTTP状态码，逗号分隔，如 `200,201,404` |
| NAME | | 测试名称（第5个`|`之后全部作为名称） |
| HEADERS | | 额外请求头，`;` 分隔，支持 `{{VAR}}` 模板变量 |

**可选配置（在路由文件中设置）：**

```bash
# 认证测试
source lib/auth_helpers.sh
setup_auth  # 注册测试用户并获取TOKEN

# 路由元数据
ROUTE_META=(["/api/ai/chat"]="timeout=30" ["/skip"]="skip")

# 响应验证
RESPONSE_CHECKS=(["GET /api/users"]="has_field:users;not_empty")

# 自定义测试
module_custom_tests() {
    # 额外的自定义测试逻辑
}
```

---

## 自动同步工具

### extract_routes.sh — 从源码提取路由

扫描 `src/business/*.cpp` 和 `src/core/main_refactored.cpp`，提取所有 `router.get/post/put/del` 调用。

```bash
# 提取全部路由
bash scripts/extract_routes.sh

# 只提取指定模块
bash scripts/extract_routes.sh --module admin

# 输出格式
# MODULE_NAME METHOD PATH
# AdminApi get /api/admin/stats
```

### sync_routes.sh — 源码↔测试对比同步

对比源码提取的路由与测试文件中的路由，自动追加新路由。

```bash
# 查看差异（不修改文件）
bash scripts/sync_routes.sh --dry-run

# 实际同步
bash scripts/sync_routes.sh

# 只同步指定模块
bash scripts/sync_routes.sh --module admin
```

**同步行为：**
- 新增路由（源码有，测试没有）→ 自动追加到对应 `routes/*.sh`
- 过期路由（测试有，源码没有）→ 仅报告，不删除
- 新模块（源码有新模块，测试无文件）→ 自动创建路由文件
- 幂等：重复运行不重复追加

---

## 添加新路由测试

### 方式1：手动添加

编辑对应的 `routes/*_routes.sh`，在 `ROUTES` 数组末尾追加：

```bash
ROUTES+=(
    "GET|/api/new/endpoint||200|New endpoint test"
)
```

### 方式2：自动同步

源码新增路由后，运行同步：

```bash
bash scripts/sync_routes.sh        # 自动追加
bash tests/run_all_tests.sh --ci   # 或用CI模式自动处理
```

新增路由默认 `expected=200,201,404`，可手动调整。

---

## 添加新模块测试

1. 创建 `routes/<module>_routes.sh`：

```bash
#!/bin/bash
MODULE_NAME="NewModule"
ROUTES=(
    "GET|/api/new||200|List"
)
```

2. 运行测试：

```bash
bash tests/run_module_test.sh new
```

3. 或通过同步工具自动创建（如果源码已有对应模块）：

```bash
bash scripts/sync_routes.sh
```

---

## CI/CD 集成

```bash
# 完整CI流程：自动同步 → 测试 → 生成报告
bash tests/run_all_tests.sh --ci
```

`--ci` 等同于：

```bash
bash tests/run_all_tests.sh \
    --sync \
    --retry 2 \
    --junit test-results.xml \
    --json test-results.json \
    --perf 3000
```

**CI 输出文件：**
- `test-results.xml` — JUnit XML（CI平台解析）
- `test-results.json` — JSON报告（自定义处理）

**退出码：** 有模块失败则返回非零。

---

## 环境变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `BASE_URL` | `http://localhost:8080` | 服务器地址 |
| `TIMEOUT` | `10` | 请求超时（秒） |
| `VERBOSE` | `0` | 详细输出 |
| `RETRY_COUNT` | `0` | 重试次数 |
| `DRY_RUN` | `0` | 预览模式 |
| `TOKEN` | — | 认证token（由auth_helpers自动设置） |
