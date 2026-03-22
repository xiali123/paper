# WebSocket 实时同步快速启动指南

## 快速开始

### 1. 前端设置

```bash
cd frontend

# 安装依赖（如果还没有安装）
npm install

# 配置环境变量
cat > .env.development << EOF
VITE_WS_URL=ws://localhost:8088/ws
VITE_API_URL=http://localhost:8087
EOF

# 启动开发服务器
npm run dev
```

### 2. 后端设置

```bash
cd backend

# 编译 WebSocket 服务器
mkdir -p build && cd build
cmake ..
make

# 运行服务器
./PaperCrawlerServer
```

### 3. 验证连接

启动前端后，打开浏览器控制台，应该看到：

```
[WebSocket] Connecting to ws://localhost:8088/ws
[WebSocket] Connected to ws://localhost:8088/ws
```

## 使用示例

### 在 Vue 组件中使用

```vue
<script setup lang="ts">
import { onMounted } from 'vue'
import { useWebSocketStore } from '@/stores/websocket'
import { usePapersStore } from '@/stores/papers'

const wsStore = useWebSocketStore()
const papersStore = usePapersStore()

onMounted(() => {
  // 初始化 WebSocket 连接
  wsStore.initialize()

  // 设置论文数据的实时更新监听
  papersStore.setupWebSocketListeners()
})
</script>

<template>
  <div>
    <!-- 显示连接状态 -->
    <WebSocketStatus />

    <!-- 显示论文列表（会自动更新） -->
    <div v-for="paper in papersStore.searchResults" :key="paper.id">
      {{ paper.title }}
    </div>
  </div>
</template>
```

### 监听特定事件

```typescript
import { useWebSocket } from '@/composables/useWebSocket'

const { send } = useWebSocket({}, {
  onPaperUpdate: (paper) => {
    console.log('Paper updated:', paper.title)
    // 显示通知或更新 UI
  },
  onPaperNew: (paper) => {
    console.log('New paper:', paper.title)
    // 添加到列表或显示通知
  },
  onStatsUpdate: (stats) => {
    console.log('Stats updated:', stats.total_papers)
    // 更新统计显示
  }
})
```

### 手动发送消息

```typescript
import { useWebSocketStore } from '@/stores/websocket'

const wsStore = useWebSocketStore()

// 检查连接状态
if (wsStore.isConnected) {
  // 连接正常
  console.log('Connection uptime:', wsStore.getConnectionUptime())
} else {
  // 尝试重连
  wsStore.reconnect()
}
```

## 测试实时功能

### 1. 测试连接状态

在浏览器控制台：

```javascript
// 获取 WebSocket store
import { useWebSocketStore } from '@/stores/websocket'
const wsStore = useWebSocketStore()

// 检查状态
console.log('Connected:', wsStore.isConnected)
console.log('State:', wsStore.connectionState)
console.log('Stats:', wsStore.stats)
```

### 2. 测试后端广播

在后端代码中添加测试广播：

```cpp
// 测试论文更新
WSPaperData test_paper{
    .id = 999,
    .title = "Test Paper",
    .authors = "Test Author",
    .year = 2024,
    .venue = "Test Venue",
    .citation_count = 42
};
ws_server.broadcastPaperUpdate(test_paper);

// 测试统计更新
WSStatsData test_stats{
    .total_papers = 1000,
    .total_citations = 50000,
    .recent_additions = 5,
    .last_updated = "2024-03-21T12:00:00"
};
ws_server.broadcastStatsUpdate(test_stats);
```

### 3. 使用 wscat 测试

```bash
# 安装 wscat
npm install -g wscat

# 连接到 WebSocket 服务器
wscat -c ws://localhost:8088/ws

# 发送心跳消息
{"type":"heartbeat","timestamp":"2024-03-21T12:00:00","id":"test_123"}
```

## 集成到现有页面

### 1. 在主应用中初始化

修改 `frontend/src/main.ts`：

```typescript
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import { useWebSocketStore } from './stores/websocket'

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)
app.mount('#app')

// 初始化 WebSocket 连接
const wsStore = useWebSocketStore()
wsStore.initialize()
```

### 2. 在布局中添加状态指示器

