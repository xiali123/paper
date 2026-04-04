# PaperCrawler 仪表盘快速开始指南

## 5分钟快速体验

### 步骤1: 启动开发服务器

```bash
cd frontend
npm install  # 首次运行需要安装依赖
npm run dev
```

### 步骤2: 访问仪表盘

打开浏览器访问: `http://localhost:5173/dashboard`

## 组件快速使用

### 1. 使用欢迎横幅

```vue
<script setup lang="ts">
import WelcomeBanner from '@/components/dashboard/WelcomeBanner.vue'

const handleStart = () => {
  console.log('开始使用')
}
</script>

<template>
  <WelcomeBanner
    user-name="研究者"
    @start="handleStart"
  />
</template>
```

### 2. 使用统计卡片

```vue
<script setup lang="ts">
import StatCard from '@/components/dashboard/StatCard.vue'
import { Document } from '@element-plus/icons-vue'

const stats = ref(1234)
const trend = ref(12.5)
</script>

<template>
  <StatCard
    :value="stats"
    label="论文总数"
    :icon="Document"
    type="primary"
    :trend="trend"
    description="数据库中论文总数"
  />
</template>
```

### 3. 使用图表容器

```vue
<script setup lang="ts">
import ChartContainer from '@/components/dashboard/ChartContainer.vue'
import { TrendCharts } from '@element-plus/icons-vue'

const chartData = {
  labels: ['1月', '2月', '3月', '4月', '5月', '6月'],
  datasets: [{
    label: '论文数量',
    data: [120, 190, 300, 500, 200, 300],
    borderColor: '#667eea',
    backgroundColor: 'rgba(102, 126, 234, 0.1)',
  }]
}

const handleRefresh = () => {
  console.log('刷新数据')
}
</script>

<template>
  <ChartContainer
    title="论文增长趋势"
    type="line"
    :data="chartData"
    :height="350"
    :icon="TrendCharts"
    @refresh="handleRefresh"
  />
</template>
```

### 4. 使用快速操作

```vue
<script setup lang="ts">
import QuickActions from '@/components/dashboard/QuickActions.vue'
</script>

<template>
  <QuickActions />
</template>
```

### 5. 使用最近活动

```vue
<script setup lang="ts">
import RecentActivity from '@/components/dashboard/RecentActivity.vue'

const activities = ref([
  {
    id: '1',
    type: 'paper_added',
    title: '添加了论文《深度学习》',
    description: '来自 CVPR 2024',
    timestamp: new Date().toISOString(),
    data: { link: '/papers/123' }
  },
  // ...更多活动
])

const handleViewAll = () => {
  console.log('查看全部')
}
</script>

<template>
  <RecentActivity
    :activities="activities"
    @view-all="handleViewAll"
  />
</template>
```

### 6. 使用推荐内容

```vue
<script setup lang="ts">
import RecommendedContent from '@/components/dashboard/RecommendedContent.vue'

const papers = ref([
  {
    paper: {
      id: '1',
      title: '深度学习基础',
      journal: 'CVPR',
      year: '2024'
    },
    score: 4.5,
    reason: '基于你的研究兴趣'
  }
  // ...更多论文
])

const trendingSearches = ref([
  { keyword: '深度学习', count: 1234, trend: 'up' },
  { keyword: '自然语言处理', count: 987, trend: 'up' }
  // ...更多搜索
])

const handleSearch = (keyword: string) => {
  console.log('搜索:', keyword)
}
</script>

<template>
  <RecommendedContent
    :papers="papers"
    :trending-searches="trendingSearches"
    @search="handleSearch"
  />
</template>
```

### 7. 使用待办事项

```vue
<script setup lang="ts">
import TodoList from '@/components/dashboard/TodoList.vue'

const todos = ref([
  {
    id: '1',
    title: '审查新提交的论文',
    description: '需要审查3篇新提交的论文',
    priority: 'high',
    status: 'pending',
    dueDate: new Date().toISOString()
  }
  // ...更多待办
])

const handleToggleStatus = (todo) => {
  console.log('切换状态:', todo)
}
</script>

<template>
  <TodoList
    :todo-items="todos"
    @toggle-status="handleToggleStatus"
  />
</template>
```

## 自定义样式

### 修改颜色主题

```vue
<style scoped lang="scss">
// 在组件中覆盖颜色
.stat-card--primary .stat-card__icon {
  background: linear-gradient(135deg, #your-color-1 0%, #your-color-2 100%);
}
</style>
```

### 修改间距

