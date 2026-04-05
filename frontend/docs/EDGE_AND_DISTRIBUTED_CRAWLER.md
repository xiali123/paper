# 边缘爬虫和分布式爬虫前端

## 创建日期
2026-04-06

## 功能概述

### 边缘爬虫 (Edge Crawler)
**路径**: `/crawler/edge`

允许用户在浏览器中直接爬取学术数据，然后同步到服务器。

**核心功能**:
- 在浏览器中直接爬取URL（无需后端）
- 支持Arxiv、Google Scholar等学术网站
- 本地管理爬取结果
- 选择性同步到服务器
- 自动同步选项

**使用场景**:
- 快速爬取少量论文
- 网络受限环境
- 减轻服务器负载
- 实时预览爬取结果

### 分布式爬虫管理器 (Distributed Crawler)
**路径**: `/crawler/distributed`

管理和监控分布式爬虫集群。

**核心功能**:
- 工作节点管理
- 任务队列监控
- 集群状态仪表板
- 任务分配和调度
- 实时进度跟踪

**仪表板指标**:
- 工作节点数量和状态
- 任务总数和执行状态
- 论文总数和今日新增
- 平均响应时间和吞吐量

## 文件结构

```
frontend/src/
├── views/crawler/
│   ├── EdgeCrawlerView.vue          # 边缘爬虫主界面
│   └── DistributedCrawlerView.vue    # 分布式爬虫管理界面
├── services/
│   └── crawlerApi.ts                 # 爬虫API（含边缘爬取）
└── router/
    └── index.ts                      # 路由配置
```

## 边缘爬虫使用流程

### 1. 创建爬取任务
```
1. 点击"新建爬取任务"
2. 输入任务名称
3. 输入目标URL（如: https://arxiv.org/list/cs.AI/recent）
4. 设置爬取数量（最多100篇）
5. 选择是否自动同步
6. 点击"创建并开始"
```

### 2. 查看爬取结果
- 左侧面板显示所有任务
- 点击任务查看爬取结果
- 每篇论文显示标题、作者、摘要

### 3. 同步到服务器
- 单篇同步: 点击论文的"同步"按钮
- 批量同步: 勾选论文后点击"同步选中"
- 自动同步: 创建任务时启用"自动同步"

## 分布式爬虫使用流程

### 1. 查看集群状态
- 顶部仪表板显示集群概况
- 4个关键指标卡片

### 2. 管理工作节点
- 左侧面板显示所有节点
- 点击节点查看详细信息
- 支持: 禁用/启用节点
- 查看节点当前任务
- 查看节点性能指标

### 3. 创建分布式任务
```
1. 点击"创建分布式任务"
2. 选择爬虫模板
3. 设置任务优先级
4. 选择目标节点（或自动分配）
5. 配置任务参数
6. 点击"创建任务"
```

### 4. 监控任务执行
- 中间面板显示任务队列
- 按状态筛选任务
- 点击任务查看详情
- 实时进度跟踪

## API接口

### 边缘爬虫API

```typescript
// 浏览器端爬取
edgeCrawlerApi.crawlUrl(url: string, maxPapers: number)
  => Promise<Paper[]>

// 同步到服务器
edgeCrawlerApi.syncPapers(papers: Paper[])
  => Promise<{synced: number, failed: number}>

// 本地存储管理
edgeCrawlerApi.getEdgeTasks() => any[]
edgeCrawlerApi.saveEdgeTasks(tasks: any[]) => void
```

### 分布式爬虫API

```typescript
// 获取工作节点列表
GET /api/crawler/workers

// 获取任务列表
GET /api/crawler/tasks

// 创建任务
POST /api/crawler/tasks

// 分配任务到节点
POST /api/crawler/tasks/:id/assign

// 取消任务
DELETE /api/crawler/tasks/:id

// 重试失败任务
POST /api/crawler/tasks/:id/retry

// 获取集群统计
GET /api/crawler/statistics
```

## 技术特点

### 边缘爬虫
1. **浏览器原生爬取**
   - 使用fetch API
   - DOMParser解析HTML
   - 无需后端参与

2. **本地存储**
   - 使用localStorage缓存
   - 离线可用
   - 数据持久化

3. **按需同步**
   - 选择性上传
   - 批量操作
   - 失败重试

### 分布式爬虫
1. **实时监控**
   - WebSocket连接
   - 实时状态更新
   - 进度跟踪

2. **智能调度**
   - 自动节点选择
   - 负载均衡
   - 故障转移

3. **可视化展示**
   - 仪表板统计
   - 节点状态
   - 任务进度

## 样式特点

### 响应式布局
- 支持桌面和移动设备
- 自适应网格布局
- 灵活的卡片组件

### 交互设计
- 拖拽选择
- 右键菜单
- 快捷操作
- 状态指示

### 颜色系统
- 成功: 绿色 (#67c23a)
- 警告: 橙色 (#e6a23c)
- 危险: 红色 (#f56c6c)
- 信息: 蓝色 (#409eff)

## 后续优化

1. **边缘爬虫**
   - 支持更多数据源
   - 导出为CSV/JSON
   - 爬取模板系统

2. **分布式爬虫**
   - 节点地图可视化
   - 任务依赖关系图
   - 自动扩缩容

3. **通用优化**
   - 离线支持
   - 数据导出
   - 性能优化
