# PaperCrawler 爬虫管理界面

完整的爬虫管理界面实现，包含仪表盘、模板管理、任务监控和节点管理等功能。

## 功能特性

### 1. 爬虫仪表盘 (CrawlerDashboardView)
- ✅ 实时统计卡片（任务总数、活跃任务、成功率、论文总数）
- ✅ 实时任务列表（显示前5个活跃任务）
- ✅ 任务状态分布图表
- ✅ 节点健康状态监控
- ✅ 最近活动日志
- ✅ 快速操作按钮

### 2. 模板管理 (TemplateListView)
- ✅ 表格/网格双视图模式
- ✅ 模板搜索和过滤
- ✅ 批量操作（删除、导出）
- ✅ 模板操作（运行、编辑、导出、删除）
- ✅ 模板状态徽章
- ✅ 使用次数统计

### 3. 模板编辑 (TemplateEditView)
- ✅ 基本信息表单（名称、数据源、关键词）
- ✅ 高级选项配置（摘要、全文、日期范围）
- ✅ 实时查询测试工具
- ✅ 示例结果预览
- ✅ 验证提示
- ✅ 数据源连接状态

### 4. 任务管理 (TaskListView)
- ✅ 实时任务列表（WebSocket监控）
- ✅ 任务状态分类（全部、运行中、已完成、失败）
- ✅ 任务进度条
- ✅ 日志查看抽屉
- ✅ 任务操作（暂停、继续、取消、重试）
- ✅ 批量操作
- ✅ 实时更新指示器

### 5. 节点管理 (NodeManagementView)
- ✅ 节点列表（网格/列表视图）
- ✅ 节点状态监控
- ✅ 性能指标展示
- ✅ 节点操作（编辑、启用/禁用、测试、删除）
- ✅ 节点详情对话框
- ✅ 统计图表（性能趋势、任务分布）

### 6. WebSocket 实时服务 (crawlerWebSocket.ts)
- ✅ 自动重连机制
- ✅ 心跳保活
- ✅ 事件订阅系统
- ✅ 消息处理
- ✅ 连接状态管理
- ✅ Vue 3 组合式API (useCrawlerWebSocket)

### 7. 专用组件库
- ✅ **TaskProgressCard**: 任务进度卡片组件
- ✅ **SourceSelector**: 数据源选择器组件
- ✅ **RealTimeLogViewer**: 实时日志查看器组件

## 文件结构

```
frontend/
├── src/
│   ├── views/
│   │   └── crawler/
│   │       ├── CrawlerDashboardView.vue    # 仪表盘
│   │       ├── TemplateListView.vue         # 模板列表
│   │       ├── TemplateEditView.vue         # 模板编辑
│   │       ├── TaskListView.vue             # 任务列表
│   │       └── NodeManagementView.vue       # 节点管理
│   ├── components/
│   │   └── crawler/
│   │       ├── TaskProgressCard.vue         # 任务进度卡片
│   │       ├── SourceSelector.vue           # 数据源选择器
│   │       ├── RealTimeLogViewer.vue        # 实时日志查看器
│   │       └── index.ts                     # 组件导出
│   ├── services/
│   │   └── crawlerWebSocket.ts              # WebSocket服务
│   ├── stores/
│   │   └── crawlerStore.ts                  # 爬虫状态管理
│   ├── api/
│   │   └── modules/
│   │       └── crawler.ts                   # 爬虫API
│   └── router/
│       └── index.ts                         # 路由配置
```

## 路由配置

```typescript
// 爬虫管理路由
{
  path: '/crawler',
  name: 'Crawler',
  redirect: '/crawler/dashboard',
  meta: { requiresAuth: true, title: 'Crawler Management', icon: 'Connection' }
},
{
  path: '/crawler/dashboard',
  name: 'CrawlerDashboard',
  component: () => import('@/views/crawler/CrawlerDashboardView.vue'),
  meta: { requiresAuth: true, title: 'Crawler Dashboard', icon: 'Odometer' }
},
{
  path: '/crawler/templates',
  name: 'TemplateList',
  component: () => import('@/views/crawler/TemplateListView.vue'),
  meta: { requiresAuth: true, title: 'Template Management', icon: 'Grid' }
},
{
  path: '/crawler/templates/new',
  name: 'TemplateCreate',
  component: () => import('@/views/crawler/TemplateEditView.vue'),
  meta: { requiresAuth: true, title: 'Create Template' }
},
{
  path: '/crawler/templates/:id/edit',
  name: 'TemplateEdit',
  component: () => import('@/views/crawler/TemplateEditView.vue'),
  meta: { requiresAuth: true, title: 'Edit Template' }
},
{
  path: '/crawler/tasks',
  name: 'TaskList',
  component: () => import('@/views/crawler/TaskListView.vue'),
  meta: { requiresAuth: true, title: 'Task Management', icon: 'List' }
},
{
  path: '/crawler/nodes',
  name: 'NodeManagement',
  component: () => import('@/views/crawler/NodeManagementView.vue'),
  meta: { requiresAuth: true, title: 'Node Management', icon: 'Monitor' }
}
```

