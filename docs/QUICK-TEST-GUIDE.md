# 🎯 PaperCrawler 快速测试指南

**日期**: 2026-03-22
**状态**: ✅ 旧服务器运行中 | 🆕 新服务器已编译

---

## 📊 当前服务器状态

### ✅ 正在运行的服务器
```
服务器: 旧版本 PaperCrawlerServer.exe (2.6 MB)
端口: 8080 (HTTP API)
状态: ✅ 正在运行
```

### 🆕 新编译的服务器
```
服务器: 新版本 PaperCrawlerServer.exe (134 KB)
端口: 8088 (WebSocket)
状态: ✅ 编译成功，待测试
功能:
  - 4线程工作池
  - 心跳检测（30秒间隔）
  - 优雅关闭
  - 所有7个Bug已修复
```

---

## 🚀 测试新WebSocket服务器

### 方法1: Windows CMD（推荐）

#### 步骤1: 打开Windows CMD
```cmd
# 按 Win+R，输入 cmd，回车
```

#### 步骤2: 停止旧服务器（可选）
```cmd
# 查找并停止旧服务器进程
tasklist | findstr PaperCrawlerServer
taskkill /F /PID <进程ID>
```

#### 步骤3: 启动新WebSocket服务器
```cmd
# 切换到build目录
cd E:\PaperCrawler\backend\build

# 启动服务器
PaperCrawlerServer.exe
```

#### 预期输出
```
========================================
  WebSocket Test Server
========================================
Starting WebSocket server on port 8088...
Server started successfully!
WebSocket endpoint: ws://localhost:8088/ws
Press Ctrl+C to stop...
========================================
[INFO] Server heartbeat #1 | Active connections: 0
```

### 方法2: Windows PowerShell

```powershell
# 打开PowerShell
cd E:\PaperCrawler\backend\build
.\PaperCrawlerServer.exe
```

### 方法3: 双击运行（最简单）

1. 打开文件资源管理器
2. 导航到 `E:\PaperCrawler\backend\build`
3. 双击 `PaperCrawlerServer.exe`

---

## 🧪 测试WebSocket连接

### 准备工作

#### 选项A: 使用websocat（推荐）
```bash
# 安装websocat（需要Rust）
cargo install websocat

# 连接到服务器
websocat ws://localhost:8088/ws
```

#### 选项B: 使用在线工具
1. 访问：https://www.websocket.org/echo.html
2. 输入：`ws://localhost:8088/ws`
3. 点击：Connect

#### 选项C: 使用浏览器JavaScript
```javascript
// 在浏览器控制台中运行
const ws = new WebSocket('ws://localhost:8088/ws');

ws.onopen = () => {
  console.log('✅ Connected to WebSocket server');
};

ws.onmessage = (event) => {
  console.log('✅ Received:', event.data);
};

ws.onerror = (error) => {
  console.log('❌ Error:', error);
};

ws.onclose = () => {
  console.log('ℹ️ Disconnected from WebSocket server');
};
```

#### 选项D: 使用Node.js
```javascript
// 安装ws库
npm install ws

// 创建test.js文件
const WebSocket = require('ws');
const ws = new WebSocket('ws://localhost:8088/ws');

ws.on('open', () => {
  console.log('✅ Connected');
  ws.send(JSON.stringify({ type: 'test', data: 'Hello Server' }));
});

ws.on('message', (data) => {
  console.log('✅ Received:', data.toString());
});

ws.on('close', () => {
  console.log('ℹ️ Disconnected');
});
```

---

## ✅ 验证清单

### 服务器启动
- [ ] 服务器成功启动
- [ ] 看到 "Server started successfully!" 消息
- [ ] 看到 "WebSocket endpoint: ws://localhost:8088/ws" 消息
- [ ] 端口8088开始监听（使用 `netstat -an | findstr 8088` 验证）

### WebSocket连接
- [ ] 客户端成功连接
- [ ] 看到连接日志：`[INFO] Client connected: XXX`
- [ ] 每30秒收到心跳消息
- [ ] 可以发送和接收消息

