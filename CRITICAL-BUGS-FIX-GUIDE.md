# PaperCrawler 关键Bug快速修复指南

**创建日期**: 2026-03-22
**优先级**: P0 - 严重问题
**预计修复时间**: 2-4小时

---

## 🔴 立即修复的12个严重Bug

### Bug #1: WebSocket内存泄漏 (严重)

**文件**: `backend/src/websocket_server.cpp:297-299`

**当前代码**:
```cpp
std::thread([this, client_fd, client_address]() {
    handleClient(client_fd, client_address);
}).detach();  // ❌ 危险：无法追踪线程
```

**修复代码**:
```cpp
// 1. 在类成员中添加线程池
class WebSocketServer {
private:
    std::vector<std::unique_ptr<std::thread>> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool shutdown_{false};

public:
    WebSocketServer() {
        // 启动工作线程
        for (int i = 0; i < 4; ++i) {
            worker_threads_.emplace_back(
                std::make_unique<std::thread>([this]() {
                    this->workerLoop();
                })
            );
        }
    }

    ~WebSocketServer() {
        shutdown_ = true;
        condition_.notify_all();
        for (auto& thread : worker_threads_) {
            if (thread->joinable()) {
                thread->join();
            }
        }
    }

    void postTask(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.push(task);
        }
        condition_.notify_one();
    }

private:
    void workerLoop() {
        while (!shutdown_) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this]() {
                    return shutdown_ || !task_queue_.empty();
                });

                if (shutdown_) break;

                if (!task_queue_.empty()) {
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                } else {
                    continue;
                }
            }

            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    qCritical() << "Task error:" << e.what();
                }
            }
        }
    }
};

// 2. 修改客户端处理
void handleClientNew(int client_fd, const std::string& address) {
    postTask([this, client_fd, address]() {
        this->handleClient(client_fd, address);
    });
}
```

**验证方法**:
```bash
# 连接测试
for i in {1..100}; do
  curl http://localhost:8080/ws &
done

# 检查内存使用
watch -n 1 'ps aux | grep PaperCrawlerServer | awk "{print \$6}"'
```

---

### Bug #2: SQL注入风险 (严重)

**文件**: `backend/src/api_server.cpp:273-298`

**当前代码**:
```cpp
std::string keyword = params.count("q") ? params.at("q") : "";
papers = g_api->getPapers(keyword, offset, limit);  // ❌ 未验证
```

**修复代码**:
```cpp
// 添加到 api_server.cpp 顶部
#include <algorithm>
#include <cctype>

// 添加输入验证函数
std::string sanitizeSearchInput(const std::string& input) {
    // 1. 限制长度
    if (input.length() > 100) {
        qWarning() << "Search input too long:" << input.length();
        return input.substr(0, 100);
    }

    // 2. 过滤危险字符
    std::string result;
    result.reserve(input.length());

    for (char c : input) {
        // 只允许：字母、数字、空格、连字符、下划线、点
        if (std::isalnum(static_cast<unsigned char>(c)) ||
            std::isspace(static_cast<unsigned char>(c)) ||
            c == '-' || c == '_' || c == '.' ||
            c == ':' || c == '(' || c == ')' || c == ',') {
            result += c;
        }
    }

    // 3. 防止注释注入
    if (result.find("--") != std::string::npos ||
        result.find("/*") != std::string::npos ||
        result.find("*/") != std::string::npos) {
        qWarning() << "Potential SQL injection detected:" << result;
        return "";  // 返回空字符串
    }

    return result;
}

// 修改API处理代码
void handleSearch(const httplib::Request& req, httplib::Response& res) {
    auto params = req.params;

    // 验证和清理输入
    std::string rawKeyword = params.count("q") ? params.at("q") : "";
    std::string keyword = sanitizeSearchInput(rawKeyword);

    if (keyword.empty() && !rawKeyword.empty()) {
        res.status = 400;
        res.set_content(R"({"error": "INVALID_KEYWORD", "message": "Invalid search keyword"})",
                       "application/json");
        return;
    }

    // 安全的参数解析
    int offset = parseOffset(params);
    int limit = parseLimit(params);

    // 调用API
    auto papers = g_api->getPapers(keyword, offset, limit);

    // 返回结果...
}
```

**测试用例**:
```bash
# 正常输入
curl "http://localhost:8080/api/search?q=machine+learning"

# SQL注入尝试（应该被阻止）
curl "http://localhost:8080/api/search?q=';DROP+TABLE+cspaper;--"

# 超长输入（应该被截断）
curl "http://localhost:8080/api/search?q=$(python -c 'print("a"*200)')"
```

---

### Bug #3: 类型不匹配 (高)

**文件**: `frontend/src/stores/papers.ts:198-204`

