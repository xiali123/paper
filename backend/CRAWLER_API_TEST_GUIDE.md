# CrawlerApiModule API测试指南

## 📋 测试概述

本测试套件验证CrawlerApiModule的所有32个API端点功能。

### 测试覆盖范围

| 类别 | API数量 | 测试数量 |
|------|---------|----------|
| 模板管理 | 8 | 8 |
| 任务管理 | 6 | 6 |
| 定时任务 | 7 | 7 |
| 工作节点 | 4 | 4 |
| 统计接口 | 2 | 2 |
| 任务统计 | 1 | 1 |
| WebSocket | 5 | 0 (手动测试) |
| **总计** | **33** | **28** |

### 注意事项

- ⚠️ WebSocket功能需要手动测试（自动化测试工具支持有限）
- ⚠️ 部分API需要数据库中存在相应数据（如templates, workers等）
- ⚠️ 测试前确保服务器正在运行（`http://localhost:8080`）

## 🚀 快速开始

### 1. 启动服务器

```bash
cd e:/PaperCrawler/backend/build/Release
./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json
```

### 2. 运行自动化测试

**Linux/Git Bash:**
```bash
cd e:/PaperCrawler/backend
bash test_crawler_api.sh
```

**Windows PowerShell:**
```powershell
cd e:\PaperCrawler\backend
.\test_crawler_api.sh
```

### 3. 查看测试结果

测试脚本会输出每个API的测试结果：
- ✅ **PASSED** (绿色) - HTTP状态码 2xx
- ❌ **FAILED** (红色) - HTTP状态码非 2xx

示例输出：
```
[1] 创建模板 ... PASSED (HTTP 200)
[2] 列出模板 ... PASSED (HTTP 200)
...
======================================
测试结果汇总
======================================
总测试数: 28
通过: 26
失败: 2
```

## 🧪 手动测试指南

### WebSocket功能测试

WebSocket功能无法用curl测试，请使用以下方法：

#### 方法1: 使用WebSocket客户端工具

推荐工具：
- **wscat** (命令行)
  ```bash
  npm install -g wscat
  wscat -c ws://localhost:8080/api/crawler/ws
  ```

- **WebSocket Test Client** (浏览器扩展)
  - Chrome: WebSocket Test Client
  - Firefox: WebSocket Tester

#### 方法2: 使用浏览器JavaScript

打开浏览器控制台（F12），运行：

```javascript
const ws = new WebSocket('ws://localhost:8080/api/crawler/ws');

ws.onopen = () => {
  console.log('WebSocket connected');

  // 发送工作节点注册消息
  ws.send(JSON.stringify({
    type: 'worker_register',
    workerId: 'test_worker_001',
    workerType: 'BROWSER',
    maxTasks: 5
  }));

  // 发送心跳消息
  setTimeout(() => {
    ws.send(JSON.stringify({
      type: 'heartbeat',
      workerId: 'test_worker_001',
      currentTasks: 2,
      status: 'ONLINE'
    }));
  }, 2000);

  // 发送任务结果
  setTimeout(() => {
    ws.send(JSON.stringify({
      type: 'task_result',
      taskId: 'task_123',
      status: 'COMPLETED',
      papersFound: 42
    }));
  }, 4000);
};

ws.onmessage = (event) => {
  console.log('Received:', event.data);
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

ws.onclose = () => {
  console.log('WebSocket disconnected');
};
```

### 预期WebSocket消息格式

**工作节点注册确认:**
```json
{
  "type": "register_confirm",
  "workerId": "test_worker_001",
  "status": "REGISTERED",
  "timestamp": "1234567890"
}
```

**心跳确认:**
```json
{
  "type": "heartbeat_ack",
  "workerId": "test_worker_001",
  "timestamp": "1234567890"
}
```

**任务结果确认:**
```json
{
  "type": "result_ack",
  "taskId": "task_123",
  "status": "RECEIVED",
  "timestamp": "1234567890"
}
```

## 📊 API端点列表

### 模板管理 (8个)

| 方法 | 路径 | 功能 |
|------|------|------|
| POST | /api/crawler/templates | 创建模板 |
| GET | /api/crawler/templates | 列出模板 |
| GET | /api/crawler/templates/:id | 获取模板详情 |
| PUT | /api/crawler/templates/:id | 更新模板 |
| DELETE | /api/crawler/templates/:id | 删除模板 |
| POST | /api/crawler/templates/validate | 验证模板 |
| POST | /api/crawler/templates/:id/test | 测试模板 |
| GET | /api/crawler/templates/:id/export | 导出模板 |

