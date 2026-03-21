# 桌面客户端超时问题修复

## 🔍 问题根源

### 错误: "Operation canceled"
```
搜索论文时出错:网络错误:Operation canceled
```

### 原因分析

**原来的错误代码** (ApiManager.cpp 构造函数):
```cpp
ApiManager::ApiManager(QObject* parent)
    : QObject(parent), networkManager_(new QNetworkAccessManager(this)) {
    baseUrl_ = "http://localhost:8080";

    // ❌ 错误：这个定时器会在对象创建 30 秒后触发一次
    QTimer::singleShot(30000, this, [this]() {
        if (healthReply_) healthReply_->abort();
        if (searchReply_) searchReply_->abort();
        if (paperDetailsReply_) paperDetailsReply_->abort();
        if (recentPapersReply_) recentPapersReply_->abort();
    });
}
```

**问题**:
- `QTimer::singleShot(30000, ...)` 在 ApiManager 创建后 30 秒触发 **一次**
- 如果用户在 30 秒后发起任何请求，定时器已经触发过，不会再次触发
- 但如果用户在 30 秒**内**发起请求，定时器仍在等待
- 更严重的是：这个定时器会 **取消所有正在进行的请求**
- 结果：任何请求都可能被突然取消，导致 "Operation canceled" 错误

## ✅ 修复方案

### 1. 移除全局定时器

```cpp
ApiManager::ApiManager(QObject* parent)
    : QObject(parent), networkManager_(new QNetworkAccessManager(this)) {
    baseUrl_ = "http://localhost:8080";
    // ✅ 移除了错误的定时器
}
```

### 2. 添加每个请求独立的超时机制

**新增方法**:
```cpp
void ApiManager::setupRequestTimeout(QNetworkReply* reply, int timeoutMs) {
    if (!reply) return;

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(timeoutMs);

    connect(timer, &QTimer::timeout, this, [this, reply, timer]() {
        if (reply && reply->isRunning()) {
            qWarning() << "Request timeout, aborting:" << reply->url();
            reply->abort();
        }
        timer->deleteLater();
    });

    // 请求完成时停止计时器
    connect(reply, &QNetworkReply::finished, timer, [timer]() {
        if (timer->isActive()) {
            timer->stop();
        }
        timer->deleteLater();
    });

    timer->start();
}
```

### 3. 为每个请求设置独立超时

**健康检查** (10秒):
```cpp
void ApiManager::checkHealth() {
    QNetworkRequest request = createRequest("/health");
    healthReply_ = networkManager_->get(request);
    setupRequestTimeout(healthReply_, 10000);  // ✅ 10秒超时
    // ...
}
```

**搜索** (15秒):
```cpp
void ApiManager::searchPapers(...) {
    // ...
    searchReply_ = networkManager_->get(request);
    setupRequestTimeout(searchReply_, 15000);  // ✅ 15秒超时
    // ...
}
```

**论文详情** (10秒):
```cpp
void ApiManager::getPaperDetails(int paperId) {
    // ...
    paperDetailsReply_ = networkManager_->get(request);
    setupRequestTimeout(paperDetailsReply_, 10000);  // ✅ 10秒超时
    // ...
}
```

**最近论文** (10秒):
```cpp
void ApiManager::getRecentPapers(int limit) {
    // ...
    recentPapersReply_ = networkManager_->get(request);
    setupRequestTimeout(recentPapersReply_, 10000);  // ✅ 10秒超时
    // ...
}
```

## 🎯 修复效果

### 修复前
```
用户点击搜索
    ↓
请求发送
    ↓
30秒后定时器触发 ❌
    ↓
请求被取消: "Operation canceled"
    ↓
显示错误
```

### 修复后
```
用户点击搜索
    ↓
请求发送 + 启动15秒计时器
    ↓
请求完成 (通常 < 1秒) ✅
    ↓
计时器停止
    ↓
正常显示结果
```

## 📊 技术细节

### 新超时机制的优势

| 特性 | 旧机制 | 新机制 |
|------|--------|--------|
| **超时范围** | 全局30秒一次 | 每个请求独立 |
| **内存管理** | 无清理 | 自动deleteLater |
| **错误信息** | "Operation canceled" | "Request timeout" + URL |
| **调试信息** | 无 | qWarning输出超时URL |
| **灵活性** | 固定30秒 | 可配置(10/15秒) |

### 计时器生命周期

```
请求开始
    ↓
创建 QTimer (timeoutMs)
    ↓
┌─────────────────────────┐
│  计时器运行中...         │
│  ┌─────────┐            │
│  │ 请求处理 │            │
│  └─────────┘            │
└─────────────────────────┘
    ↓
情况1: 请求完成 → 停止计时器 → deleteLater
情况2: 超时 → abort请求 → deleteLater
```

## 🚀 现在应该可以了！

### 测试步骤

1. **后端已启动** ✅
   ```bash
   cd backend
   ./PaperCrawlerServer.exe
   # 运行中 PID: 301284
   ```

2. **桌面客户端已启动** ✅
   ```bash
   cd desktop/build
   ./PaperCrawlerDesktop.exe
   # 运行中 PID: 310276
   ```

3. **测试搜索**
   - 输入: `test`
   - 输入: `deep`
   - 输入: `learning`

4. **预期结果**
   - 不再出现 "Operation canceled" 错误
   - 正常显示论文列表
   - 请求在 1 秒内完成

## 📝 总结

| 问题 | 解决方案 | 状态 |
|------|----------|------|
| **错误超时机制** | 移除全局定时器 | ✅ 已修复 |
| **缺少超时控制** | 每请求独立超时 | ✅ 已添加 |
| **内存泄漏** | 自动deleteLater | ✅ 已修复 |
| **调试困难** | 添加qWarning输出 | ✅ 已改进 |

---

**修复时间**: 2026-03-22 01:08
**关键修改**: ApiManager.cpp 超时机制
**状态**: ✅ 已重新编译并运行

现在搜索功能应该完全正常了！每个请求都有独立的超时控制，不会再被意外取消。
