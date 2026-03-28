# PaperCrawler WebSocket 实时同步实现指南

## 概述

本文档描述了 PaperCrawler 的 WebSocket 实时数据同步功能的完整实现，包括前端、后端和协议定义。

## 系统架构

```
┌─────────────────┐         WebSocket          ┌─────────────────┐
│   前端 Vue 3    │◄───────────────────────►│   后端 C++      │
│                 │     JSON 消息协议         │                 │
│  ┌───────────┐  │                           │  ┌───────────┐  │
│  │   Pinia   │  │                           │  │   API     │  │
│  │  Stores   │  │                           │  │  Server   │  │
│  └───────────┘  │                           │  └───────────┘  │
│       ↓         │                           │        ↓        │
│  ┌───────────┐  │                           │  ┌───────────┐  │
│  │WebSocket  │  │                           │  │WebSocket  │  │
│  │ Manager   │  │                           │  │  Server   │  │
│  └───────────┘  │                           │  └───────────┘  │
└─────────────────┘                           └─────────────────┘
```

## 前端实现

### 1. 类型定义

**文件**: `frontend/src/types/websocket.ts`

定义了完整的 WebSocket 通信协议：

```typescript
// 消息类型
enum MessageType {
  PAPER_UPDATE = 'paper_update',
  PAPER_DELETE = 'paper_delete',
  PAPER_NEW = 'paper_new',
  STATS_UPDATE = 'stats_update',
  HEARTBEAT = 'heartbeat',
  SYNC_COMPLETE = 'sync_complete',
  ERROR = 'error'
}

// 消息接口
interface WSMessage {
  type: MessageType;
  timestamp: string;
  id: string;
  data?: any;
}
```

### 2. WebSocket 管理器

**文件**: `frontend/src/composables/useWebSocket.ts`

核心功能：

- **自动重连**: 连接断开时自动尝试重连（最多10次）
- **心跳检测**: 每30秒发送心跳，90秒无响应断开
- **消息队列**: 离线时缓存消息，连接后发送
- **事件处理**: 支持多种消息类型的处理器

**使用示例**:

```typescript
import { useWebSocket } from '@/composables/useWebSocket'

const {
  connectionState,
  stats,
  isConnected,
  connect,
  disconnect,
  send
} = useWebSocket({
  url: 'ws://localhost:8088/ws',
  reconnectInterval: 3000
}, {
  onPaperUpdate: (paper) => {
    console.log('Paper updated:', paper)
  },
  onStatsUpdate: (stats) => {
    console.log('Stats updated:', stats)
  }
})
```

### 3. Pinia Store 集成

**文件**: `frontend/src/stores/websocket.ts`

全局 WebSocket 状态管理：

```typescript
import { useWebSocketStore } from '@/stores/websocket'

const wsStore = useWebSocketStore()

// 初始化连接
wsStore.initialize()

// 获取状态
wsStore.isConnected        // 是否连接
wsStore.connectionState    // 连接状态
wsStore.updateNotifications // 更新通知数
```

**文件**: `frontend/src/stores/papers.ts`

论文数据自动实时更新：

```typescript
import { usePapersStore } from '@/stores/papers'

const papersStore = usePapersStore()

// 设置 WebSocket 监听器
papersStore.setupWebSocketListeners()

// 论文数据会自动更新
papersStore.searchResults // 实时更新的搜索结果
```

### 4. 连接状态组件

**文件**: `frontend/src/components/WebSocketStatus.vue`

显示连接状态的 UI 组件：

```vue
<template>
  <WebSocketStatus />
</template>

<script setup>
import WebSocketStatus from '@/components/WebSocketStatus.vue'
</script>
```

**功能**：
- 实时连接状态指示器
- 更新通知徽章
- 连接统计信息
- 点击重连功能

## 后端实现

### 1. WebSocket 服务器

**文件**: `backend/src/websocket_server.hpp`
**文件**: `backend/src/websocket_server.cpp`

核心功能：

- **多客户端支持**: 同时处理多个 WebSocket 连接
- **消息广播**: 向所有客户端发送更新
- **心跳检测**: 定期发送心跳并检测客户端连接
- **线程安全**: 使用互斥锁保护共享数据