### 任务管理 (6个)

| 方法 | 路径 | 功能 |
|------|------|------|
| POST | /api/crawler/tasks | 创建任务 |
| GET | /api/crawler/tasks | 列出任务 |
| GET | /api/crawler/tasks/:id | 获取任务详情 |
| DELETE | /api/crawler/tasks/:id | 取消任务 |
| POST | /api/crawler/tasks/:id/retry | 重试任务 |
| GET | /api/crawler/tasks/:id/logs | 获取任务日志 |
| GET | /api/crawler/tasks/statistics | 获取任务统计 |

### 定时任务 (7个)

| 方法 | 路径 | 功能 |
|------|------|------|
| POST | /api/crawler/schedules | 创建定时任务 |
| GET | /api/crawler/schedules | 列出定时任务 |
| PUT | /api/crawler/schedules/:id | 更新定时任务 |
| DELETE | /api/crawler/schedules/:id | 删除定时任务 |
| POST | /api/crawler/schedules/:id/enable | 启用定时任务 |
| POST | /api/crawler/schedules/:id/disable | 禁用定时任务 |
| POST | /api/crawler/schedules/:id/trigger | 触发定时任务 |

### 工作节点 (4个)

| 方法 | 路径 | 功能 |
|------|------|------|
| GET | /api/crawler/workers | 列出工作节点 |
| GET | /api/crawler/workers/:id | 获取工作节点详情 |
| POST | /api/crawler/workers/:id/disable | 禁用工作节点 |
| GET | /api/crawler/workers/:id/statistics | 获取节点统计 |

### 统计接口 (2个)

| 方法 | 路径 | 功能 |
|------|------|------|
| GET | /api/crawler/dashboard | 获取系统仪表盘 |
| GET | /api/crawler/statistics | 获取系统统计 |

### WebSocket (1个)

| 方法 | 路径 | 功能 |
|------|------|------|
| WS | /api/crawler/ws | WebSocket实时通信 |

## 🔧 故障排除

### 问题1: 服务器未启动

**错误信息**: `Failed to connect to localhost port 8080`

**解决方案**:
```bash
# 启动服务器
cd e:/PaperCrawler/backend/build/Release
./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json
```

### 问题2: 模块未加载

**错误信息**: `404 Not Found`

**解决方案**:
1. 检查CrawlerApiModule.dll是否存在
2. 查看服务器日志，确认模块已加载
3. 确认modules_auto.json配置正确

### 问题3: 数据库错误

**错误信息**: `Database not available`

**解决方案**:
1. 确保数据库已初始化
2. 检查数据库连接配置
3. 创建必要的数据库表：
   ```sql
   -- 创建测试所需的表
   CREATE TABLE IF NOT EXISTS crawler_templates (...);
   CREATE TABLE IF NOT EXISTS distributed_crawl_tasks (...);
   CREATE TABLE IF NOT EXISTS scheduled_tasks (...);
   CREATE TABLE IF NOT EXISTS worker_nodes (...);
   ```

### 问题4: 权限错误

**错误信息**: `Permission denied`

**解决方案**:
```bash
# 给测试脚本添加执行权限
chmod +x backend/test_crawler_api.sh
```

## 📝 测试报告模板

测试完成后，可以记录测试结果：

```
日期: 2026-04-04
测试人员: [您的名字]
环境: Windows 11 / PaperCrawlerServerHotPlug.exe

测试结果:
- 自动化测试: 26/28 通过
- WebSocket测试: 5/5 通过 (手动)
- 总体通过率: 96.9%

发现的问题:
1. [问题描述]
2. [问题描述]

建议:
1. [改进建议]
2. [改进建议]
```

## 🎯 下一步

1. **运行完整测试套件** - 执行test_crawler_api.sh
2. **手动测试WebSocket** - 使用wscat或浏览器工具
3. **记录测试结果** - 填写测试报告
4. **修复发现的问题** - 根据测试结果进行调试
5. **性能测试** - 测试高并发场景
6. **集成测试** - 与其他模块联调

## 📚 相关文档

- [CrawlerApiModule实现报告](CRAWLER_API_MODULE_ANALYSIS.md)
- [API接口文档](../docs/API_DOCUMENTATION.md)
- [WebSocket测试指南](WEBSOCKET_TESTING_GUIDE.md)

---

**最后更新**: 2026-04-04
**状态**: 测试脚本已就绪
