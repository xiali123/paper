# 分页功能问题诊断报告

## 🔍 问题现象

分页功能不工作：
- 点击"下一页"没有反应
- 修改每页显示数量无效
- 所有页面显示相同内容

## 🎯 根本原因

### 后端 Bug：URL 参数解析错误

**问题 1: 参数污染**
```bash
请求: GET /api/search?q=test&offset=0&limit=3
后端接收: keyword = "test&offset=0&limit=3"
结果: 论文标题变成 "Deep Learning for test&offset=0&limit=3"
```

**问题 2: limit 参数被忽略**
```bash
请求: limit=3
预期: 返回 3 篇论文
实际: 返回 20+ 篇论文
```

**问题 3: offset 参数被忽略**
```bash
请求: offset=5
预期: 跳过前 5 篇，从第 6 篇开始
实际: 仍然从第 1 篇开始
```

## 📊 测试证据

### 测试 1: limit 参数
```bash
$ curl "http://localhost:8080/api/search?q=test&offset=0&limit=3"

响应:
{
  "papers": [
    {"id": 1, "title": "Paper 1: Deep Learning for test&offset=0&limit=3"},
    {"id": 2, "title": "Paper 2: Deep Learning for test&offset=0&limit=3"},
    {"id": 3, "title": "Paper 3: Deep Learning for test&offset=0&limit=3"},
    {"id": 4, "title": "Paper 4: Deep Learning for test&offset=0&limit=3"},
    {"id": 5, "title": "Paper 5: Deep Learning for test&offset=0&limit=3"},
    {"id": 6, "title": "Paper 6: Deep Learning for test&offset=0&limit=3"},
    {"id": 7, "title": "Paper 7: Deep Learning for test&offset=0&limit=3"},
    // ... 返回了 7+ 篇，limit=3 被忽略
  ]
}
```

### 测试 2: offset 参数
```bash
$ curl "http://localhost:8080/api/search?q=test&offset=5&limit=3"

响应:
{
  "papers": [
    {"id": 1, "title": "Paper 1: Deep Learning for test&offset=5&limit=3"},
    // ... 仍然从 id=1 开始，offset=5 被忽略
  ]
}
```

## ✅ 客户端代码验证

客户端代码完全正确，所有逻辑都已正确实现：

### 1. 信号连接 ✅
```cpp
// PaperCardView.hpp
signals:
    void pageChanged(int offset, int limit);

// MainWindow.cpp
connect(resultView_, &PaperCardView::pageChanged,
        this, &MainWindow::onPageChanged);
```

### 2. 参数计算 ✅
```cpp
// 下一页
currentPage_++;
currentOffset_ = (currentPage_ - 1) * currentPageSize_;
emit pageChanged(currentOffset_, currentPageSize_);

// 示例：第2页，每页20条
currentOffset_ = (2 - 1) * 20 = 20 ✅
```

### 3. API 调用 ✅
```cpp
// ApiManager.cpp
urlQuery.addQueryItem("q", query);       // ✅
urlQuery.addQueryItem("offset", QString::number(offset));  // ✅
urlQuery.addQueryItem("limit", QString::number(limit));    // ✅
```

### 4. 数据流 ✅
```
用户点击"下一页"
    ↓
PaperCardView::onNextPage()
    ↓
计算: currentPage_ = 2, offset = 20
    ↓
emit pageChanged(20, 20)
    ↓
MainWindow::onPageChanged(20, 20)
    ↓
ApiManager::searchPapers("test", "", "", 20, 20)
    ↓
发送请求: GET /api/search?q=test&offset=20&limit=20  ✅
    ↓
后端返回: 错误的数据（offset/limit 被忽略） ❌
```

## 🔧 需要修复后端

### 后端需要实现以下功能：

#### 1. 正确解析 URL 参数
```cpp
// backend/src/api_server.cpp

// 当前（错误）:
std::string query = req.get_param_value("q");  // 获取整个查询字符串

// 应该改为:
std::string query = req.get_param_value("q");
int offset = std::stoi(req.get_param_value("offset", "0"));
int limit = std::stoi(req.get_param_value("limit", "20"));
```

#### 2. SQL 查询支持分页
```sql
-- 当前（可能）:
SELECT * FROM papers WHERE title LIKE '%keyword%' LIMIT 20;

-- 应该改为:
SELECT * FROM papers
WHERE title LIKE '%keyword%'
LIMIT ? OFFSET ?;
```

#### 3. 正确返回分页信息
```json
{
  "papers": [...],
  "total": 50,
  "offset": 20,
  "limit": 20,
  "page": 2
}
```

## 💡 解决方案选项

### 选项 1: 修复后端（推荐）

**优点**:
- ✅ 正确的解决方案
- ✅ 支持大数据集
- ✅ 减少网络传输

**缺点**:
- ❌ 需要重新编译后端
- ❌ 需要修改后端代码

**步骤**:
1. 找到后端 API 服务器代码
2. 修复 URL 参数解析
3. 添加 SQL offset/limit 支持
4. 重新编译并测试

### 选项 2: 客户端缓存（临时方案）

**优点**:
- ✅ 不需要修改后端
- ✅ 可以立即实现
- ✅ 适用于小数据集

**缺点**:
- ❌ 第一次搜索慢（加载全部数据）
- ❌ 不适合大数据集（>1000篇）
- ❌ 内存占用较高

**实现思路**:
```cpp
// MainWindow 中缓存所有搜索结果
QList<Paper> allPapers_;  // 存储所有论文

void MainWindow::onSearchSuccess(const SearchResult& result) {
    allPapers_ = papers;  // 保存全部
    showPage(1, 20);      // 显示第一页
}

void MainWindow::onPageChanged(int offset, int limit) {
    // 从缓存中取出对应页的数据
    int start = offset;
    int end = std::min(offset + limit, allPapers_.size());
    QList<Paper> pageData = allPapers_.mid(start, end - start);
    resultView_->setPapers(pageData, allPapers_.size());
}
```

## 📝 建议

**短期**: 先实现选项 2（客户端缓存），让分页功能立即可用

**长期**: 修复后端（选项 1），实现正确的服务端分页

---

**诊断时间**: 2026-03-22
**问题确认**: 后端 URL 参数解析错误
**客户端状态**: ✅ 代码正确，等待后端修复或实现客户端缓存
