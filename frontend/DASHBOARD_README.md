# PaperCrawler 主仪表盘

## 概述

PaperCrawler 主仪表盘是一个功能完整、视觉美观的仪表盘系统，为用户提供全面的论文管理和研究活动概览。

## 功能特性

### 1. 欢迎横幅 (WelcomeBanner)
- **用户问候**: 根据时间显示不同的问候语（早上好、下午好、晚上好等）
- **日期显示**: 显示当前日期和星期
- **天气信息**: 可选的天气信息展示
- **快速操作**: 提供开始使用和功能导览按钮

### 2. 统计卡片 (StatCard)
- **论文总数**: 数据库中的论文总数，带增长趋势
- **本周新增**: 本周新增论文数量
- **收藏数量**: 用户收藏的论文数量
- **导出次数**: 累计导出次数
- **可视化效果**: 渐变色图标、趋势标签、悬停动画

### 3. 数据可视化图表 (ChartContainer)

#### 论文增长趋势图
- **图表类型**: 折线图
- **数据维度**: 论文总数、新增论文
- **交互特性**: 工具提示、图例切换、响应式缩放
- **动画效果**: 平滑的增长曲线动画

#### 期刊分布图
- **图表类型**: 环形图
- **数据展示**: 各期刊论文数量分布
- **颜色方案**: 7种渐变色
- **交互**: 点击图例隐藏/显示数据

#### CCF等级分布图
- **图表类型**: 柱状图
- **数据分类**: CCF-A、CCF-B、CCF-C
- **视觉效果**: 圆角柱体、渐变填充
- **响应式**: 自适应容器大小

### 4. 快速操作 (QuickActions)
- **添加论文**: 快速跳转到论文添加页面
- **创建爬虫**: 创建新的爬虫任务
- **导出数据**: 导出论文数据
- **查看统计**: 查看详细统计信息
- **视觉设计**: 卡片式布局、图标+文字描述

### 5. 最近活动 (RecentActivity)
- **活动类型**:
  - 论文添加
  - 搜索历史
  - 导出记录
  - 收藏操作
  - 爬虫创建
- **时间显示**: 相对时间（如"2小时前"）
- **活动图标**: 不同类型使用不同颜色和图标
- **快速跳转**: 点击活动可跳转到相关页面

### 6. 推荐内容 (RecommendedContent)

#### 推荐论文
- **个性化推荐**: 基于用户兴趣的论文推荐
- **推荐理由**: 显示推荐原因
- **推荐分数**: 星级评分系统
- **快速访问**: 点击跳转到论文详情

#### 热门搜索
- **搜索排名**: Top 10 热门搜索关键词
- **搜索次数**: 显示搜索次数
- **趋势指示**: 上升/下降/平稳趋势图标
- **快速搜索**: 点击关键词直接搜索

### 7. 待办事项 (TodoList)
- **优先级**: 高/中/低优先级，用不同颜色标识
- **截止日期**: 显示截止日期，逾期提醒
- **状态管理**: 勾选完成/恢复待办
- **快速操作**: 处理按钮跳转到相关页面

## 组件架构

```
src/
├── views/dashboard/
│   └── DashboardView.vue          # 主仪表盘页面
├── components/dashboard/
│   ├── WelcomeBanner.vue          # 欢迎横幅
│   ├── StatCard.vue               # 统计卡片
│   ├── ChartContainer.vue         # 图表容器
│   ├── QuickActions.vue           # 快速操作
│   ├── RecentActivity.vue         # 最近活动
│   ├── RecommendedContent.vue     # 推荐内容
│   ├── TodoList.vue               # 待办事项
│   └── index.ts                   # 组件导出
├── types/
│   └── dashboard.ts               # 仪表盘类型定义
└── api/modules/
    └── dashboard.ts               # 仪表盘API
```

## 技术栈

- **Vue 3**: Composition API、TypeScript
- **Element Plus**: UI组件库
- **Chart.js**: 图表库
- **date-fns**: 日期处理
- **Vue Router**: 路由管理
- **Pinia**: 状态管理

## 设计系统