```vue
<template>
  <el-row :gutter="32"> <!-- 修改卡片间距 -->
    <el-col :xs="24" :sm="12" :lg="6">
      <StatCard />
    </el-col>
  </el-row>
</template>
```

### 修改图表配置

```typescript
const customChartOptions = computed<ChartOptions>(() => ({
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      position: 'top', // 修改图例位置
      labels: {
        font: {
          size: 14 // 修改字体大小
        }
      }
    }
  },
  scales: {
    y: {
      beginAtZero: true,
      grid: {
        color: 'rgba(0, 0, 0, 0.1)' // 修改网格线颜色
      }
    }
  }
}))
```

## API集成示例

### 获取统计数据

```typescript
import { dashboardApi } from '@/api'

const loadStats = async () => {
  try {
    const stats = await dashboardApi.getStats()
    console.log('统计数据:', stats)
  } catch (error) {
    console.error('加载失败:', error)
  }
}
```

### 获取最近活动

```typescript
const loadActivities = async () => {
  try {
    const activities = await dashboardApi.getRecentActivities(10)
    console.log('最近活动:', activities)
  } catch (error) {
    console.error('加载失败:', error)
  }
}
```

### 更新待办状态

```typescript
const updateTodo = async (id: string, status: 'completed' | 'pending') => {
  try {
    const result = await dashboardApi.updateTodoStatus(id, status)
    console.log('更新成功:', result)
  } catch (error) {
    console.error('更新失败:', error)
  }
}
```

## 常见问题

### Q: 如何修改显示的组件数量？

```typescript
// 在 DashboardView.vue 中修改
const displayCount = ref(5) // 修改为你想要的数量
```

### Q: 如何禁用某个功能？

```vue
<!-- 添加 v-if 指令 -->
<RecentActivity v-if="showActivities" />
```

### Q: 如何自定义数据加载？

```typescript
// 替换默认的数据加载函数
const loadCustomData = async () => {
  // 你的自定义加载逻辑
  const data = await fetchCustomAPI()
  return data
}
```

### Q: 如何添加新的图表类型？

```vue
<ChartContainer
  type="radar" <!-- 使用支持的图表类型 -->
  :data="radarData"
/>
```

支持的图表类型: `line`, `bar`, `pie`, `doughnut`, `radar`, `polarArea`, `bubble`, `scatter`

### Q: 如何实现暗色主题？

```vue
<template>
  <div :class="{ 'dark-theme': isDark }">
    <DashboardView />
  </div>
</template>

<style>
.dark-theme {
  --bg-color: #1a1a1a;
  --text-color: #ffffff;
}
</style>
```

## 调试技巧

### 1. 查看组件数据

```vue
<script setup lang="ts">
import { watchEffect } from 'vue'

watchEffect(() => {
  console.log('当前数据:', dashboardStats.value)
})
</script>
```

### 2. 检查API响应

```typescript
const loadData = async () => {
  const response = await dashboardApi.getStats()
  console.log('API响应:', response)
  return response
}
```

### 3. 监听图表变化

```vue
<script setup lang="ts">
const chartRef = ref()

watch(() => chartData, (newData) => {
  console.log('图表数据已更新:', newData)
}, { deep: true })
</script>
```

## 性能优化建议

### 1. 使用防抖

```typescript
import { useDebounceFn } from '@vueuse/core'

const debouncedSearch = useDebounceFn((keyword) => {
  // 搜索逻辑
}, 300)
```

### 2. 懒加载组件

```vue
<script setup lang="ts">
const HeavyComponent = defineAsyncComponent(() =>
  import('./HeavyComponent.vue')
)
</script>
```

### 3. 虚拟滚动

```vue
<template>
  <RecycleScroller
    :items="largeList"
    :item-size="50"
    key-field="id"
  >
    <template #default="{ item }">
      <div>{{ item.name }}</div>
    </template>
  </RecycleScroller>
</template>
```

## 下一步

- 📖 阅读 [DASHBOARD_README.md](./DASHBOARD_README.md) 了解完整功能
- 🔧 查看 [DASHBOARD_IMPLEMENTATION.md](./DASHBOARD_IMPLEMENTATION.md) 了解实现细节
- 🎨 访问 `http://localhost:5173/dashboard` 体验仪表盘
- 💡 根据需求自定义和扩展功能

## 获取帮助

- 查看组件源码: `src/components/dashboard/`
- 查看类型定义: `src/types/dashboard.ts`
- 查看API文档: `src/api/modules/dashboard.ts`
- 查看使用示例: `src/views/dashboard/components.ts`

---

**祝你使用愉快！** 🚀
