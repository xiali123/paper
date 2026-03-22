# 本地数据库集成 - 简化方案

## ⚠️ 当前问题

编译错误：类型不匹配和重复定义

**问题根源**：
1. `Paper` 结构在多个文件中定义
2. `SearchResult` 也有重复定义
3. 需要统一类型系统

## 💡 快速解决方案

### 方案 A: 暂时禁用数据库（推荐用于测试）

快速恢复编译，测试分页修复：

```cpp
// MainWindow.cpp 构造函数中注释掉数据库初始化
/*
localDb_ = new LocalDatabase(this);
if (!localDb_->open()) {
    // ...
}
*/

// onSearch 中简化为只调用后端
void MainWindow::onSearch(const QString& keyword) {
    // 直接调用后端，不查本地
    apiManager_->searchPapers(keyword, "", "", 0, 20);
}
```

### 方案 B: 创建统一的类型系统

创建一个公共的头文件定义所有共享类型：

**创建文件**: `desktop/include/PaperTypes.hpp`

```cpp
#pragma once
#include <QString>
#include <QDateTime>

// 论文类型（显示用）
struct DisplayPaper {
    int id;
    QString title;
    QString journal;
    QString year;
    QString level;
    QString authors;
    QString doiUrl;
};

// 论文类型（数据库用）
struct DbPaper {
    int id{-1};
    QString doi;
    QString title;
    QString authors;
    QString abstract;
    QString journal;
    QString year;
    QString volume;
    QString issue;
    QString pages;
    QString keywords;
    QString pdfUrl;
    QString source;
    QDateTime createdAt;
    QDateTime updatedAt;
    QString syncStatus;
};

// API 论文类型
struct ApiPaper {
    int id;
    QString title;
    QString journalFull;
    QString journalShort;
    QString year;
    QString level;
    QString authors;
    QString doiUrl;
    QString type;
};

// 搜索结果类型
struct SearchResult {
    QList<DisplayPaper> papers;
    int total;
    int offset;
    int limit;
    qreal durationMs;
    QString query;
};
```

然后所有文件都 `#include "PaperTypes.hpp"`

## 🎯 建议

**立即执行**：
1. 暂时注释掉数据库相关代码
2. 优先修复后端 API 的 offset/limit 问题
3. 完成后再添加数据库功能

**原因**：
- 后端 API 是分页功能的根本问题
- 数据库是增强功能，不是必需的
- 先让基本功能工作，再添加高级功能

## 📝 修改步骤

### Step 1: 注释数据库代码

```cpp
// MainWindow.cpp 构造函数
// localDb_ = new LocalDatabase(this);
// localDb_ = nullptr;

// MainWindow.cpp onSearch
// DbSearchResult localResult = localDb_->searchPapers(keyword, 0, 20);
// ... 暂时跳过本地搜索
```

### Step 2: 编译测试

```bash
cd desktop/build
cmake --build . --config Release
```

### Step 3: 修复后端（更重要）

修复 `backend/src/api_server.cpp` 中的 URL 参数解析。

---

**建议**：选择方案 A，先让分页功能工作，再慢慢添加数据库。

您想选择哪个方案？
