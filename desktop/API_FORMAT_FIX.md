# API 数据格式适配修复

## 🔧 问题分析

### 后端实际返回格式

后端API返回的JSON格式与预期不同：

**实际格式**:
```json
{
  "papers": [
    {
      "id": 1,
      "title": "Paper 1: Deep Learning for deep",
      "journal": "CVPR 2024",
      "year": "2024",
      "level": "A"
    }
  ],
  "total": 10
}
```

**之前期望的格式**:
```json
{
  "success": true,
  "data": {
    "papers": [...],
    "pagination": {
      "total": 10,
      "offset": 0,
      "limit": 20
    }
  }
}
```

## ✅ 修复内容

### 1. 搜索结果解析 (`onSearchReply`)

**修改前**:
```cpp
ApiResponse response = parseResponse(reply);
result.total = response.data["pagination"].toObject()["total"].toInt();
QJsonArray papersArray = response.data["papers"].toArray();
```

**修改后**:
```cpp
QJsonObject json = doc.object();
QJsonArray papersArray = json["papers"].toArray();
result.total = json["total"].toInt(papersArray.size());
```

### 2. 健康检查解析 (`onHealthCheckReply`)

**修改前**:
```cpp
QString status = response.data["status"].toString();
bool isHealthy = (status == "healthy");
```

**修改后**:
```cpp
QString status = json["status"].toString();
bool isHealthy = (status == "ok" || status == "healthy");
```

### 3. 论文数据解析 (`ApiPaper::fromJson`)

**字段映射**:
```cpp
// 后端字段 -> ApiPaper字段
"id" -> id
"title" -> title
"journal" -> journalFull, journalShort (相同)
"year" -> year
"level" -> level
```

### 4. 简化解析逻辑

移除了 `parseResponse()` 方法的使用，直接解析JSON：

**优点**:
- 减少中间层
- 更清晰的错误处理
- 直接适配后端格式

## 📊 当前API端点状态

| 端点 | 格式 | 状态 |
|------|------|------|
| `GET /health` | `{"status":"ok",...}` | ✅ 已适配 |
| `GET /api/search` | `{"papers":[...],"total":N}` | ✅ 已适配 |
| `GET /api/papers/{id}` | `{...paper fields...}` | ✅ 已适配 |
| `GET /api/papers/recent` | `{"papers":[...]}` | ✅ 已适配 |

## 🎯 测试结果

### 后端测试
```bash
$ curl http://localhost:8080/health
{"status":"ok","service":"PaperCrawler API",...}

$ curl "http://localhost:8080/api/search?q=deep"
{"papers":[...],"total":10}
```

### 桌面客户端
- ✅ 启动时检查健康状态
- ✅ 搜索功能正常
- ✅ 显示真实论文数据
- ✅ 错误处理完善

## 🔄 数据流程

```
用户输入 "deep learning"
    ↓
ApiManager::searchPapers("deep learning")
    ↓
HTTP: GET /api/search?q=deep+learning
    ↓
后端返回: {"papers":[...],"total":10}
    ↓
ApiManager::onSearchReply()
    ↓
解析JSON:
  - papers数组 -> QList<ApiPaper>
  - total -> result.total
    ↓
MainWindow::onSearchSuccess(result)
    ↓
转换: ApiPaper -> Paper
    ↓
PaperCardView::setPapers(papers)
    ↓
显示卡片列表
```

## 📝 后续优化建议

### 1. 后端改进
建议后端统一响应格式：
```json
{
  "success": true,
  "data": {...},
  "message": "",
  "timestamp": 1234567890
}
```

### 2. 添加更多字段
- 作者信息
- DOI链接
- 期刊URL
- 论文摘要

### 3. 分页支持
- offset/limit参数
- 总数统计
- 页面导航

### 4. 缓存机制
- 本地缓存搜索结果
- 减少API调用
- 离线模式

## ✨ 现在可以正常使用了！

1. **启动后端** - 确保运行在 http://localhost:8080
2. **启动桌面客户端** - 会自动检查健康状态
3. **搜索论文** - 输入关键词如 "deep learning"
4. **查看结果** - 显示真实的论文数据

---

**修复时间**: 2026-03-22
**状态**: ✅ 已修复并测试通过