在 `frontend/src/App.vue` 中：

```vue
<template>
  <div id="app">
    <header>
      <h1>PaperCrawler</h1>
      <WebSocketStatus />
    </header>

    <main>
      <router-view />
    </main>
  </div>
</template>

<script setup lang="ts">
import WebSocketStatus from '@/components/WebSocketStatus.vue'
</script>
```

### 3. 在论文列表页面中监听更新

```vue
<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import { usePapersStore } from '@/stores/papers'

const papersStore = usePapersStore()

onMounted(() => {
  // 设置 WebSocket 监听器
  papersStore.setupWebSocketListeners()
})

onUnmounted(() => {
  // 清理监听器
  papersStore.cleanupWebSocketListeners()
})
</script>

<template>
  <div>
    <!-- 如果有实时更新，显示提示 -->
    <v-alert
      v-if="papersStore.hasRealtimeUpdates"
      type="info"
      closable
      @click:close="papersStore.hasRealtimeUpdates = false"
    >
      Data has been updated with real-time changes
    </v-alert>

    <!-- 论文列表 -->
    <div v-for="paper in papersStore.searchResults" :key="paper.id">
      <h3>{{ paper.title }}</h3>
      <p>{{ paper.authors }}</p>
    </div>
  </div>
</template>
```

## 调试技巧

### 1. 启用详细日志

```typescript
// 在浏览器控制台
localStorage.setItem('debug', 'websocket:*')

// 重新加载页面
location.reload()
```

### 2. 监控网络流量

1. 打开浏览器开发者工具
2. 切换到 "Network" 标签
3. 过滤 "WS" (WebSocket)
4. 查看消息流

### 3. 检查连接状态

```javascript
// 定期检查连接状态
setInterval(() => {
  const wsStore = useWebSocketStore()
  console.log('Connection:', wsStore.connectionState)
  console.log('Messages received:', wsStore.stats.messagesReceived)
  console.log('Messages sent:', wsStore.stats.messagesSent)
}, 5000)
```

## 常见问题解决

### 问题：无法连接到 WebSocket 服务器

**解决方案**：

1. 检查后端是否启动：
```bash
# Linux/Mac
lsof -i :8088

# Windows
netstat -an | findstr :8088
```

2. 检查防火墙设置
3. 确认 URL 配置正确

### 问题：频繁断开重连

**解决方案**：

1. 调整心跳间隔：
```typescript
const ws = useWebSocket({
  heartbeatInterval: 60000 // 60秒
})
```

2. 检查网络稳定性
3. 增加重连延迟

### 问题：消息丢失

**解决方案**：

1. 启用消息队列（已默认启用）
2. 检查网络延迟
3. 实现消息确认机制

## 性能优化

### 1. 减少更新频率

```typescript
import { debounce } from 'lodash-es'

const debouncedUpdate = debounce(() => {
  // 更新 UI
}, 1000)

// 在消息处理器中使用
onPaperUpdate: (paper) => {
  debouncedUpdate()
}
```

### 2. 批量处理更新

```typescript
let updateBatch: PaperData[] = []

setInterval(() => {
  if (updateBatch.length > 0) {
    // 批量处理
    processBatch(updateBatch)
    updateBatch = []
  }
}, 5000)

onPaperUpdate: (paper) => {
  updateBatch.push(paper)
}
```

### 3. 使用虚拟滚动

对于大量数据，使用虚拟滚动组件：

```vue
<template>
  <RecycleScroller
    :items="papersStore.searchResults"
    :item-size="100"
    key-field="id"
  >
    <template #default="{ item }">
      <div>{{ item.title }}</div>
    </template>
  </RecycleScroller>
</template>
```

## 下一步

1. **添加身份验证**：实现 WebSocket 握手时的 token 验证
2. **实现消息压缩**：使用 gzip 或 brotli 压缩消息
3. **添加离线支持**：使用 Service Worker 缓存数据
4. **监控和分析**：添加 WebSocket 连接监控

## 支持

如有问题，请查看详细文档：`WEBSOCKET_IMPLEMENTATION.md`

或提交 Issue：https://github.com/yourusername/PaperCrawler/issues