### 2. 使用示例

```cpp
#include "websocket_server.hpp"

// 创建服务器
WebSocketServer ws_server(8088);

// 启动服务器
ws_server.start();

// 广播论文更新
WSPaperData paper{
    .id = 123,
    .title = "Deep Learning",
    .authors = "Author Name",
    .year = 2024,
    .venue = "CVPR",
    .citation_count = 100
};
ws_server.broadcastPaperUpdate(paper);

// 广播统计更新
WSStatsData stats{
    .total_papers = 1000,
    .total_citations = 50000,
    .recent_additions = 10,
    .last_updated = "2024-03-21T12:00:00"
};
ws_server.broadcastStatsUpdate(stats);

// 获取连接数
size_t connections = ws_server.getConnectionCount();
```

### 3. 与 HTTP 服务器集成

修改现有的 HTTP 服务器以包含 WebSocket 支持：

```cpp
#include "websocket_server.hpp"

// 在主函数中启动两个服务器
int main() {
    // HTTP 服务器 (端口 8087)
    // ... 现有的 HTTP 服务器代码 ...

    // WebSocket 服务器 (端口 8088)
    WebSocketServer ws_server(8088);
    if (!ws_server.start()) {
        std::cerr << "Failed to start WebSocket server" << std::endl;
        return 1;
    }

    std::cout << "WebSocket server started on port 8088" << std::endl;

    // 主循环
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 当论文数据更新时，通过 WebSocket 广播
        if (has_paper_updates) {
            ws_server.broadcastPaperUpdate(updated_paper);
        }
    }

    return 0;
}
```

## 消息协议

### 消息格式

所有消息都是 JSON 格式：

```json
{
  "type": "message_type",
  "timestamp": "2024-03-21T12:00:00",
  "id": "unique_message_id",
  "data": { /* 消息特定数据 */ }
}
```

### 消息类型

#### 1. 论文更新

```json
{
  "type": "paper_update",
  "timestamp": "2024-03-21T12:00:00",
  "id": "msg_123",
  "data": {
    "paper": {
      "id": 123,
      "title": "Paper Title",
      "authors": "Author 1, Author 2",
      "year": 2024,
      "venue": "Conference Name",
      "citation_count": 100
    },
    "changes": ["title", "citation_count"]
  }
}
```

#### 2. 新论文

```json
{
  "type": "paper_new",
  "timestamp": "2024-03-21T12:00:00",
  "id": "msg_124",
  "data": {
    "paper": {
      "id": 124,
      "title": "New Paper",
      "authors": "Author Name",
      "year": 2024
    }
  }
}
```

#### 3. 论文删除

```json
{
  "type": "paper_delete",
  "timestamp": "2024-03-21T12:00:00",
  "id": "msg_125",
  "data": {
    "paper_id": 123
  }
}
```

#### 4. 统计更新

```json
{
  "type": "stats_update",
  "timestamp": "2024-03-21T12:00:00",
  "id": "msg_126",
  "data": {
    "total_papers": 1000,
    "total_citations": 50000,
    "papers_by_year": {
      "2024": 100,
      "2023": 200
    },
    "recent_additions": 10,
    "last_updated": "2024-03-21T12:00:00"
  }
}
```

#### 5. 心跳

```json
{
  "type": "heartbeat",
  "timestamp": "2024-03-21T12:00:00",
  "id": "msg_127",
  "data": {
    "server_time": "2024-03-21T12:00:00"
  }
}
```

#### 6. 同步完成

```json
{
  "type": "sync_complete",
  "timestamp": "2024-03-21T12:00:00",
  "id": "msg_128",
  "data": {
    "papers_count": 100,
    "duration_ms": 5000
  }
}
```

## 配置

### 前端配置

**文件**: `frontend/.env.development`

```env
VITE_WS_URL=ws://localhost:8088/ws
VITE_API_URL=http://localhost:8087
```

**文件**: `frontend/.env.production`

```env
VITE_WS_URL=wss://api.papercrawler.com/ws
VITE_API_URL=https://api.papercrawler.com
```

### 后端配置

在 C++ 代码中配置端口：

