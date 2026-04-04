# PaperCrawler 爬虫管理界面 - 快速启动指南

## 项目完成情况

已成功实现完整的爬虫管理界面，包含以下功能模块：

### 核心页面 (5个)
1. **CrawlerDashboardView.vue** - 爬虫仪表盘
2. **TemplateListView.vue** - 模板管理
3. **TemplateEditView.vue** - 模板编辑
4. **TaskListView.vue** - 任务管理
5. **NodeManagementView.vue** - 节点管理

### 专用组件 (3个)
1. **TaskProgressCard.vue** - 任务进度卡片
2. **SourceSelector.vue** - 数据源选择器
3. **RealTimeLogViewer.vue** - 实时日志查看器

### 服务层 (1个)
1. **crawlerWebSocket.ts** - WebSocket实时通信服务

## 文件位置

```
e:\PaperCrawler\frontend\
├── src/
│   ├── views/crawler/          # 爬虫管理页面
│   ├── components/crawler/     # 爬虫专用组件
│   ├── services/               # WebSocket服务
│   ├── stores/crawlerStore.ts  # 状态管理
│   └── router/index.ts         # 路由配置（已更新）
└── CRAWLER_UI_README.md        # 完整文档
```

## 快速启动

### 1. 检查依赖

确保已安装以下依赖：

```bash
cd /e/PaperCrawler/frontend

# 检查 package.json 中的依赖
# 必需:
# - vue ^3.3.0
# - element-plus ^2.4.0
# - pinia ^2.1.0
# - vue-router ^4.2.0
```

### 2. 启动开发服务器

```bash
npm run dev
```

访问: `http://localhost:5173`

### 3. 导航到爬虫管理界面

在浏览器中访问以下路径：

- 仪表盘: `http://localhost:5173/crawler/dashboard`
- 模板管理: `http://localhost:5173/crawler/templates`
- 任务管理: `http://localhost:5173/crawler/tasks`
- 节点管理: `http://localhost:5173/crawler/nodes`

## 功能演示

### 1. 创建爬虫模板

1. 进入模板管理页面 (`/crawler/templates`)
2. 点击"创建模板"按钮
3. 填写模板信息：
   - 模板名称: "深度学习论文"
   - 数据源: "arXiv"
   - 搜索关键词: "deep learning"
   - 结果数量: 20
4. 点击"测试查询"验证配置
5. 点击"保存"

### 2. 运行爬虫任务

1. 在模板列表中找到刚创建的模板
2. 点击"运行"按钮
3. 系统自动跳转到任务管理页面
4. 实时查看任务进度

### 3. 监控任务进度

1. 进入任务管理页面 (`/crawler/tasks`)
2. 查看实时任务列表
3. 点击任务旁的更多按钮可以：
   - 暂停/继续任务
   - 取消任务
   - 查看日志
   - 重试失败任务

### 4. 管理爬虫节点

1. 进入节点管理页面 (`/crawler/nodes`)
2. 查看所有节点的健康状态
3. 点击"添加节点"添加新节点
4. 测试节点连接
5. 查看节点性能指标

## WebSocket 实时更新

任务管理页面集成了WebSocket实时更新功能：

```typescript
// WebSocket会自动连接
// 在任务管理页面可以看到实时进度更新

// 手动使用示例：
import { useCrawlerWebSocket } from '@/services/crawlerWebSocket'

const { connect, subscribeToTask } = useCrawlerWebSocket()

await connect()
subscribeToTask('your_task_id')
```

## 环境配置

创建 `.env` 文件：

```bash
VITE_API_BASE_URL=http://localhost:8080
VITE_WS_BASE_URL=ws://localhost:8080
```

## 后端API要求

确保后端实现以下API端点：

```typescript
// 爬虫任务
POST   /api/crawler/search           # 启动搜索任务
GET    /api/crawler/task/:id         # 获取任务状态
DELETE /api/crawler/task/:id         # 取消任务
POST   /api/crawler/task/:id/pause   # 暂停任务
POST   /api/crawler/task/:id/resume  # 恢复任务

// 数据管理
GET    /api/crawler/history          # 获取历史记录
POST   /api/crawler/save             # 保存论文
GET    /api/crawler/stats            # 获取统计信息

// 配置管理
GET    /api/crawler/config           # 获取配置
PUT    /api/crawler/config           # 更新配置
GET    /api/crawler/sources          # 获取支持的数据源
GET    /api/crawler/test/:source     # 测试连接

// WebSocket
WS     /api/crawler/ws               # 实时更新
```

## 故障排除

### 1. 路由404错误

确保路由配置正确：

```typescript
// src/router/index.ts
{
  path: '/crawler/dashboard',
  name: 'CrawlerDashboard',
  component: () => import('@/views/crawler/CrawlerDashboardView.vue')
}
```

### 2. 组件导入错误

使用以下方式导入组件：

```typescript
// 单个导入
import TaskProgressCard from '@/components/crawler/TaskProgressCard.vue'

// 批量导入
import { TaskProgressCard, SourceSelector } from '@/components/crawler'
```

### 3. WebSocket连接失败

检查：

1. 后端WebSocket服务是否启动
2. 环境变量 `VITE_WS_BASE_URL` 是否正确
3. 浏览器控制台是否有CORS错误

### 4. 样式问题

确保Element Plus样式已导入：

```typescript
// main.ts
import 'element-plus/dist/index.css'
```

## 性能优化建议

1. **启用生产构建**:
```bash
npm run build
```

2. **代码分割**: 已实现路由级别的代码分割

3. **图片优化**: 使用WebP格式，添加懒加载

4. **缓存策略**: 配置Service Worker缓存静态资源

## 浏览器兼容性

- Chrome/Edge 90+
- Firefox 88+
- Safari 14+
- 移动端浏览器

## 下一步

1. 根据实际后端API调整接口调用
2. 添加单元测试
3. 配置CI/CD流程
4. 优化生产环境构建
5. 添加国际化支持

## 支持

如有问题，请参考完整文档: `CRAWLER_UI_README.md`

---

**PaperCrawler Development Team**
2024年4月