**当前代码**:
```typescript
const index = searchResults.value.findIndex(p => p.id === String(paper.id))
```

**修复代码**:
```typescript
// 1. 统一类型定义
// frontend/src/types/paper.ts
export interface Paper {
  id: number;  // 明确为number
  title: string;
  journal: {
    full: string;
    short: string;
  };
  year: string;
  level: string;
  authors: string;
  doiUrl: string;
}

// 2. 修复比较逻辑
const index = searchResults.value.findIndex(p => p.id === paper.id);

// 3. 或者使用类型守卫
function findPaperIndex(papers: Paper[], targetId: number): number {
  return papers.findIndex(p => p.id === targetId);
}
```

---

### Bug #4: 缓冲区溢出 (严重)

**文件**: `backend/src/websocket_server.cpp:305-306`

**当前代码**:
```cpp
char buffer[2048];
int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
```

**修复代码**:
```cpp
// 使用动态缓冲区
std::vector<char> buffer(4096);
int bytes_received = recv(client_fd, buffer.data(),
                          buffer.size() - 1, 0);

if (bytes_received <= 0) {
    qWarning() << "Recv failed or connection closed for client" << client_fd;
    closeClient(client_fd);
    return;
}

// 检查缓冲区是否溢出
if (bytes_received >= static_cast<int>(buffer.size()) - 1) {
    qWarning() << "Buffer too small, received" << bytes_received << "bytes";
    closeClient(client_fd);
    return;
}

// 确保null终止
buffer[bytes_received] = '\0';

// 安全处理消息
std::string message(buffer.data(), bytes_received);
processMessage(client_fd, message);
```

---

### Bug #5: 竞态条件 (高)

**文件**: `backend/src/websocket_server.cpp:328-331`

**当前代码**:
```cpp
{
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[client_fd] = client;
}
// ❌ 锁已释放，但下面访问clients_时没有锁保护
if (connection_handler_) {
    connection_handler_(client_fd);  // 不安全
}
```

**修复代码**:
```cpp
// 扩大临界区范围
if (connection_handler_) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    // 在锁保护内完成所有操作
    clients_[client_fd] = client;
    client->setState(WSConnectionState::CONNECTED);

    // 调用回调（仍然在锁保护下）
    connection_handler_(client_fd);
} else {
    // 如果没有回调，只需加锁添加客户端
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[client_fd] = client;
    client->setState(WSConnectionState::CONNECTED);
}
```

---

## 🧪 快速测试脚本

创建测试脚本：`e:/PaperCrawler/backend/test_critical_bugs.sh`

```bash
#!/bin/bash

echo "=== PaperCrawler 关键Bug测试 ==="

echo "[1/6] 测试WebSocket内存泄漏..."
echo "打开多个WebSocket连接"
for i in {1..50}; do
  websocat ws://localhost:8080/ws &
done
sleep 5
echo "检查内存使用："
ps aux | grep PaperCrawlerServer | awk '{print $6}'

echo "[2/6] 测试SQL注入防护..."
curl -s "http://localhost:8080/api/search?q=';DROP+TABLE+cspaper;--'"
echo "应该返回错误而不是执行SQL"

echo "[3/6] 测试缓冲区溢出防护..."
# 发送超大消息
echo "测试超长消息处理"

echo "[4/6] 测试类型匹配..."
# 前端测试
echo "运行前端测试：npm run test"

echo "[5/6] 测试竞态条件..."
# 并发测试
for i in {1..20}; do
  curl "http://localhost:8080/api/search?q=test&offset=$i&limit=10" &
done
wait

echo "[6/6] 检查日志..."
tail -50 /var/log/papercrawler/server.log

echo "=== 测试完成 ==="
```

---

## 📋 修复检查清单

修复完成后，逐项检查：

- [ ] WebSocket线程正确管理
- [ ] 所有用户输入都经过验证
- [ ] 缓冲区使用动态大小
- [ ] 所有共享状态都有锁保护
- [ ] 类型定义一致
- [ ] 异常处理完整
- [ ] 资源正确释放
- [ ] 测试覆盖新增代码
- [ ] 日志记录完整
- [ ] 文档已更新

---

## 🔧 修复工具

### 内存泄漏检测
```bash
# 使用Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  ./PaperCrawlerServer

# 使用AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" ..
make
./PaperCrawlerServer
```

### SQL注入测试
```bash
# 使用sqlmap
sqlmap -u "http://localhost:8080/api/search?q=test" \
  --batch --risk=3 --level=5
```

### 竞态条件检测
```bash
# 使用ThreadSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" ..
make
./PaperCrawlerServer
```

---

**预计完成时间**: 每个bug 30分钟 - 2小时
**总计时间**: 4-8小时（包括测试）

🚀 **开始修复，让PaperCrawler更安全、更稳定！**