### 颜色方案
- **主色调**: 紫色渐变 (#667eea → #764ba2)
- **成功色**: 粉红渐变 (#f093fb → #f5576c)
- **警告色**: 蓝色渐变 (#4facfe → #00f2fe)
- **危险色**: 橙红渐变 (#fa709a → #fee140)
- **信息色**: 青紫渐变 (#30cfd0 → #330867)

### 间距系统
- **基础单位**: 4px
- **卡片间距**: 20px
- **元素间距**: 12px、16px、24px
- **页面边距**: 24px

### 字体大小
- **标题**: 32px、24px、20px、16px
- **正文**: 14px、13px、12px
- **辅助文字**: 12px、11px

### 圆角
- **卡片**: 12px、8px
- **按钮**: 4px
- **标签**: 4px

### 阴影
- **卡片悬停**: 0 8px 24px rgba(0, 0, 0, 0.12)
- **按钮悬停**: 0 4px 12px rgba(0, 0, 0, 0.1)

## 响应式设计

### 断点
- **移动端**: < 768px
- **平板**: 768px - 1024px
- **桌面**: 1024px - 1280px
- **大屏**: > 1280px

### 适配策略
- **移动优先**: 基础样式为移动端，向上扩展
- **弹性布局**: 使用Flexbox和Grid
- **组件自适应**: 组件内部响应式处理
- **图表自适应**: Chart.js自动调整大小

## API集成

### 已集成API
- `GET /api/stats` - 统计数据
- `GET /api/papers` - 论文列表
- `GET /api/recommendations/papers` - 推荐论文
- `GET /api/recommendations/trending` - 热门搜索

### 待实现API
- `GET /api/dashboard/stats` - 仪表盘统计
- `GET /api/dashboard/activities` - 最近活动
- `GET /api/dashboard/todos` - 待办事项
- `GET /api/dashboard/growth` - 增长趋势
- `GET /api/dashboard/distribution/journals` - 期刊分布
- `GET /api/dashboard/distribution/ccf` - CCF分布

## 使用说明

### 路由访问
```typescript
// 访问主仪表盘
router.push('/dashboard')

// 访问旧版仪表盘（对比）
router.push('/dashboard-old')
```

### 组件使用
```vue
<template>
  <DashboardView />
</template>

<script setup lang="ts">
import DashboardView from '@/views/dashboard/DashboardView.vue'
</script>
```

### 单独使用组件
```vue
<template>
  <StatCard
    :value="1234"
    label="论文总数"
    :icon="Document"
    type="primary"
    :trend="12.5"
    description="数据库中论文总数"
  />
</template>

<script setup lang="ts">
import StatCard from '@/components/dashboard/StatCard.vue'
import { Document } from '@element-plus/icons-vue'
</script>
```

## 自定义配置

### 修改显示数据量
```typescript
// DashboardView.vue
const displayCount = ref(5) // 修改显示数量
```

### 修改图表配置
```typescript
// 修改图表选项
const growthChartOptions = computed<ChartOptions>(() => ({
  // 自定义配置
}))
```

### 修改颜色方案
```scss
// 在组件的style标签中修改CSS变量
.stat-card--primary .stat-card__icon {
  background: linear-gradient(135deg, #your-color-1 0%, #your-color-2 100%);
}
```

## 性能优化

- **懒加载**: 路由级别的代码分割
- **数据缓存**: API响应缓存
- **图表优化**: Canvas渲染，避免DOM操作
- **虚拟滚动**: 大列表使用虚拟滚动
- **防抖节流**: 搜索和调整事件使用防抖

## 可访问性

- **键盘导航**: 所有交互支持键盘操作
- **屏幕阅读器**: 语义化HTML和ARIA标签
- **颜色对比**: 符合WCAG AA标准
- **焦点管理**: 清晰的焦点指示器
- **触摸目标**: 最小44px触摸区域

## 未来扩展

### 计划功能
- [ ] 自定义仪表盘布局（拖拽排序）
- [ ] 主题切换（暗色模式）
- [ ] 数据导出（PDF/图片）
- [ ] 实时数据更新（WebSocket）
- [ ] 更多图表类型（雷达图、热力图）
- [ ] 个性化推荐算法
- [ ] 数据钻取功能
- [ ] 对比分析功能

### 优化方向
- [ ] 性能优化（减少重渲染）
- [ ] 离线支持（Service Worker）
- [ ] PWA支持（安装到桌面）
- [ ] 多语言支持（i18n）

## 文件路径

- **主页面**: `e:\PaperCrawler\frontend\src\views\dashboard\DashboardView.vue`
- **组件目录**: `e:\PaperCrawler\frontend\src\components\dashboard\`
- **类型定义**: `e:\PaperCrawler\frontend\src\types\dashboard.ts`
- **API模块**: `e:\PaperCrawler\frontend\src\api\modules\dashboard.ts`

## 作者

UI Designer Agent - PaperCrawler 仪表盘系统

## 版本

1.0.0 - 初始版本