```cpp
// WebSocket 服务器端口
const int WS_PORT = 8088;

// HTTP 服务器端口
const int HTTP_PORT = 8087;
```

## 部署

### 前端部署

1. 构建前端：

```bash
cd frontend
npm run build
```

2. 配置 nginx 以支持 WebSocket：

```nginx
server {
    listen 80;
    server_name your-domain.com;

    # 前端静态文件
    location / {
        root /var/www/papercrawler/frontend/dist;
        try_files $uri $uri/ /index.html;
    }

    # WebSocket 代理
    location /ws {
        proxy_pass http://localhost:8088;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_read_timeout 3600s;
        proxy_send_timeout 3600s;
    }

    # HTTP API 代理
    location /api {
        proxy_pass http://localhost:8087;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

### 后端部署

1. 编译后端：

```bash
cd backend
mkdir build && cd build
cmake ..
make
```

2. 运行服务器：

```bash
./PaperCrawlerServer
```

3. 使用 systemd 创建服务：

**文件**: `/etc/systemd/system/papercrawler.service`

```ini
[Unit]
Description=PaperCrawler Backend Server
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/var/www/papercrawler/backend
ExecStart=/var/www/papercrawler/backend/PaperCrawlerServer
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

启动服务：

```bash
sudo systemctl daemon-reload
sudo systemctl start papercrawler
sudo systemctl enable papercrawler
```

## 测试

### 前端测试

1. 启动开发服务器：

```bash
cd frontend
npm run dev
```

2. 打开浏览器开发者工具，查看 WebSocket 连接：

```javascript
// 在浏览器控制台
import { useWebSocketStore } from '@/stores/websocket'
const wsStore = useWebSocketStore()
console.log(wsStore.connectionState)
console.log(wsStore.stats)
```

### 后端测试

1. 使用 wscat 测试 WebSocket 连接：

```bash
npm install -g wscat
wscat -c ws://localhost:8088/ws
```

2. 发送测试消息：

```json
{"type":"heartbeat","timestamp":"2024-03-21T12:00:00","id":"test"}
```

## 性能优化

### 前端优化

1. **连接池管理**: 限制同时连接数
2. **消息去重**: 避免重复处理相同消息
3. **批量更新**: 合并多个更新操作

### 后端优化

1. **连接限制**: 限制最大连接数
2. **消息压缩**: 对大消息进行压缩
3. **负载均衡**: 使用多台服务器分担负载

## 故障排除

### 常见问题

1. **连接失败**
   - 检查端口是否正确
   - 确认防火墙设置
   - 验证 WebSocket URL

2. **频繁断开**
   - 检查网络稳定性
   - 调整心跳间隔
   - 查看服务器日志

3. **消息丢失**
   - 验证消息队列配置
   - 检查网络延迟
   - 增加重试次数

### 调试

启用详细日志：

```typescript
// 前端
const ws = useWebSocket({
  url: 'ws://localhost:8088/ws',
  reconnectInterval: 3000
}, {
  onStateChange: (state) => {
    console.log('WebSocket state:', state)
  },
  onError: (error) => {
    console.error('WebSocket error:', error)
  }
})
```

```cpp
// 后端
ws_server.setMessageHandler([](const WSMessage& msg) {
    std::cout << "Received message: " << msg.toJSON() << std::endl;
});
```

## 安全考虑

1. **身份验证**: 实现 WebSocket 握手时的身份验证
2. **消息验证**: 验证所有传入消息的格式
3. **速率限制**: 限制客户端消息频率
4. **TLS/WSS**: 生产环境使用加密连接

## 未来改进

1. **消息压缩**: 减少带宽使用
2. **二进制协议**: 使用 MessagePack 替代 JSON
3. **集群支持**: 支持多服务器部署
4. **消息持久化**: 存储离线消息

## 总结

这个 WebSocket 实时同步系统为 PaperCrawler 提供了完整的实时数据同步功能：

- ✅ 自动重连和心跳检测
- ✅ 消息队列和离线缓存
- ✅ 多种消息类型支持
- ✅ 线程安全的后端实现
- ✅ Vue 3 + Pinia 集成
- ✅ 连接状态 UI 组件

系统已经可以投入使用，并为未来的扩展提供了良好的基础。
