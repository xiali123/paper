# 桌面客户端数据库集成指南

## 📋 已完成

### 1. 本地数据库模块

**创建文件**:
- ✅ `desktop/src/database/LocalDatabase.hpp` - 头文件
- ✅ `desktop/src/database/LocalDatabase.cpp` - 实现文件

**功能**:
- ✅ SQLite 数据库管理
- ✅ 论文 CRUD 操作
- ✅ 全文搜索（FTS5）
- ✅ DOI 去重
- ✅ 标题哈希去重
- ✅ 同步状态管理
- ✅ 数据备份

## 🔧 集成步骤

### Step 1: 更新 CMakeLists.txt

在 `desktop/CMakeLists.txt` 中添加：

```cmake
# 数据库模块
set(DATABASE_SOURCES
    src/database/LocalDatabase.cpp
)

set(DATABASE_HEADERS
    include/database/LocalDatabase.hpp
)

# 添加到构建
list(APPEND SOURCES ${DATABASE_SOURCES})
list(APPEND HEADERS ${DATABASE_HEADERS})
```

### Step 2: 更新 MainWindow.hpp

在 `desktop/include/MainWindow.hpp` 中添加：

```cpp
#include "database/LocalDatabase.hpp"

class MainWindow : public QMainWindow {
    // ... 现有代码 ...

private:
    // 数据库
    LocalDatabase* localDb_{nullptr};

    // ... 现有代码 ...
};
```

### Step 3: 初始化数据库

在 `MainWindow.cpp` 的构造函数中：

```cpp
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    // ... 现有初始化代码 ...

    // 初始化本地数据库
    localDb_ = new LocalDatabase(this);
    if (!localDb_->open()) {
        QMessageBox::warning(this, "数据库错误",
                           "无法打开本地数据库");
    }

    // 连接数据库信号
    connect(localDb_, &LocalDatabase::paperAdded,
            this, &MainWindow::onPaperAdded);
    connect(localDb_, &LocalDatabase::databaseError,
            this, &MainWindow::onDatabaseError);
}
```

### Step 4: 实现优先级搜索

修改 `MainWindow::onSearch` 方法：

```cpp
void MainWindow::onSearch(const QString& keyword) {
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "搜索", "请输入关键词");
        return;
    }

    currentKeyword_ = keyword;
    statusBar()->showMessage("正在搜索: " + keyword + "...");
    resultView_->clear();

    // === 优先级 1: 本地数据库 ===
    SearchResult localResult = localDb_->searchPapers(keyword, 0, 20);

    if (!localResult.papers.isEmpty()) {
        qDebug() << "Found" << localResult.papers.size()
                 << "papers in local database";

        // 显示本地结果
        displayPapers(localResult.papers, localResult.totalCount);
        statusBar()->showMessage(
            QString("本地: 找到 %1 篇论文").arg(localResult.totalCount)
        );

        // 如果本地结果足够，不需要继续
        if (localResult.papers.size() >= 20) {
            return;
        }
    }

    // === 优先级 2: 后端服务器 ===
    qDebug() << "Searching backend server...";
    apiManager_->searchPapers(keyword, "", "", 0, 20);
}

void MainWindow::onSearchSuccess(const SearchResult& apiResult) {
    // 后端搜索成功

    // 转换并保存到本地数据库
    QList<Paper> newPapers;
    for (const auto& apiPaper : apiResult.papers) {
        Paper paper;
        paper.id = apiPaper.id;
        paper.title = apiPaper.title;
        paper.journal = apiPaper.journalShort.isEmpty()
                        ? apiPaper.journalFull
                        : apiPaper.journalShort;
        paper.year = apiPaper.year;
        paper.level = apiPaper.level;
        paper.authors = apiPaper.authors;
        paper.doiUrl = apiPaper.doiUrl;

        // 检查是否已存在
        if (!localDb_->existsByTitle(paper.title)) {
            int paperId = localDb_->savePaper(paper);
            if (paperId > 0) {
                paper.id = paperId;
                newPapers.append(paper);
            }
        }
    }

    // 显示结果
    displayPapers(newPapers, apiResult.total);

    // === 优先级 3: 爬虫（如果结果不足）===
    if (newPapers.size() < 10) {
        qDebug() << "Not enough results, starting crawler...";
        startCrawler(keyword);
    }
}
```

### Step 5: 添加新的槽函数