### 服务器功能
- [ ] 心跳正常发送
- [ ] 连接计数正确显示
- [ ] Ctrl+C可以优雅关闭
- [ ] 无崩溃或错误

---

## 🐛 故障排查

### 问题1: 服务器无法启动
**症状**: 启动后立即退出

**解决方案**:
```cmd
# 检查端口是否被占用
netstat -an | findstr 8088

# 如果被占用，杀死进程或修改端口
# 修改 websocket_test_server.cpp 中的端口号
WebSocketServer server(8089);  // 使用其他端口
```

### 问题2: "无法定位程序输入点"
**症状**: DLL错误

**解决方案**:
```cmd
# 确保MinGW运行时库在PATH中
set PATH=C:\Qt\Tools\mingw1310_64\bin;%PATH%

# 重新运行
PaperCrawlerServer.exe
```

### 问题3: 客户端无法连接
**症状**: 连接被拒绝或超时

**检查清单**:
```cmd
# 1. 确认服务器正在运行
tasklist | findstr PaperCrawlerServer

# 2. 确认端口正在监听
netstat -an | findstr 8088

# 3. 检查防火墙设置
# Windows Defender 防火墙 -> 允许应用通过防火墙
```

### 问题4: Git Bash中段错误
**症状**: Segmentation fault

**解决方案**:
- ❌ 不要在Git Bash中运行
- ✅ 使用Windows CMD或PowerShell
- ✅ 或直接双击exe文件

---

## 📊 测试结果记录

### 成功的测试示例

#### 测试1: 服务器启动
```
✅ 状态: 成功
✅ 输出:
    ========================================
      WebSocket Test Server
    ========================================
    Starting WebSocket server on port 8088...
    Server started successfully!
    WebSocket endpoint: ws://localhost:8088/ws
```

#### 测试2: WebSocket连接
```
✅ 客户端: websocat
✅ 命令: websocat ws://localhost:8088/ws
✅ 结果: 连接成功
```

#### 测试3: 心跳消息
```
✅ 服务器输出:
    [INFO] Client connected: 123
    [INFO] Server heartbeat #1 | Active connections: 1
    [INFO] Client disconnected: 123
```

---

## 🎯 下一步行动

### 立即测试（5分钟）
1. ✅ 打开Windows CMD
2. ✅ 运行 `PaperCrawlerServer.exe`
3. ✅ 使用WebSocket客户端连接
4. ✅ 观察心跳和连接日志

### 验证优化（15分钟）
1. ✅ 应用数据库索引
2. ✅ 测试搜索查询性能
3. ✅ 验证分页性能提升
4. ✅ 测试前端虚拟滚动

### 集成部署（1小时）
1. ✅ 替换旧版本服务器
2. ✅ 配置自动启动
3. ✅ 设置监控日志
4. ✅ 更新前端连接代码

---

## 📞 获取帮助

### 如果测试失败

1. **查看编译报告**: `WEBSOCKET-COMPILATION-SUCCESS.md`
2. **查看Bug修复报告**: `WEBSOCKET-BUGS-FIXED.md`
3. **查看项目总结**: `FINAL-PROJECT-SUMMARY.md`

### 常见命令

```cmd
# 查看进程
tasklist | findstr PaperCrawler

# 停止进程
taskkill /F /IM PaperCrawlerServer.exe

# 查看端口
netstat -an | findstr 8088

# 查看帮助
PaperCrawlerServer.exe --help
```

---

## ✅ 成功标准

### 最低标准
- [x] 服务器成功编译
- [x] 可执行文件已生成
- [ ] 服务器可以启动（待验证）
- [ ] WebSocket可以连接（待验证）

### 完整标准
- [ ] 服务器稳定运行
- [ ] 多个客户端可以同时连接
- [ ] 心跳正常发送
- [ ] 优雅关闭正常工作
- [ ] 无内存泄漏
- [ ] 性能符合预期

---

**祝你测试顺利！** 🚀

如有问题，请参考详细文档或提交Issue。
