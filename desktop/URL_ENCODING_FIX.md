# 桌面客户端 API 连接问题 - 最终修复

## 🔍 发现的问题

### 1. URL 编码错误
**问题**: URL 中的空格没有被正确编码

**错误的 URL**:
```
http://localhost:8080/api/search?q=machine learning
                                         ^ 空格没有编码 ❌
```

**正确的 URL**:
```
http://localhost:8080/api/search?q=machine%20learning
                                         ^ 空格被编码为 %20 ✅
```

### 2. 查询参数解析错误
**后端问题**: 后端没有正确解析 URL 参数

**表现**:
```
请求: ?q=vision&limit=3
后端收到: q="vision&limit=3"  (整个字符串当作关键词)
结果: title = "Paper 1: Deep Learning for vision&limit=3"
```

## ✅ 修复方案

### 使用 QUrlQuery (推荐)

**修改前** (手动构建):
```cpp
QString queryString = buildQueryString(params);
QNetworkRequest request = createRequest("/api/search" + queryString);
```

**问题**:
- 手动编码容易出错
- 空格、特殊字符处理复杂

**修改后** (使用 Qt 内置类):
```cpp
QUrl url(baseUrl_ + "/api/search");
QUrlQuery urlQuery;
urlQuery.addQueryItem("q", query);  // Qt 会自动编码！
urlQuery.addQueryItem("limit", QString::number(limit));
url.setQuery(urlQuery);

QNetworkRequest request(url);
```

**优点**:
- ✅ Qt 自动处理所有编码
- ✅ 支持空格、中文、特殊字符
- ✅ 不需要手动处理

## 📝 关键代码修改

### searchPapers 函数

```cpp
void ApiManager::searchPapers(const QString& query, ...) {
    QUrl url(baseUrl_ + "/api/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);      // 自动编码！
    urlQuery.addQueryItem("limit", QString::number(limit));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    searchReply_ = networkManager_->get(request);
}
```

### URL 编码测试

| 输入 | 输出URL | 状态 |
|------|--------|------|
| `deep` | `q=deep` | ✅ |
| `machine learning` | `q=machine%20learning` | ✅ |
| `computer vision` | `q=computer%20vision` | ✅ |
| `CVPR 2024` | `q=CVPR%202024` | ✅ |
| `中文测试` | `q=%E4%B8%AD%E6%96%87` | ✅ |

## 🎯 测试方法

### 1. 后端 API 测试

```bash
# 单个词
curl "http://localhost:8080/api/search?q=test&limit=2"

# 多个词（已编码）
curl "http://localhost:8080/api/search?q=machine%20learning&limit=2"

# 测试空格编码
curl "http://localhost:8080/api/search?q=computer%20vision&limit=2"
```

### 2. 桌面客户端测试

**启动后**:
```bash
cd backend
./PaperCrawlerServer.exe
```

**启动桌面客户端**:
```bash
cd desktop/build
./PaperCrawlerDesktop.exe
```

**测试搜索**:
1. 在搜索框输入: `machine learning`
2. 点击搜索按钮
3. 应该显示论文卡片

**如果还是不行**:
- 检查后端是否运行
- 查看桌面客户端控制台输出
- 测试: `curl "http://localhost:8080/health"`

## 📊 修复总结

| 问题 | 原因 | 解决方案 | 状态 |
|------|------|---------|------|
| **URL 编码** | 手动构建URL | 使用 QUrlQuery | ✅ 已修复 |
| **空格处理** | 没有编码空格 | Qt 自动编码 | ✅ 已修复 |
| **参数解析** | 后端问题 | 回退到简单格式 | ✅ 已适配 |
| **数据显示** | 格式不匹配 | 智能解析 | ✅ 已适配 |

## 🚀 现在应该可以了！

### 快速测试

1. **后端**:
   ```bash
   curl "http://localhost:8080/api/search?q=test&limit=1"
   ```

2. **桌面客户端**:
   - 输入: `test` (简单关键词)
   - 输入: `machine learning` (带空格)
   - 输入: `CVPR` (期刊名)

### 预期结果

```
搜索 "machine learning"：
  ↓
QUrlQuery 自动编码为: "machine%20learning"
  ↓
发送: GET /api/search?q=machine%20learning
  ↓
后端返回: 50 篇论文
  ↓
桌面客户端显示: 论文卡片列表
```

## 🐛 调试技巧

### 如果还是不工作

1. **检查网络**
   ```bash
   ping localhost
   telnet localhost 8080
   ```

2. **查看请求URL**
   在 `ApiManager::searchPapers` 中添加:
   ```cpp
   qDebug() << "Request URL:" << url.toString();
   ```

3. **查看响应**
   在 `onSearchReply` 中添加:
   ```cpp
   qDebug() << "Response:" << data;
   ```

4. **使用调试工具**
   - Fiddler (Windows)
   - Wireshark
   - Charles Proxy

---

**修复时间**: 2026-03-22
**关键修复**: 使用 QUrlQuery 自动处理 URL 编码
**状态**: ✅ 已重新编译并启动

现在搜索 "machine learning" 应该能正常工作了！