## 使用示例

### 1. 使用爬虫Store

```typescript
import { useCrawlerStore } from '@/stores/crawlerStore'

const crawlerStore = useCrawlerStore()

// 启动爬虫任务
const task = await crawlerStore.startTask({
  query: 'deep learning',
  source: 'arxiv',
  limit: 20
})

// 获取任务状态
const status = await crawlerStore.fetchTaskStatus(task.id)

// 取消任务
await crawlerStore.cancelTask(task.id)

// 获取统计信息
const stats = await crawlerStore.fetchStats()
```

### 2. 使用WebSocket实时更新

```typescript
import { useCrawlerWebSocket } from '@/services/crawlerWebSocket'

const {
  connected,
  connect,
  disconnect,
  subscribeToTask,
  onMessage
} = useCrawlerWebSocket()

// 连接WebSocket
await connect()

// 订阅任务更新
subscribeToTask('task_id')

// 监听消息
onMessage((message) => {
  if (message.type === 'task_update') {
    console.log('Task updated:', message.data)
  }
})

// 断开连接
disconnect()
```

### 3. 使用爬虫组件

```vue
<template>
  <!-- 任务进度卡片 -->
  <TaskProgressCard
    :task="task"
    @pause="handlePause"
    @resume="handleResume"
    @cancel="handleCancel"
  />

  <!-- 数据源选择器 -->
  <SourceSelector
    v-model="selectedSource"
    :show-test-button="true"
  />

  <!-- 实时日志查看器 -->
  <RealTimeLogViewer
    :logs="logs"
    :is-live="true"
    @clear="handleClearLogs"
    @export="handleExportLogs"
  />
</template>

<script setup lang="ts">
import { TaskProgressCard, SourceSelector, RealTimeLogViewer } from '@/components/crawler'
</script>
```

## API集成

### 后端API端点

- `POST /api/crawler/search` - 通用搜索接口
- `GET /api/crawler/task/:id` - 获取任务状态
- `DELETE /api/crawler/task/:id` - 取消任务
- `POST /api/crawler/task/:id/pause` - 暂停任务
- `POST /api/crawler/task/:id/resume` - 恢复任务
- `GET /api/crawler/history` - 获取历史记录
- `GET /api/crawler/config` - 获取配置
- `PUT /api/crawler/config` - 更新配置
- `GET /api/crawler/stats` - 获取统计信息
- `GET /api/crawler/test/:source` - 测试连接
- `GET /api/crawler/sources` - 获取支持的数据源
- `WS /api/crawler/ws` - WebSocket实时更新

## 响应式设计

所有页面都采用移动优先的响应式设计：

- **桌面端** (>1024px): 完整功能布局
- **平板端** (768px-1024px): 优化布局
- **移动端** (<768px): 单列布局，优化触摸操作

## 性能优化

1. **虚拟滚动**: 大列表使用虚拟滚动减少DOM节点
2. **代码分割**: 路由级别的代码分割
3. **懒加载**: 图片和组件按需加载
4. **防抖节流**: 搜索和滚动事件优化
5. **WebSocket优化**: 消息批量处理，减少重渲染

## 浏览器支持

- Chrome/Edge >= 90
- Firefox >= 88
- Safari >= 14
- 移动端浏览器

## 开发建议

1. **环境变量配置**:
```bash
VITE_WS_BASE_URL=ws://localhost:8080
VITE_API_BASE_URL=http://localhost:8080
```

2. **依赖安装**:
```bash
npm install element-plus @element-plus/icons-vue
npm install pinia pinia-plugin-persistedstate
```

3. **开发服务器**:
```bash
npm run dev
```

## 后续扩展

- [ ] 添加任务调度功能
- [ ] 实现任务依赖关系
- [ ] 添加更多数据源支持
- [ ] 实现任务结果导出（Excel、CSV）
- [ ] 添加任务性能分析
- [ ] 实现分布式爬虫节点管理
- [ ] 添加爬虫规则可视化编辑器

## 许可证

MIT License

## 作者

PaperCrawler Development Team