```cpp
void MainWindow::onPaperAdded(int paperId) {
    qDebug() << "Paper added to database:" << paperId;
}

void MainWindow::onDatabaseError(const QString& error) {
    qWarning() << "Database error:" << error;
    statusBar()->showMessage("数据库错误: " + error.left(50));
}
```

### Step 6: 更新显示逻辑

```cpp
void MainWindow::displayPapers(const QList<Paper>& papers, int total) {
    resultView_->setPapers(papers, total);

    QString message = QString("找到 %1 篇论文").arg(total);
    if (total > 20) {
        message += QString(" (显示前 20 篇)");
    }

    statusBar()->showMessage(message, 5000);
}
```

## 📊 数据流程

```
用户搜索 "deep learning"
    ↓
1. localDb_->searchPapers("deep learning")
    ↓ SELECT * FROM papers_fts WHERE MATCH 'deep learning*'
    ↓
2. 如果找到 ≥20 篇 → 显示结果 ✓
    ↓ 如果找到 <20 篇
    ↓
3. apiManager_->searchPapers("deep learning")
    ↓ GET /api/search?q=deep learning
    ↓
4. 后端返回论文
    ↓
5. 保存到本地数据库
    ↓ for each paper: localDb_->savePaper(paper)
    ↓
6. 显示结果
    ↓
7. 如果还是 <10 篇
    ↓
8. startCrawler("deep learning")
    ↓
9. 爬虫: arXiv, Semantic Scholar, etc.
    ↓
10. 去重
    ↓
11. 保存到本地 + 标记为 'pending' 同步
    ↓
12. 异步同步到后端
    ↓
13. 显示最终结果
```

## 🎯 下一步

### 立即可做

1. **编译测试**
   ```bash
   cd desktop/build
   cmake --build . --config Release
   ```

2. **运行测试**
   ```bash
   ./PaperCrawlerDesktop.exe
   ```

3. **验证功能**
   - 搜索论文
   - 检查本地数据库文件
   - 查看日志输出

### 数据库文件位置

- **Windows**: `C:\Users\<用户名>\AppData\Local\PaperCrawler\papers.db`
- **Linux**: `~/.local/share/PaperCrawler/papers.db`
- **macOS**: `~/Library/Application Support/PaperCrawler/papers.db`

### 查看数据库

使用 DB Browser for SQLite:
```bash
# 下载: https://sqlitebrowser.org/
# 打开: papers.db
# 表: papers, papers_fts
```

## 🔍 调试技巧

### 1. 查看数据库内容

```cpp
// 添加到 MainWindow::onSearch
QList<Paper> all = localDb_->getAllPapers();
qDebug() << "Total papers in database:" << all.size();
for (const auto& paper : all) {
    qDebug() << " -" << paper.title;
}
```

### 2. 测试全文搜索

```sql
-- 在 SQLite 中测试
SELECT * FROM papers_fts WHERE papers_fts MATCH 'deep*';
```

### 3. 检查去重

```cpp
// 测试 DOI 去重
bool exists = localDb_->existsByDoi("10.1234/example");
qDebug() << "DOI exists:" << exists;

// 测试标题去重
bool titleExists = localDb_->existsByTitle("Deep Learning for Computer Vision");
qDebug() << "Title exists:" << titleExists;
```

## ⚠️ 注意事项

1. **SQLite 依赖**
   - Qt 默认包含 SQLite 驱动
   - 确认编译时启用了 SQL 模块：`QT += sql`

2. **线程安全**
   - LocalDatabase 使用读写锁
   - 可以在多线程环境中安全使用

3. **性能优化**
   - FTS5 全文搜索很快
   - 如果有 10000+ 篇论文，考虑分页

4. **数据备份**
   - 定期调用 `localDb_->backup()`
   - 建议每天备份一次

## 📝 文件清单

**新增文件**:
- ✅ `desktop/src/database/LocalDatabase.hpp`
- ✅ `desktop/src/database/LocalDatabase.cpp`
- ✅ `desktop/DATABASE_INTEGRATION.md` (本文档)

**需要修改的文件**:
- ⏳ `desktop/CMakeLists.txt`
- ⏳ `desktop/include/MainWindow.hpp`
- ⏳ `desktop/src/MainWindow.cpp`

---

**创建时间**: 2026-03-22
**状态**: 数据库模块完成，待集成
**优先级**: 高
